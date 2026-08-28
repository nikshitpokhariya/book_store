#include "repositories/UserRepository.h"

#include "db/MongoDatabase.h"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/oid.hpp>

#include <mongocxx/exception/exception.hpp>

using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;

namespace {
User toUser(const bsoncxx::document::view &view) {
  User user;

  user.id = view["_id"].get_oid().value.to_string();

  user.username = std::string(view["username"].get_string().value.data(),
                              view["username"].get_string().value.size());

  user.email = std::string(view["email"].get_string().value.data(),
                           view["email"].get_string().value.size());

  user.passwordHash =
      std::string(view["passwordHash"].get_string().value.data(),
                  view["passwordHash"].get_string().value.size());

  return user;
}
} // namespace

std::optional<User> UserRepository::findById(const std::string &id) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{id};
  } catch (const std::exception &) {
    return std::nullopt;
  }

  auto collection = MongoDatabase::instance().database()["users"];

  document filter;
  filter.append(kvp("_id", objectId));
  auto result = collection.find_one(filter.view());

  if (!result)
    return std::nullopt;

  return toUser(result->view());
}

std::optional<User> UserRepository::findByEmail(const std::string &email) {
  auto collection = MongoDatabase::instance().database()["users"];

  document filter;
  filter.append(kvp("email", email));
  auto result = collection.find_one(filter.view());

  if (!result)
    return std::nullopt;

  return toUser(result->view());
}

std::optional<User>
UserRepository::findByUsername(const std::string &username) {
  auto collection = MongoDatabase::instance().database()["users"];

  document filter;
  filter.append(kvp("username", username));
  auto result = collection.find_one(filter.view());

  if (!result)
    return std::nullopt;

  return toUser(result->view());
}

bool UserRepository::existsByEmail(const std::string &email) {
  return findByEmail(email).has_value();
}

bool UserRepository::existsByUsername(const std::string &username) {
  return findByUsername(username).has_value();
}

std::string UserRepository::create(const User &user) {
  auto collection = MongoDatabase::instance().database()["users"];

  document doc;
  doc.append(kvp("username", user.username));
  doc.append(kvp("email", user.email));
  doc.append(kvp("passwordHash", user.passwordHash));
  doc.append(kvp("createdAt",
                 bsoncxx::types::b_date{std::chrono::system_clock::now()}));
  doc.append(kvp("updatedAt",
                 bsoncxx::types::b_date{std::chrono::system_clock::now()}));

  auto result = collection.insert_one(doc.view());

  if (!result) {
    throw std::runtime_error("USER_CREATE_FAILED");
  }

  return result->inserted_id().get_oid().value.to_string();
}