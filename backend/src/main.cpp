#include <drogon/drogon.h>

#include "config/AppConfig.h"
#include "controllers/AuthController.h"
#include "controllers/BookController.h"
#include "db/MongoDatabase.h"

int main() {
  try {
    AppConfig::instance();

    auto &mongo = MongoDatabase::instance();

    mongo.ensureIndexes();

    AuthController::initPathRouting();
    BookController::initPathRouting();

    drogon::app().loadConfigFile("config.json").run();
  } catch (const std::exception &e) {
    LOG_ERROR << "Application startup failed: " << e.what();

    return 1;
  }

  return 0;
}