#include "repositories/ReviewRepository.h"
#include "db/MongoDatabase.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/options/find.hpp>

#include <algorithm>
#include <cmath>

using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

namespace {
std::string getString(const bsoncxx::document::view &view, const char *key) {
  if (view[key] && view[key].type() == bsoncxx::type::k_string) {
    return std::string(view[key].get_string().value.data(),
                       view[key].get_string().value.size());
  }
  return "";
}

int getInt(const bsoncxx::document::view &view, const char *key,
           int defaultValue = 0) {
  if (!view[key])
    return defaultValue;
  if (view[key].type() == bsoncxx::type::k_int32) {
    return view[key].get_int32().value;
  }
  if (view[key].type() == bsoncxx::type::k_int64) {
    return static_cast<int>(view[key].get_int64().value);
  }
  return defaultValue;
}

Review toReview(const bsoncxx::document::view &view) {
  Review rev;
  rev.id = view["_id"].get_oid().value.to_string();
  rev.bookId = getString(view, "bookId");
  rev.orderId = getString(view, "orderId");
  rev.reviewerId = getString(view, "reviewerId");
  rev.reviewerUsername = getString(view, "reviewerUsername");
  rev.rating = getInt(view, "rating", 5);
  rev.comment = getString(view, "comment");

  if (view["createdAt"] && view["createdAt"].type() == bsoncxx::type::k_date) {
    rev.createdAt = std::chrono::system_clock::time_point{
        view["createdAt"].get_date().value};
  }
  if (view["updatedAt"] && view["updatedAt"].type() == bsoncxx::type::k_date) {
    rev.updatedAt = std::chrono::system_clock::time_point{
        view["updatedAt"].get_date().value};
  }

  return rev;
}
} // namespace

std::string ReviewRepository::create(const Review &review) {
  auto collection = MongoDatabase::instance().database()["reviews"];
  const auto now = std::chrono::system_clock::now();

  document doc;
  doc.append(kvp("bookId", review.bookId));
  doc.append(kvp("orderId", review.orderId));
  doc.append(kvp("reviewerId", review.reviewerId));
  doc.append(kvp("reviewerUsername", review.reviewerUsername));
  doc.append(kvp("rating", review.rating));
  doc.append(kvp("comment", review.comment));
  doc.append(kvp("createdAt", bsoncxx::types::b_date{now}));
  doc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  auto res = collection.insert_one(doc.view());
  if (!res) {
    throw std::runtime_error("REVIEW_CREATE_FAILED");
  }

  return res->inserted_id().get_oid().value.to_string();
}

std::optional<Review> ReviewRepository::findById(const std::string &id) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{id};
  } catch (...) {
    return std::nullopt;
  }

  auto collection = MongoDatabase::instance().database()["reviews"];
  auto doc = collection.find_one(make_document(kvp("_id", objectId)));
  if (!doc) {
    return std::nullopt;
  }

  return toReview(doc->view());
}

std::optional<Review>
ReviewRepository::findByOrderAndBook(const std::string &orderId,
                                    const std::string &bookId) {
  auto collection = MongoDatabase::instance().database()["reviews"];
  auto doc = collection.find_one(
      make_document(kvp("orderId", orderId), kvp("bookId", bookId)));
  if (!doc) {
    return std::nullopt;
  }

  return toReview(doc->view());
}

PageResult<Review> ReviewRepository::findByBookId(const std::string &bookId,
                                                  int page, int limit) {
  auto collection = MongoDatabase::instance().database()["reviews"];

  document filter;
  filter.append(kvp("bookId", bookId));

  const auto total = collection.count_documents(filter.view());

  mongocxx::options::find options;
  options.sort(make_document(kvp("createdAt", -1)));
  options.skip(static_cast<std::int64_t>((page - 1) * limit));
  options.limit(static_cast<std::int64_t>(limit));

  auto cursor = collection.find(filter.view(), options);

  std::vector<Review> items;
  for (const auto &doc : cursor) {
    items.push_back(toReview(doc));
  }

  const int totalPages =
      limit > 0 ? static_cast<int>((total + limit - 1) / limit) : 0;

  PageResult<Review> result;
  result.items = std::move(items);
  result.total = total;
  result.page = page;
  result.limit = limit;
  result.totalPages = totalPages;
  result.hasNextPage = page < totalPages;
  result.hasPrevPage = page > 1;

  return result;
}

PageResult<Review> ReviewRepository::findByReviewerId(
    const std::string &reviewerId, int page, int limit) {
  auto collection = MongoDatabase::instance().database()["reviews"];

  document filter;
  filter.append(kvp("reviewerId", reviewerId));

  const auto total = collection.count_documents(filter.view());

  mongocxx::options::find options;
  options.sort(make_document(kvp("createdAt", -1)));
  options.skip(static_cast<std::int64_t>((page - 1) * limit));
  options.limit(static_cast<std::int64_t>(limit));

  auto cursor = collection.find(filter.view(), options);

  std::vector<Review> items;
  for (const auto &doc : cursor) {
    items.push_back(toReview(doc));
  }

  const int totalPages =
      limit > 0 ? static_cast<int>((total + limit - 1) / limit) : 0;

  PageResult<Review> result;
  result.items = std::move(items);
  result.total = total;
  result.page = page;
  result.limit = limit;
  result.totalPages = totalPages;
  result.hasNextPage = page < totalPages;
  result.hasPrevPage = page > 1;

  return result;
}

bool ReviewRepository::update(const std::string &id, int rating,
                              const std::string &comment) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{id};
  } catch (...) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["reviews"];
  const auto now = std::chrono::system_clock::now();

  document filter;
  filter.append(kvp("_id", objectId));

  document setDoc;
  setDoc.append(kvp("rating", rating));
  setDoc.append(kvp("comment", comment));
  setDoc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  auto res = collection.update_one(filter.view(),
                                   make_document(kvp("$set", setDoc.view())));
  return res && res->modified_count() > 0;
}

bool ReviewRepository::deleteById(const std::string &id) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{id};
  } catch (...) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["reviews"];
  auto res =
      collection.delete_one(make_document(kvp("_id", objectId)));
  return res && res->deleted_count() > 0;
}

std::pair<double, int>
ReviewRepository::calculateRatingAggregate(const std::string &bookId) {
  auto collection = MongoDatabase::instance().database()["reviews"];

  mongocxx::pipeline pipe;
  pipe.match(make_document(kvp("bookId", bookId)));
  pipe.group(make_document(
      kvp("_id", bsoncxx::types::b_null{}),
      kvp("avgRating", make_document(kvp("$avg", "$rating"))),
      kvp("count", make_document(kvp("$sum", 1)))));

  auto cursor = collection.aggregate(pipe);
  for (const auto &doc : cursor) {
    double avg = 0.0;
    int cnt = 0;
    if (doc["avgRating"]) {
      if (doc["avgRating"].type() == bsoncxx::type::k_double) {
        avg = doc["avgRating"].get_double().value;
      } else if (doc["avgRating"].type() == bsoncxx::type::k_int32) {
        avg = static_cast<double>(doc["avgRating"].get_int32().value);
      }
    }
    if (doc["count"]) {
      if (doc["count"].type() == bsoncxx::type::k_int32) {
        cnt = doc["count"].get_int32().value;
      } else if (doc["count"].type() == bsoncxx::type::k_int64) {
        cnt = static_cast<int>(doc["count"].get_int64().value);
      }
    }

    // Round avg to 1 decimal place
    avg = std::round(avg * 10.0) / 10.0;
    return {avg, cnt};
  }

  return {0.0, 0};
}
