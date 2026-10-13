#include "controllers/UploadController.h"
#include "config/AppConfig.h"

#include <drogon/MultiPart.h>
#include <drogon/drogon.h>
#include <openssl/sha.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

using namespace drogon;

namespace {

HttpResponsePtr jsonError(int status, const std::string &message) {
  Json::Value body;
  body["success"] = false;
  body["message"] = message;

  auto response = HttpResponse::newHttpJsonResponse(body);
  response->setStatusCode(static_cast<HttpStatusCode>(status));
  return response;
}

std::string computeSha1Hex(const std::string &input) {
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA1(reinterpret_cast<const unsigned char *>(input.data()), input.size(), hash);
  std::ostringstream oss;
  for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(hash[i]);
  }
  return oss.str();
}

bool isSafeFilename(const std::string &name) {
  if (name.empty() || name.size() > 255) return false;
  if (name.find("..") != std::string::npos) return false;
  if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) return false;
  for (unsigned char c : name) {
    if (c < 32 || c == 127) return false;
  }
  return true;
}

bool validateFileSignature(const std::string &ext, const std::string &bytes) {
  if (ext == "jpg" || ext == "jpeg") {
    if (bytes.size() < 3) return false;
    const auto *ub = reinterpret_cast<const unsigned char *>(bytes.data());
    return ub[0] == 0xFF && ub[1] == 0xD8 && ub[2] == 0xFF;
  }
  if (ext == "png") {
    if (bytes.size() < 8) return false;
    const auto *ub = reinterpret_cast<const unsigned char *>(bytes.data());
    return ub[0] == 0x89 && ub[1] == 0x50 && ub[2] == 0x4E && ub[3] == 0x47 &&
           ub[4] == 0x0D && ub[5] == 0x0A && ub[6] == 0x1A && ub[7] == 0x0A;
  }
  if (ext == "webp") {
    if (bytes.size() < 12) return false;
    return bytes.compare(0, 4, "RIFF") == 0 && bytes.compare(8, 4, "WEBP") == 0;
  }
  if (ext == "mp4") {
    if (bytes.size() < 8) return false;
    return bytes.compare(4, 4, "ftyp") == 0;
  }
  return false;
}

} // namespace

void UploadController::upload(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  // Confirm authenticated identity
  std::string userId;
  try {
    userId = req->attributes()->get<std::string>("userId");
  } catch (...) {}

  if (userId.empty()) {
    LOG_WARN << "[UploadController] Upload rejected: Missing user authentication";
    callback(jsonError(k401Unauthorized, "Authentication required to upload files"));
    return;
  }

  MultiPartParser parser;
  if (parser.parse(req) != 0) {
    LOG_ERROR << "[UploadController] Failed to parse multipart/form-data request";
    callback(jsonError(k400BadRequest, "Failed to parse multipart request"));
    return;
  }

  const auto &files = parser.getFiles();
  if (files.empty()) {
    LOG_ERROR << "[UploadController] No file provided in multipart payload";
    callback(jsonError(k400BadRequest, "No file uploaded in multipart payload"));
    return;
  }

  const auto &file = files.front();
  std::string rawFileName = std::string(file.getFileName());
  if (rawFileName.empty()) {
    callback(jsonError(k400BadRequest, "Filename cannot be empty"));
    return;
  }

  // Validate filename safety (reject directory traversal, dangerous paths)
  if (!isSafeFilename(rawFileName)) {
    LOG_WARN << "[UploadController] Malicious or invalid filename rejected: " << rawFileName;
    callback(jsonError(k400BadRequest, "Invalid filename: Directory traversal or invalid characters detected"));
    return;
  }

  std::string ext;
  auto dotPos = rawFileName.rfind('.');
  if (dotPos != std::string::npos && dotPos + 1 < rawFileName.size()) {
    ext = rawFileName.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
  }

  const bool isImage = (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "webp");
  const bool isVideo = (ext == "mp4");

  if (!isImage && !isVideo) {
    LOG_WARN << "[UploadController] Unsupported file extension rejected: " << ext;
    callback(jsonError(k400BadRequest,
                       "Unsupported file extension. Only JPG, JPEG, PNG, WEBP images and MP4 videos are supported."));
    return;
  }

  const size_t fileLen = file.fileLength();
  if (fileLen == 0) {
    callback(jsonError(k400BadRequest, "Uploaded file is empty"));
    return;
  }

  constexpr size_t kMaxImageSize = 10 * 1024 * 1024;  // 10 MB limit for images
  constexpr size_t kMaxVideoSize = 30 * 1024 * 1024;  // 30 MB limit for videos

  if (isImage && fileLen > kMaxImageSize) {
    LOG_WARN << "[UploadController] Oversized image rejected: " << fileLen << " bytes";
    callback(jsonError(k400BadRequest, "Image file exceeds maximum allowed limit of 10MB"));
    return;
  }

  if (isVideo && fileLen > kMaxVideoSize) {
    LOG_WARN << "[UploadController] Oversized video rejected: " << fileLen << " bytes";
    callback(jsonError(k400BadRequest, "Video file exceeds maximum allowed limit of 30MB"));
    return;
  }

  std::string fileBytes(file.fileData(), fileLen);
  if (!validateFileSignature(ext, fileBytes)) {
    LOG_WARN << "[UploadController] Spoofed or corrupt file signature rejected for: " << rawFileName;
    callback(jsonError(k400BadRequest,
                       "File signature validation failed. File content does not match extension."));
    return;
  }

  std::string resourceType = isVideo ? "video" : "image";
  std::string mimeType = "image/jpeg";
  if (ext == "png") {
    mimeType = "image/png";
  } else if (ext == "webp") {
    mimeType = "image/webp";
  } else if (ext == "mp4") {
    mimeType = "video/mp4";
  }

  LOG_INFO << "[UploadController] Authenticated upload accepted: user=" << userId
           << ", file=" << rawFileName << ", size=" << fileLen << " bytes, mime=" << mimeType;

  const auto &config = AppConfig::instance();
  const std::string &cloudName = config.cloudinaryCloudName();
  const std::string &apiKey = config.cloudinaryApiKey();
  const std::string &apiSecret = config.cloudinaryApiSecret();

  if (cloudName.empty() || apiKey.empty() || apiSecret.empty()) {
    LOG_ERROR << "[UploadController] Missing Cloudinary credentials in configuration";
    callback(jsonError(k500InternalServerError, "Cloudinary credentials not configured in backend"));
    return;
  }

  const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  const std::string timestamp = std::to_string(now);

  const std::string toSign = "timestamp=" + timestamp + apiSecret;
  const std::string signature = computeSha1Hex(toSign);

  const std::string boundary = "----CloudinaryBoundary" + std::to_string(now);
  std::string body;

  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"api_key\"\r\n\r\n";
  body += apiKey + "\r\n";

  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"timestamp\"\r\n\r\n";
  body += timestamp + "\r\n";

  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"signature\"\r\n\r\n";
  body += signature + "\r\n";

  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"file\"; filename=\"" + rawFileName + "\"\r\n";
  body += "Content-Type: " + mimeType + "\r\n\r\n";
  body.append(fileBytes);
  body += "\r\n";

  body += "--" + boundary + "--\r\n";

  // Native asynchronous HTTP upload to Cloudinary (No popen / curl shell fallback)
  auto client = HttpClient::newHttpClient("https://api.cloudinary.com", app().getLoop(), false, false);
  auto cloudReq = HttpRequest::newHttpRequest();
  cloudReq->setMethod(Post);
  cloudReq->setPath("/v1_1/" + cloudName + "/" + resourceType + "/upload");
  cloudReq->setContentTypeString("multipart/form-data; boundary=" + boundary);
  cloudReq->setBody(std::move(body));

  client->sendRequest(
      cloudReq,
      [callback](ReqResult result, const HttpResponsePtr &resp) {
        if (result == ReqResult::Ok && resp) {
          auto json = resp->getJsonObject();
          if (json && json->isMember("secure_url")) {
            std::string secureUrl = (*json)["secure_url"].asString();
            std::string publicId = (*json).get("public_id", "").asString();
            LOG_INFO << "[UploadController] [SUCCESS] Uploaded to Cloudinary: " << secureUrl
                     << " (public_id: " << publicId << ")";

            Json::Value out;
            out["success"] = true;
            out["url"] = secureUrl;
            out["secure_url"] = secureUrl;
            out["public_id"] = publicId;
            out["format"] = (*json).get("format", "").asString();

            auto okResponse = HttpResponse::newHttpJsonResponse(out);
            callback(okResponse);
            return;
          }

          std::string errMsg = (json && json->isMember("error") && (*json)["error"].isMember("message"))
                                   ? (*json)["error"]["message"].asString()
                                   : "Cloudinary upload rejected";
          LOG_ERROR << "[UploadController] Cloudinary returned error: " << errMsg;
          callback(jsonError(k502BadGateway, errMsg));
          return;
        }

        LOG_ERROR << "[UploadController] Drogon async HTTP client failed or timed out contacting Cloudinary";
        callback(jsonError(k502BadGateway, "Cloudinary upload service unavailable"));
      },
      30.0);
}

void UploadController::previewMedia(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  auto &params = req->getParameters();
  std::string targetUrl = "";
  if (params.find("url") != params.end()) {
    targetUrl = params.at("url");
  }

  if (targetUrl.empty()) {
    callback(jsonError(k400BadRequest, "Missing required query parameter: url"));
    return;
  }

  // 1. Local upload media path validation
  if (targetUrl.rfind("/uploads/", 0) == 0 || targetUrl.rfind("uploads/", 0) == 0) {
    if (targetUrl.find("..") != std::string::npos || targetUrl.find('\\') != std::string::npos) {
      LOG_WARN << "[UploadController] Path traversal attempted in local media preview: " << targetUrl;
      callback(jsonError(k400BadRequest, "Invalid local media path: Path traversal detected"));
      return;
    }

    std::string filename = (targetUrl.rfind("/uploads/", 0) == 0) ? targetUrl.substr(9) : targetUrl.substr(8);
    if (!isSafeFilename(filename)) {
      callback(jsonError(k400BadRequest, "Invalid filename in local media path"));
      return;
    }

    std::filesystem::path filePath = std::filesystem::path("./public/uploads") / filename;
    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
      auto resp = drogon::HttpResponse::newNotFoundResponse();
      callback(resp);
      return;
    }

    auto resp = drogon::HttpResponse::newFileResponse(filePath.string());
    callback(resp);
    return;
  }

  // 2. Cloudinary trusted media path validation
  const auto &config = AppConfig::instance();
  const std::string &cloudName = config.cloudinaryCloudName();

  if (cloudName.empty()) {
    callback(jsonError(k400BadRequest, "External media preview unavailable: Cloud name not configured"));
    return;
  }

  const std::string expectedPrefix = "https://res.cloudinary.com/" + cloudName + "/";
  if (targetUrl.rfind(expectedPrefix, 0) == 0) {
    int w = 1200;
    int h = 1200;
    if (params.find("w") != params.end()) {
      try { w = std::stoi(params.at("w")); } catch (...) {}
    }
    if (params.find("h") != params.end()) {
      try { h = std::stoi(params.at("h")); } catch (...) {}
    }
    std::string fit = "limit";
    if (params.find("fit") != params.end()) {
      fit = params.at("fit");
    }

    std::string transform = "c_" + fit + ",w_" + std::to_string(w) + ",h_" + std::to_string(h) + ",q_auto,f_jpg/";
    std::string previewUrl = targetUrl;
    auto uploadPos = previewUrl.find("/image/upload/");
    if (uploadPos != std::string::npos && previewUrl.find("/image/upload/c_") == std::string::npos) {
      previewUrl.insert(uploadPos + std::string("/image/upload/").length(), transform);
    }

    if (params.find("format") != params.end() && params.at("format") == "json") {
      Json::Value out;
      out["success"] = true;
      out["originalUrl"] = targetUrl;
      out["previewUrl"] = previewUrl;
      out["width"] = w;
      out["height"] = h;
      auto resp = drogon::HttpResponse::newHttpJsonResponse(out);
      callback(resp);
      return;
    }

    auto resp = drogon::HttpResponse::newRedirectionResponse(previewUrl);
    callback(resp);
    return;
  }

  // 3. Reject any arbitrary external or untrusted URL
  LOG_WARN << "[UploadController] Blocked untrusted redirect attempt: " << targetUrl;
  callback(jsonError(k400BadRequest, "Untrusted redirect target: URL is not an allowed local or Cloudinary media path"));
}
