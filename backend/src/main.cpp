#include <drogon/drogon.h>

#include "config/AppConfig.h"
#include "controllers/AuthController.h"
#include "controllers/BookController.h"
#include "controllers/CartController.h"
#include "controllers/ExchangeController.h"
#include "controllers/OrderController.h"
#include "controllers/ReviewController.h"
#include "controllers/UploadController.h"
#include "db/MongoDatabase.h"

int main() {
  std::cout << "[INFO] Starting BookBazzar Backend main()..." << std::endl;
  std::cout.flush();

  try {
    std::cout << "[INFO] Loading AppConfig..." << std::endl;
    std::cout.flush();
    AppConfig::instance();

    std::cout << "[INFO] Initializing MongoDB connection to " << AppConfig::instance().mongoUri() << "..." << std::endl;
    std::cout.flush();
    auto &mongo = MongoDatabase::instance();

    std::cout << "[INFO] Ensuring indexes on database: " << AppConfig::instance().mongoDatabase() << "..." << std::endl;
    std::cout.flush();
    mongo.ensureIndexes();
    std::cout << "[INFO] Indexes verified successfully." << std::endl;
    std::cout.flush();

    AuthController::initPathRouting();
    BookController::initPathRouting();
    CartController::initPathRouting();
    ExchangeController::initPathRouting();
    OrderController::initPathRouting();
    ReviewController::initPathRouting();
    UploadController::initPathRouting();

    drogon::app().registerHandler(
        "/uploads/{filename}",
        [](const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback,
           const std::string &filename) {
          std::string path = "./public/uploads/" + filename;
          auto resp = drogon::HttpResponse::newFileResponse(path);
          if (filename.find(".mp4") != std::string::npos) {
            resp->setContentTypeCode(drogon::CT_CUSTOM);
            resp->setContentTypeString("video/mp4");
          }
          callback(resp);
        },
        {drogon::Get});

    drogon::app()
        .setClientMaxBodySize(50 * 1024 * 1024)
        .setClientMaxMemoryBodySize(50 * 1024 * 1024)
        .loadConfigFile("config.json")
        .run();
  } catch (const std::exception &e) {
    std::cerr << "[FATAL] Application startup failed: " << e.what() << std::endl;
    std::cerr.flush();
    std::cout << "[FATAL] Application startup failed: " << e.what() << std::endl;
    std::cout.flush();
    LOG_ERROR << "Application startup failed: " << e.what();

    return 1;
  } catch (...) {
    std::cerr << "[FATAL] Application startup failed with unknown exception." << std::endl;
    std::cerr.flush();
    std::cout << "[FATAL] Application startup failed with unknown exception." << std::endl;
    std::cout.flush();
    return 1;
  }

  return 0;
}