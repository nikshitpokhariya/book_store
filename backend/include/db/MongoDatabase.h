#pragma once

#include <mongocxx/client.hpp>
#include <mongocxx/client_session.hpp>
#include <mongocxx/instance.hpp>
#include <mutex>

enum class MongoTransactionSupport {
  UNKNOWN,       // Topology probe failed, timed out, or unverified
  SUPPORTED,     // Replica set or sharded cluster verified
  UNSUPPORTED    // Standalone deployment verified (no transaction support)
};

class MongoDatabase {
public:
  static MongoDatabase &instance();

  mongocxx::database database();
  mongocxx::client &client();
  mongocxx::client_session startSession(const mongocxx::options::client_session &options = {});

  // Evaluates or returns cached transaction capability.
  // Avoids unnecessary per-request topology round trips.
  bool supportsTransactions();
  MongoTransactionSupport transactionSupportStatus();

  void ensureIndexes();

private:
  MongoDatabase();
  void initTransactionSupport();

  mongocxx::instance instance_;
  mongocxx::client client_;
  MongoTransactionSupport transactionSupport_{MongoTransactionSupport::UNKNOWN};
  std::mutex supportMutex_;
};