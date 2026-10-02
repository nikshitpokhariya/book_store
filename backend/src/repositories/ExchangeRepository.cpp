#include "repositories/ExchangeRepository.h"

#include "db/MongoDatabase.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/options/find.hpp>

#include <cmath>

using bsoncxx::builder::basic::array;
using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace {
std::string getString(const bsoncxx::document::view &view, const char *key) {
  if (view[key] && view[key].type() == bsoncxx::type::k_string) {
    return std::string(view[key].get_string().value.data(),
                       view[key].get_string().value.size());
  }
  return "";
}

double getDouble(const bsoncxx::document::view &view, const char *key) {
  if (!view[key])
    return 0.0;
  if (view[key].type() == bsoncxx::type::k_double) {
    return view[key].get_double().value;
  }
  if (view[key].type() == bsoncxx::type::k_int64) {
    return static_cast<double>(view[key].get_int64().value);
  }
  if (view[key].type() == bsoncxx::type::k_int32) {
    return static_cast<double>(view[key].get_int32().value);
  }
  return 0.0;
}

ExchangeBookSnapshot toSnapshot(const bsoncxx::document::view &doc) {
  ExchangeBookSnapshot snap;
  snap.bookId = getString(doc, "bookId");
  snap.title = getString(doc, "title");
  snap.author = getString(doc, "author");
  snap.condition = getString(doc, "condition");
  snap.category = getString(doc, "category");
  snap.price = getDouble(doc, "price");
  snap.coverImage = getString(doc, "coverImage");
  snap.originalOwnerId = getString(doc, "originalOwnerId");
  snap.originalOwnerUsername = getString(doc, "originalOwnerUsername");
  return snap;
}

document snapshotToDoc(const ExchangeBookSnapshot &snap) {
  document doc;
  doc.append(kvp("bookId", snap.bookId));
  doc.append(kvp("title", snap.title));
  doc.append(kvp("author", snap.author));
  doc.append(kvp("condition", snap.condition));
  doc.append(kvp("category", snap.category));
  doc.append(kvp("price", snap.price));
  doc.append(kvp("coverImage", snap.coverImage));
  doc.append(kvp("originalOwnerId", snap.originalOwnerId));
  doc.append(kvp("originalOwnerUsername", snap.originalOwnerUsername));
  return doc;
}

ExchangeRequest toExchangeRequest(const bsoncxx::document::view &view) {
  ExchangeRequest req;
  req.id = view["_id"].get_oid().value.to_string();
  req.exchangeNumber = getString(view, "exchangeNumber");
  req.requesterId = getString(view, "requesterId");
  req.requesterUsername = getString(view, "requesterUsername");
  req.receiverId = getString(view, "receiverId");
  req.receiverUsername = getString(view, "receiverUsername");
  req.status = getString(view, "status");
  req.message = getString(view, "message");
  req.statusNote = getString(view, "statusNote");

  if (view["requestedBook"] &&
      view["requestedBook"].type() == bsoncxx::type::k_document) {
    req.requestedBook = toSnapshot(view["requestedBook"].get_document().value);
  }
  if (view["offeredBook"] &&
      view["offeredBook"].type() == bsoncxx::type::k_document) {
    req.offeredBook = toSnapshot(view["offeredBook"].get_document().value);
  }

  if (view["createdAt"] && view["createdAt"].type() == bsoncxx::type::k_date) {
    req.createdAt = std::chrono::system_clock::time_point{
        view["createdAt"].get_date().value};
  }
  if (view["updatedAt"] && view["updatedAt"].type() == bsoncxx::type::k_date) {
    req.updatedAt = std::chrono::system_clock::time_point{
        view["updatedAt"].get_date().value};
  }
  if (view["completedAt"] &&
      view["completedAt"].type() == bsoncxx::type::k_date) {
    req.completedAt = std::chrono::system_clock::time_point{
        view["completedAt"].get_date().value};
  }

  return req;
}
} // namespace

std::string ExchangeRepository::create(const ExchangeRequest &req) {
  auto collection = MongoDatabase::instance().database()["exchanges"];

  const auto now = std::chrono::system_clock::now();

  document doc;
  doc.append(kvp("exchangeNumber", req.exchangeNumber));
  doc.append(kvp("requesterId", req.requesterId));
  doc.append(kvp("requesterUsername", req.requesterUsername));
  doc.append(kvp("receiverId", req.receiverId));
  doc.append(kvp("receiverUsername", req.receiverUsername));
  doc.append(kvp("requestedBook", snapshotToDoc(req.requestedBook).view()));
  doc.append(kvp("offeredBook", snapshotToDoc(req.offeredBook).view()));
  doc.append(kvp("status", req.status.empty() ? "PENDING" : req.status));
  doc.append(kvp("message", req.message));
  doc.append(kvp("statusNote", req.statusNote));
  doc.append(kvp("createdAt", bsoncxx::types::b_date{now}));
  doc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  auto result = collection.insert_one(doc.view());
  if (!result) {
    throw std::runtime_error("EXCHANGE_CREATE_FAILED");
  }

  return result->inserted_id().get_oid().value.to_string();
}

std::optional<ExchangeRequest>
ExchangeRepository::findById(const std::string &id) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{id};
  } catch (const std::exception &) {
    return std::nullopt;
  }

  auto collection = MongoDatabase::instance().database()["exchanges"];

  document filter;
  filter.append(kvp("_id", objectId));

  auto result = collection.find_one(filter.view());
  if (!result) {
    return std::nullopt;
  }

  return toExchangeRequest(result->view());
}

PageResult<ExchangeRequest>
ExchangeRepository::findByRequester(const std::string &requesterId, int page,
                                    int limit) {
  auto collection = MongoDatabase::instance().database()["exchanges"];

  document filter;
  filter.append(kvp("requesterId", requesterId));

  const std::int64_t total = collection.count_documents(filter.view());

  mongocxx::options::find options;
  const auto skipCount = static_cast<std::int64_t>((page - 1) * limit);
  options.skip(skipCount);
  options.limit(static_cast<std::int64_t>(limit));

  document sortDoc;
  sortDoc.append(kvp("createdAt", -1));
  options.sort(sortDoc.view());

  auto cursor = collection.find(filter.view(), options);

  PageResult<ExchangeRequest> result;
  result.total = total;
  result.page = page;
  result.limit = limit;
  result.totalPages =
      limit > 0 ? static_cast<int>(std::ceil(static_cast<double>(total) / limit))
                : 0;
  result.hasNextPage = result.page < result.totalPages;
  result.hasPrevPage = result.page > 1 && result.totalPages > 0;

  for (const auto &docView : cursor) {
    result.items.push_back(toExchangeRequest(docView));
  }

  return result;
}

PageResult<ExchangeRequest>
ExchangeRepository::findByReceiver(const std::string &receiverId, int page,
                                   int limit) {
  auto collection = MongoDatabase::instance().database()["exchanges"];

  document filter;
  filter.append(kvp("receiverId", receiverId));

  const std::int64_t total = collection.count_documents(filter.view());

  mongocxx::options::find options;
  const auto skipCount = static_cast<std::int64_t>((page - 1) * limit);
  options.skip(skipCount);
  options.limit(static_cast<std::int64_t>(limit));

  document sortDoc;
  sortDoc.append(kvp("createdAt", -1));
  options.sort(sortDoc.view());

  auto cursor = collection.find(filter.view(), options);

  PageResult<ExchangeRequest> result;
  result.total = total;
  result.page = page;
  result.limit = limit;
  result.totalPages =
      limit > 0 ? static_cast<int>(std::ceil(static_cast<double>(total) / limit))
                : 0;
  result.hasNextPage = result.page < result.totalPages;
  result.hasPrevPage = result.page > 1 && result.totalPages > 0;

  for (const auto &docView : cursor) {
    result.items.push_back(toExchangeRequest(docView));
  }

  return result;
}

PageResult<ExchangeRequest>
ExchangeRepository::findUserHistory(const std::string &userId, int page,
                                    int limit) {
  auto collection = MongoDatabase::instance().database()["exchanges"];

  array orClauses;
  orClauses.append(make_document(kvp("requesterId", userId)));
  orClauses.append(make_document(kvp("receiverId", userId)));

  document filter;
  filter.append(kvp("$or", orClauses.view()));

  const std::int64_t total = collection.count_documents(filter.view());

  mongocxx::options::find options;
  const auto skipCount = static_cast<std::int64_t>((page - 1) * limit);
  options.skip(skipCount);
  options.limit(static_cast<std::int64_t>(limit));

  document sortDoc;
  sortDoc.append(kvp("createdAt", -1));
  options.sort(sortDoc.view());

  auto cursor = collection.find(filter.view(), options);

  PageResult<ExchangeRequest> result;
  result.total = total;
  result.page = page;
  result.limit = limit;
  result.totalPages =
      limit > 0 ? static_cast<int>(std::ceil(static_cast<double>(total) / limit))
                : 0;
  result.hasNextPage = result.page < result.totalPages;
  result.hasPrevPage = result.page > 1 && result.totalPages > 0;

  for (const auto &docView : cursor) {
    result.items.push_back(toExchangeRequest(docView));
  }

  return result;
}

bool ExchangeRepository::updateStatus(const std::string &id,
                                      const std::string &newStatus,
                                      const std::string &note,
                                      bool setCompletedAt) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{id};
  } catch (const std::exception &) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["exchanges"];

  const auto now = std::chrono::system_clock::now();

  document setFields;
  setFields.append(kvp("status", newStatus));
  setFields.append(kvp("statusNote", note));
  setFields.append(kvp("updatedAt", bsoncxx::types::b_date{now}));
  if (setCompletedAt) {
    setFields.append(kvp("completedAt", bsoncxx::types::b_date{now}));
  }

  document updateDoc;
  updateDoc.append(kvp("$set", setFields.view()));

  document filter;
  filter.append(kvp("_id", objectId));

  auto result = collection.update_one(filter.view(), updateDoc.view());
  return result && result->modified_count() > 0;
}
