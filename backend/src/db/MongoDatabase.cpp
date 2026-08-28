#include "db/MongoDatabase.h"
#include "config/AppConfig.h"

#include <mongocxx/options/index.hpp>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

MongoDatabase::MongoDatabase()
    : client_{mongocxx::uri{AppConfig::instance().mongoUri()}} {}

MongoDatabase &MongoDatabase::instance() {
  static MongoDatabase instance;
  return instance;
}

mongocxx::database MongoDatabase::database() {
  return client_[AppConfig::instance().mongoDatabase()];
}

void MongoDatabase::ensureIndexes() {
  auto db = database();

  auto users = db["users"];
  auto sessions = db["sessions"];

  // users.username UNIQUE
  {
    mongocxx::options::index options;
    options.unique(true);

    users.create_index(make_document(kvp("username", 1)), options);
  }

  // users.email UNIQUE
  {
    mongocxx::options::index options;
    options.unique(true);

    users.create_index(make_document(kvp("email", 1)), options);
  }

  // sessions.refreshTokenHash UNIQUE
  {
    mongocxx::options::index options;
    options.unique(true);

    sessions.create_index(make_document(kvp("refreshTokenHash", 1)), options);
  }

  // Automatically remove expired sessions
  {
    mongocxx::options::index options;
    options.expire_after(std::chrono::seconds(0));

    sessions.create_index(make_document(kvp("expiresAt", 1)), options);
  }
}