#include <drogon/drogon.h>

#include <filesystem>
#include <iostream>
#include <string>

#include "config/AppConfig.h"
#include "controllers/AuthController.h"
#include "controllers/BookController.h"
#include "controllers/CartController.h"
#include "controllers/ExchangeController.h"
#include "controllers/OrderController.h"
#include "controllers/ReviewController.h"
#include "controllers/UploadController.h"
#include "db/MongoDatabase.h"

namespace {
std::string sanitizeErrorMessage(std::string msg) {
  for (const std::string &scheme : {"mongodb://", "mongodb+srv://"}) {
    size_t pos = 0;
    while ((pos = msg.find(scheme, pos)) != std::string::npos) {
      size_t credStart = pos + scheme.length();
      size_t atPos = msg.find('@', credStart);
      size_t colonPos = msg.find(':', credStart);
      if (atPos != std::string::npos && colonPos != std::string::npos && colonPos < atPos) {
        msg.replace(colonPos + 1, atPos - colonPos - 1, "***");
        pos = atPos + 1;
      } else {
        pos = credStart;
      }
    }
  }
  return msg;
}
} // namespace

int main() {
  std::cout << "[INFO] Starting BookBazzar Backend main()..." << std::endl;
  std::cout.flush();

  try {
    std::cout << "[INFO] Loading AppConfig..." << std::endl;
    std::cout.flush();

    const auto &config = AppConfig::instance();

    std::cout << "[INFO] Initializing MongoDB connection..." << std::endl;
    std::cout.flush();

    auto &mongo = MongoDatabase::instance();

    std::cout << "[INFO] Ensuring indexes on database: "
              << config.mongoDatabase() << "..." << std::endl;
    std::cout.flush();

    mongo.ensureIndexes();

    std::cout << "[INFO] Indexes verified successfully." << std::endl;
    std::cout.flush();

    // Ensure local uploads directory exists
    std::error_code ec;
    std::filesystem::create_directories("./public/uploads", ec);
    if (ec) {
      LOG_WARN << "Failed to create directory ./public/uploads: " << ec.message();
    }

    AuthController::initPathRouting();
    BookController::initPathRouting();
    CartController::initPathRouting();
    ExchangeController::initPathRouting();
    OrderController::initPathRouting();
    ReviewController::initPathRouting();
    UploadController::initPathRouting();

    // Static upload route with path traversal protection and 404 handling
    drogon::app().registerHandler(
        "/uploads/{filename}",
        [](const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback,
           const std::string &filename) {
          if (filename.find("..") != std::string::npos ||
              filename.find('/') != std::string::npos ||
              filename.find('\\') != std::string::npos) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
          }
          std::filesystem::path filePath =
              std::filesystem::path("./public/uploads") / filename;
          if (!std::filesystem::exists(filePath) ||
              !std::filesystem::is_regular_file(filePath)) {
            auto resp = drogon::HttpResponse::newNotFoundResponse();
            callback(resp);
            return;
          }
          auto resp = drogon::HttpResponse::newFileResponse(filePath.string());
          if (filename.find(".mp4") != std::string::npos) {
            resp->setContentTypeCode(drogon::CT_CUSTOM);
            resp->setContentTypeString("video/mp4");
          }
          callback(resp);
        },
        {drogon::Get});

    // Lightweight, non-sensitive health check endpoints
    auto healthHandler =
        [](const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
          Json::Value body;
          body["status"] = "ok";
          body["service"] = "BookBazzar Backend";
          auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
          resp->setStatusCode(drogon::k200OK);
          callback(resp);
        };

    drogon::app().registerHandler("/health", healthHandler, {drogon::Get});
    drogon::app().registerHandler("/api/health", healthHandler, {drogon::Get});

    std::cout << "[INFO] Binding Drogon listener to 0.0.0.0:" << config.port()
              << "..." << std::endl;
    std::cout.flush();

    drogon::app()
        .loadConfigFile("config.json")
        .setClientMaxBodySize(50 * 1024 * 1024)
        .setClientMaxMemoryBodySize(50 * 1024 * 1024);

    const char *logLevelEnv = std::getenv("LOG_LEVEL");
    if (logLevelEnv) {
      std::string lvl(logLevelEnv);
      if (lvl == "WARN" || lvl == "WARNING") {
        drogon::app().setLogLevel(trantor::Logger::kWarn);
      } else if (lvl == "ERROR") {
        drogon::app().setLogLevel(trantor::Logger::kError);
      } else if (lvl == "DEBUG") {
        drogon::app().setLogLevel(trantor::Logger::kDebug);
      } else {
        drogon::app().setLogLevel(trantor::Logger::kInfo);
      }
    } else {
      drogon::app().setLogLevel(trantor::Logger::kInfo);
    }

    drogon::app()
        .addListener("0.0.0.0", config.port())
        .run();
  } catch (const std::exception &e) {
    std::string safeError = sanitizeErrorMessage(e.what());
    std::cerr << "[FATAL] Application startup failed: " << safeError << std::endl;
    std::cerr.flush();
    std::cout << "[FATAL] Application startup failed: " << safeError << std::endl;
    std::cout.flush();
    LOG_ERROR << "Application startup failed: " << safeError;

    return 1;
  } catch (...) {
    std::cerr << "[FATAL] Application startup failed with unknown exception."
              << std::endl;
    std::cerr.flush();
    std::cout << "[FATAL] Application startup failed with unknown exception."
              << std::endl;
    std::cout.flush();
    return 1;
  }

  return 0;
}