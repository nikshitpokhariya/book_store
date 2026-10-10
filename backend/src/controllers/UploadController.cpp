#include "controllers/UploadController.h"
#include "config/AppConfig.h"

#include <drogon/MultiPart.h>
#include <drogon/drogon.h>
#include <openssl/sha.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#endif

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

std::string getTempFilePath(const std::string &prefix, const std::string &ext) {
  auto now = std::chrono::duration_cast<std::chrono::microseconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
#ifdef _WIN32
  char tempDir[MAX_PATH];
  DWORD len = GetTempPathA(MAX_PATH, tempDir);
  std::string dir = (len > 0 && len < MAX_PATH) ? std::string(tempDir) : ".\\";
  return dir + prefix + "_" + std::to_string(now) + (ext.empty() ? "" : "." + ext);
#else
  return "/tmp/" + prefix + "_" + std::to_string(now) + (ext.empty() ? "" : "." + ext);
#endif
}

void uploadViaCurlFallback(
    const std::string &cloudName, const std::string &resourceType,
    const std::string &apiKey, const std::string &timestamp,
    const std::string &signature, const std::string &fileName,
    const std::string &fileBytes,
    std::function<void(const HttpResponsePtr &)> callback) {
  std::string ext;
  auto dotPos = fileName.rfind('.');
  if (dotPos != std::string::npos) {
    ext = fileName.substr(dotPos + 1);
  }

  std::string tempFile = getTempFilePath("cld_up", ext);
  std::ofstream out(tempFile, std::ios::binary);
  if (!out.is_open()) {
    callback(jsonError(k500InternalServerError, "Failed to create temporary file for upload"));
    return;
  }
  out.write(fileBytes.data(), fileBytes.size());
  out.close();

  std::string url = "https://api.cloudinary.com/v1_1/" + cloudName + "/" + resourceType + "/upload";
  std::string cmd = "curl.exe -s -X POST \"" + url + "\" " +
                    "-F \"file=@" + tempFile + "\" " +
                    "-F \"api_key=" + apiKey + "\" " +
                    "-F \"timestamp=" + timestamp + "\" " +
                    "-F \"signature=" + signature + "\"";

  std::string responseOutput;
#ifdef _WIN32
  FILE *pipe = _popen(cmd.c_str(), "r");
#else
  FILE *pipe = popen(cmd.c_str(), "r");
#endif

  if (pipe) {
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      responseOutput += buffer;
    }
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
  }

  // Delete temp file
  std::remove(tempFile.c_str());

  if (responseOutput.empty()) {
    callback(jsonError(k502BadGateway, "Empty response from Cloudinary upload service"));
    return;
  }

  Json::CharReaderBuilder readerBuilder;
  Json::Value root;
  std::string parseErrors;
  std::istringstream sstr(responseOutput);
  if (!Json::parseFromStream(readerBuilder, sstr, &root, &parseErrors)) {
    callback(jsonError(k502BadGateway, "Failed to parse Cloudinary response: " + parseErrors));
    return;
  }

  if (root.isMember("secure_url")) {
    Json::Value result;
    result["success"] = true;
    result["url"] = root["secure_url"].asString();
    result["secure_url"] = root["secure_url"].asString();
    result["public_id"] = root.get("public_id", "").asString();
    result["format"] = root.get("format", "").asString();

    auto response = HttpResponse::newHttpJsonResponse(result);
    callback(response);
    return;
  }

  std::string errMsg = root.isMember("error") && root["error"].isMember("message")
                           ? root["error"]["message"].asString()
                           : "Cloudinary upload failed";
  callback(jsonError(k500InternalServerError, errMsg));
}

} // namespace

void UploadController::upload(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  MultiPartParser parser;
  if (parser.parse(req) != 0) {
    callback(jsonError(k400BadRequest, "Failed to parse multipart request"));
    return;
  }

  const auto &files = parser.getFiles();
  if (files.empty()) {
    callback(jsonError(k400BadRequest, "No file uploaded in multipart payload"));
    return;
  }

  const auto &file = files.front();
  std::string fileName = std::string(file.getFileName());
  if (fileName.empty()) {
    fileName = "upload.bin";
  }

  std::string ext;
  auto dotPos = fileName.rfind('.');
  if (dotPos != std::string::npos) {
    ext = fileName.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  }

  std::string resourceType = "image";
  if (ext == "mp4" || ext == "mov" || ext == "avi" || ext == "webm" ||
      ext == "mkv" || ext == "flv" || ext == "m4v") {
    resourceType = "video";
  }

  std::string mimeType = "application/octet-stream";
  if (ext == "jpg" || ext == "jpeg") mimeType = "image/jpeg";
  else if (ext == "png") mimeType = "image/png";
  else if (ext == "webp") mimeType = "image/webp";
  else if (ext == "gif") mimeType = "image/gif";
  else if (ext == "mp4") mimeType = "video/mp4";
  else if (ext == "mov") mimeType = "video/quicktime";
  else if (ext == "webm") mimeType = "video/webm";

  const auto &config = AppConfig::instance();
  const std::string &cloudName = config.cloudinaryCloudName();
  const std::string &apiKey = config.cloudinaryApiKey();
  const std::string &apiSecret = config.cloudinaryApiSecret();

  const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  const std::string timestamp = std::to_string(now);

  const std::string toSign = "timestamp=" + timestamp + apiSecret;
  const std::string signature = computeSha1Hex(toSign);

  std::string fileBytes(file.fileData(), file.fileLength());

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
  body += "Content-Disposition: form-data; name=\"file\"; filename=\"" + fileName + "\"\r\n";
  body += "Content-Type: " + mimeType + "\r\n\r\n";
  body.append(fileBytes);
  body += "\r\n";

  body += "--" + boundary + "--\r\n";

  // Attempt Drogon HttpClient first
  auto client = HttpClient::newHttpClient("https://api.cloudinary.com", app().getLoop(), false, false);
  auto cloudReq = HttpRequest::newHttpRequest();
  cloudReq->setMethod(Post);
  cloudReq->setPath("/v1_1/" + cloudName + "/" + resourceType + "/upload");
  cloudReq->setContentTypeCodeAndCustomString(CT_CUSTOM, "multipart/form-data; boundary=" + boundary);
  cloudReq->setBody(std::move(body));

  client->sendRequest(
      cloudReq,
      [callback, cloudName, resourceType, apiKey, timestamp, signature, fileName,
       fileBytes = std::move(fileBytes)](ReqResult result, const HttpResponsePtr &resp) {
        if (result == ReqResult::Ok && resp) {
          auto json = resp->getJsonObject();
          if (json && json->isMember("secure_url")) {
            Json::Value out;
            out["success"] = true;
            out["url"] = (*json)["secure_url"].asString();
            out["secure_url"] = (*json)["secure_url"].asString();
            out["public_id"] = (*json).get("public_id", "").asString();
            out["format"] = (*json).get("format", "").asString();

            auto okResponse = HttpResponse::newHttpJsonResponse(out);
            callback(okResponse);
            return;
          }
        }

        // If Drogon HttpClient fails (e.g. TLS certificate validation or loop timeout),
        // use resilient native curl fallback
        uploadViaCurlFallback(cloudName, resourceType, apiKey, timestamp, signature,
                              fileName, fileBytes, callback);
      },
      30.0); // 30 seconds timeout for media upload
}
