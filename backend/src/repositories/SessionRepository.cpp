#include "repositories/SessionRepository.h"

#include "config/AppConfig.h"
#include "db/MongoDatabase.h"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>

#include <optional>

using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;

void SessionRepository::create(const std::string &userId,
                               const std::string &refreshTokenHash) {
  auto collection = MongoDatabase::instance().database()["sessions"];

  const auto now = std::chrono::system_clock::now();

  const auto expiresAt =
      now + std::chrono::seconds(AppConfig::instance().refreshTokenLifetime());

  document session;
  session.append(kvp("userId", userId));
  session.append(kvp("refreshTokenHash", refreshTokenHash));
  session.append(kvp("createdAt", bsoncxx::types::b_date{now}));
  session.append(kvp("expiresAt", bsoncxx::types::b_date{expiresAt}));
  collection.insert_one(session.view());
}

bool SessionRepository::exists(const std::string &refreshTokenHash) {
  auto collection = MongoDatabase::instance().database()["sessions"];

  document expiryFilter;
  expiryFilter.append(
      kvp("$gt", bsoncxx::types::b_date{std::chrono::system_clock::now()}));

  document filter;
  filter.append(kvp("refreshTokenHash", refreshTokenHash));
  filter.append(kvp("expiresAt", expiryFilter.view()));
  auto result = collection.find_one(filter.view());

  return result.has_value();
}

std::optional<Session> SessionRepository::findByHash(
    const std::string &refreshTokenHash) {
  auto collection = MongoDatabase::instance().database()["sessions"];

  document expiryFilter;
  expiryFilter.append(
      kvp("$gt", bsoncxx::types::b_date{std::chrono::system_clock::now()}));

  document filter;
  filter.append(kvp("refreshTokenHash", refreshTokenHash));
  filter.append(kvp("expiresAt", expiryFilter.view()));

  auto result = collection.find_one(filter.view());

  if (!result)
    return std::nullopt;

  Session session;
  session.userId =
      std::string((*result)["userId"].get_string().value.data(),
                  (*result)["userId"].get_string().value.size());
  session.refreshTokenHash = refreshTokenHash;

  return session;
}

void SessionRepository::remove(const std::string &refreshTokenHash) {
  auto collection = MongoDatabase::instance().database()["sessions"];

  document filter;
  filter.append(kvp("refreshTokenHash", refreshTokenHash));
  collection.delete_one(filter.view());
}

void SessionRepository::rotate(const std::string &oldRefreshTokenHash,
                               const std::string &newRefreshTokenHash,
                               const std::string &userId) {
  auto collection = MongoDatabase::instance().database()["sessions"];

  const auto now = std::chrono::system_clock::now();

  const auto expiresAt =
      now + std::chrono::seconds(AppConfig::instance().refreshTokenLifetime());

  document filter;
  filter.append(kvp("refreshTokenHash", oldRefreshTokenHash));

  document setFields;
  setFields.append(kvp("userId", userId));
  setFields.append(kvp("refreshTokenHash", newRefreshTokenHash));
  setFields.append(kvp("expiresAt", bsoncxx::types::b_date{expiresAt}));

  document update;
  update.append(kvp("$set", setFields.view()));
  collection.update_one(filter.view(), update.view());
}