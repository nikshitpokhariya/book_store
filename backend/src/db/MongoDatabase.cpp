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
  auto books = db["books"];

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

  // Index for browsing available books by creation date
  {
    books.create_index(make_document(kvp("status", 1), kvp("createdAt", -1)));
  }

  // Index for owner listings
  {
    books.create_index(make_document(kvp("ownerId", 1), kvp("createdAt", -1)));
  }

  // Compound index for status, category, and price filtering
  {
    books.create_index(
        make_document(kvp("status", 1), kvp("category", 1), kvp("price", 1)));
  }

  // Compound index for status and price sorting
  {
    books.create_index(make_document(kvp("status", 1), kvp("price", 1)));
  }

  // Index for ISBN lookup
  {
    books.create_index(make_document(kvp("isbn", 1)));
  }

  auto carts = db["carts"];

  // carts.userId UNIQUE
  {
    mongocxx::options::index options;
    options.unique(true);
    carts.create_index(make_document(kvp("userId", 1)), options);
  }

  auto orders = db["orders"];

  // orders.orderNumber UNIQUE
  {
    mongocxx::options::index options;
    options.unique(true);
    orders.create_index(make_document(kvp("orderNumber", 1)), options);
  }

  // Buyer order history index
  {
    orders.create_index(make_document(kvp("buyerId", 1), kvp("createdAt", -1)));
  }

  // Seller sales history index
  {
    orders.create_index(
        make_document(kvp("items.sellerId", 1), kvp("createdAt", -1)));
  }

  // Book reference index
  {
    orders.create_index(make_document(kvp("items.bookId", 1)));
  }

  auto exchanges = db["exchanges"];

  // exchanges.exchangeNumber UNIQUE
  {
    mongocxx::options::index options;
    options.unique(true);
    exchanges.create_index(make_document(kvp("exchangeNumber", 1)), options);
  }

  // Sent exchanges query
  {
    exchanges.create_index(
        make_document(kvp("requesterId", 1), kvp("createdAt", -1)));
  }

  // Received exchanges query
  {
    exchanges.create_index(
        make_document(kvp("receiverId", 1), kvp("createdAt", -1)));
  }

  // Book reference indexes
  {
    exchanges.create_index(make_document(kvp("requestedBook.bookId", 1)));
    exchanges.create_index(make_document(kvp("offeredBook.bookId", 1)));
  }

  // Status index
  {
    exchanges.create_index(
        make_document(kvp("status", 1), kvp("createdAt", -1)));
  }

  // Order status index
  {
    orders.create_index(make_document(kvp("status", 1), kvp("createdAt", -1)));
  }

  auto reviews = db["reviews"];

  // reviews: bookId index for public listing
  {
    reviews.create_index(make_document(kvp("bookId", 1), kvp("createdAt", -1)));
  }

  // reviews: reviewerId index for user's own reviews
  {
    reviews.create_index(
        make_document(kvp("reviewerId", 1), kvp("createdAt", -1)));
  }

  // reviews: unique compound index { orderId: 1, bookId: 1 } to prevent duplicate reviews
  {
    mongocxx::options::index options;
    options.unique(true);
    reviews.create_index(
        make_document(kvp("orderId", 1), kvp("bookId", 1)), options);
  }
}