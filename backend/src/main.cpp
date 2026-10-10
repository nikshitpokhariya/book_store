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
  try {
    AppConfig::instance();

    auto &mongo = MongoDatabase::instance();

    mongo.ensureIndexes();

    AuthController::initPathRouting();
    BookController::initPathRouting();
    CartController::initPathRouting();
    ExchangeController::initPathRouting();
    OrderController::initPathRouting();
    ReviewController::initPathRouting();
    UploadController::initPathRouting();

    drogon::app().loadConfigFile("config.json").run();
  } catch (const std::exception &e) {
    LOG_ERROR << "Application startup failed: " << e.what();

    return 1;
  }

  return 0;
}