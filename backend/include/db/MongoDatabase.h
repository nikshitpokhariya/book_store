#pragma once

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

class MongoDatabase {
public:
  static MongoDatabase &instance();

  mongocxx::database database();

  void ensureIndexes();

private:
  MongoDatabase();

  mongocxx::instance instance_;
  mongocxx::client client_;
};