#include "repositories/BookRepository.h"

#include "db/MongoDatabase.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/options/find.hpp>

#include <cmath>
#include <stdexcept>
#include <unordered_set>

using bsoncxx::builder::basic::array;
using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace {
std::string escapeRegex(const std::string &input) {
  std::string escaped;
  escaped.reserve(input.size() * 2);
  for (char c : input) {
    if (c == '.' || c == '^' || c == '$' || c == '*' || c == '+' || c == '?' ||
        c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
        c == '|' || c == '\\') {
      escaped += '\\';
    }
    escaped += c;
  }
  return escaped;
}

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

Book toBook(const bsoncxx::document::view &view) {
  Book book;

  book.id = view["_id"].get_oid().value.to_string();
  book.ownerId = getString(view, "ownerId");
  book.title = getString(view, "title");
  book.author = getString(view, "author");
  book.isbn = getString(view, "isbn");
  book.description = getString(view, "description");
  book.category = getString(view, "category");
  book.price = getDouble(view, "price");
  book.condition = getString(view, "condition");
  book.coverImage = getString(view, "coverImage");
  if (view["images"] && view["images"].type() == bsoncxx::type::k_array) {
    for (const auto &item : view["images"].get_array().value) {
      if (item.type() == bsoncxx::type::k_string) {
        book.images.emplace_back(item.get_string().value.data(),
                                 item.get_string().value.size());
      }
    }
  }
  if (book.images.empty() && !book.coverImage.empty()) {
    book.images.push_back(book.coverImage);
  }
  book.videoUrl = getString(view, "videoUrl");
  book.status = getString(view, "status");
  if (book.status.empty()) {
    book.status = "available";
  }

  if (view["createdAt"] && view["createdAt"].type() == bsoncxx::type::k_date) {
    book.createdAt = std::chrono::system_clock::time_point{
        view["createdAt"].get_date().value};
  }
  if (view["updatedAt"] && view["updatedAt"].type() == bsoncxx::type::k_date) {
    book.updatedAt = std::chrono::system_clock::time_point{
        view["updatedAt"].get_date().value};
  }

  book.averageRating = getDouble(view, "averageRating");
  if (view["reviewCount"] &&
      view["reviewCount"].type() == bsoncxx::type::k_int32) {
    book.reviewCount = view["reviewCount"].get_int32().value;
  } else if (view["reviewCount"] &&
             view["reviewCount"].type() == bsoncxx::type::k_int64) {
    book.reviewCount =
        static_cast<int>(view["reviewCount"].get_int64().value);
  }

  return book;
}
} // namespace

std::string BookRepository::create(const Book &book) {
  auto collection = MongoDatabase::instance().database()["books"];

  const auto now = std::chrono::system_clock::now();

  document doc;
  doc.append(kvp("ownerId", book.ownerId));
  doc.append(kvp("title", book.title));
  doc.append(kvp("author", book.author));
  doc.append(kvp("isbn", book.isbn));
  doc.append(kvp("description", book.description));
  doc.append(kvp("category", book.category));
  doc.append(kvp("price", book.price));
  doc.append(kvp("condition", book.condition));
  doc.append(kvp("coverImage", book.coverImage));
  array imagesArr;
  for (const auto &img : book.images) {
    imagesArr.append(img);
  }
  doc.append(kvp("images", imagesArr.view()));
  doc.append(kvp("videoUrl", book.videoUrl));
  doc.append(kvp("status", book.status.empty() ? "available" : book.status));
  doc.append(kvp("averageRating", book.averageRating));
  doc.append(kvp("reviewCount", book.reviewCount));
  doc.append(kvp("createdAt", bsoncxx::types::b_date{now}));
  doc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  auto result = collection.insert_one(doc.view());
  if (!result) {
    throw std::runtime_error("BOOK_CREATE_FAILED");
  }

  return result->inserted_id().get_oid().value.to_string();
}

std::optional<Book> BookRepository::findById(const std::string &id) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{id};
  } catch (const std::exception &) {
    return std::nullopt;
  }

  auto collection = MongoDatabase::instance().database()["books"];

  document filter;
  filter.append(kvp("_id", objectId));
  auto result = collection.find_one(filter.view());

  if (!result) {
    return std::nullopt;
  }

  return toBook(result->view());
}

PageResult<Book> BookRepository::findPublic(const BookFilter &filter,
                                           const Pagination &pagination) {
  auto collection = MongoDatabase::instance().database()["books"];

  document filterDoc;
  // Public browse/search only returns available books
  filterDoc.append(kvp("status", "available"));

  if (filter.category && !filter.category->empty()) {
    filterDoc.append(kvp("category", *filter.category));
  }

  if (filter.condition && !filter.condition->empty()) {
    filterDoc.append(kvp("condition", *filter.condition));
  }

  if (filter.minPrice.has_value() || filter.maxPrice.has_value()) {
    document priceFilter;
    if (filter.minPrice.has_value()) {
      priceFilter.append(kvp("$gte", *filter.minPrice));
    }
    if (filter.maxPrice.has_value()) {
      priceFilter.append(kvp("$lte", *filter.maxPrice));
    }
    filterDoc.append(kvp("price", priceFilter.view()));
  }

  if (filter.search && !filter.search->empty()) {
    const auto escaped = escapeRegex(*filter.search);
    bsoncxx::types::b_regex searchRegex{escaped, "i"};

    array orClauses;
    orClauses.append(make_document(kvp("title", searchRegex)));
    orClauses.append(make_document(kvp("author", searchRegex)));
    orClauses.append(make_document(kvp("isbn", searchRegex)));
    orClauses.append(make_document(kvp("category", searchRegex)));

    filterDoc.append(kvp("$or", orClauses.view()));
  }

  const std::int64_t total = collection.count_documents(filterDoc.view());

  mongocxx::options::find options;
  const auto skipCount =
      static_cast<std::int64_t>((pagination.page - 1) * pagination.limit);
  options.skip(skipCount);
  options.limit(static_cast<std::int64_t>(pagination.limit));

  document sortDoc;
  if (filter.sort == "price_asc") {
    sortDoc.append(kvp("price", 1));
  } else if (filter.sort == "price_desc") {
    sortDoc.append(kvp("price", -1));
  } else if (filter.sort == "oldest") {
    sortDoc.append(kvp("createdAt", 1));
  } else {
    // Default newest
    sortDoc.append(kvp("createdAt", -1));
  }
  options.sort(sortDoc.view());

  auto cursor = collection.find(filterDoc.view(), options);

  PageResult<Book> result;
  result.total = total;
  result.page = pagination.page;
  result.limit = pagination.limit;
  result.totalPages =
      pagination.limit > 0
          ? static_cast<int>(
                std::ceil(static_cast<double>(total) / pagination.limit))
          : 0;
  result.hasNextPage = result.page < result.totalPages;
  result.hasPrevPage = result.page > 1 && result.totalPages > 0;

  for (const auto &docView : cursor) {
    result.items.push_back(toBook(docView));
  }

  return result;
}

PageResult<Book>
BookRepository::findByOwner(const std::string &ownerId,
                            const std::optional<std::string> &status,
                            const Pagination &pagination) {
  auto collection = MongoDatabase::instance().database()["books"];

  document filterDoc;
  filterDoc.append(kvp("ownerId", ownerId));

  if (status && !status->empty()) {
    filterDoc.append(kvp("status", *status));
  }

  const std::int64_t total = collection.count_documents(filterDoc.view());

  mongocxx::options::find options;
  const auto skipCount =
      static_cast<std::int64_t>((pagination.page - 1) * pagination.limit);
  options.skip(skipCount);
  options.limit(static_cast<std::int64_t>(pagination.limit));

  document sortDoc;
  sortDoc.append(kvp("createdAt", -1));
  options.sort(sortDoc.view());

  auto cursor = collection.find(filterDoc.view(), options);

  PageResult<Book> result;
  result.total = total;
  result.page = pagination.page;
  result.limit = pagination.limit;
  result.totalPages =
      pagination.limit > 0
          ? static_cast<int>(
                std::ceil(static_cast<double>(total) / pagination.limit))
          : 0;
  result.hasNextPage = result.page < result.totalPages;
  result.hasPrevPage = result.page > 1 && result.totalPages > 0;

  for (const auto &docView : cursor) {
    result.items.push_back(toBook(docView));
  }

  return result;
}

bool BookRepository::update(const std::string &id, const BookUpdate &update) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{id};
  } catch (const std::exception &) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["books"];

  document setFields;
  if (update.title) {
    setFields.append(kvp("title", *update.title));
  }
  if (update.author) {
    setFields.append(kvp("author", *update.author));
  }
  if (update.isbn) {
    setFields.append(kvp("isbn", *update.isbn));
  }
  if (update.description) {
    setFields.append(kvp("description", *update.description));
  }
  if (update.category) {
    setFields.append(kvp("category", *update.category));
  }
  if (update.price) {
    setFields.append(kvp("price", *update.price));
  }
  if (update.condition) {
    setFields.append(kvp("condition", *update.condition));
  }
  if (update.coverImage) {
    setFields.append(kvp("coverImage", *update.coverImage));
  }
  if (update.images) {
    array imagesArr;
    for (const auto &img : *update.images) {
      imagesArr.append(img);
    }
    setFields.append(kvp("images", imagesArr.view()));
  }
  if (update.videoUrl) {
    setFields.append(kvp("videoUrl", *update.videoUrl));
  }

  setFields.append(
      kvp("updatedAt",
          bsoncxx::types::b_date{std::chrono::system_clock::now()}));

  document updateDoc;
  updateDoc.append(kvp("$set", setFields.view()));

  document filter;
  filter.append(kvp("_id", objectId));

  auto result = collection.update_one(filter.view(), updateDoc.view());
  return result && result->modified_count() > 0;
}

bool BookRepository::updateStatus(const std::string &id,
                                  const std::string &status) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{id};
  } catch (const std::exception &) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["books"];

  document setFields;
  setFields.append(kvp("status", status));
  setFields.append(
      kvp("updatedAt",
          bsoncxx::types::b_date{std::chrono::system_clock::now()}));

  document updateDoc;
  updateDoc.append(kvp("$set", setFields.view()));

  document filter;
  filter.append(kvp("_id", objectId));

  auto result = collection.update_one(filter.view(), updateDoc.view());
  return result && result->modified_count() > 0;
}

bool BookRepository::acquireForPurchase(const std::string &bookId) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{bookId};
  } catch (const std::exception &) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["books"];

  document filter;
  filter.append(kvp("_id", objectId));
  filter.append(kvp("status", "available"));

  document setFields;
  setFields.append(kvp("status", "sold"));
  setFields.append(
      kvp("updatedAt",
          bsoncxx::types::b_date{std::chrono::system_clock::now()}));

  document updateDoc;
  updateDoc.append(kvp("$set", setFields.view()));

  auto result = collection.update_one(filter.view(), updateDoc.view());
  return result && result->modified_count() > 0;
}

bool BookRepository::releasePurchase(const std::string &bookId) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{bookId};
  } catch (const std::exception &) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["books"];

  document filter;
  filter.append(kvp("_id", objectId));
  filter.append(kvp("status", "sold"));

  document setFields;
  setFields.append(kvp("status", "available"));
  setFields.append(
      kvp("updatedAt",
          bsoncxx::types::b_date{std::chrono::system_clock::now()}));

  document updateDoc;
  updateDoc.append(kvp("$set", setFields.view()));

  auto result = collection.update_one(filter.view(), updateDoc.view());
  return result && result->modified_count() > 0;
}

bool BookRepository::exchangeOwnership(const std::string &requestedBookId,
                                       const std::string &receiverId,
                                       const std::string &offeredBookId,
                                       const std::string &requesterId) {
  bsoncxx::oid reqOid, offOid;
  try {
    reqOid = bsoncxx::oid{requestedBookId};
    offOid = bsoncxx::oid{offeredBookId};
  } catch (const std::exception &) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["books"];
  const auto now = std::chrono::system_clock::now();

  // Phase 1: Conditionally acquire requested book (owned by receiver, must be available)
  // Transfer owner to requesterId, status to "exchanged"
  document filterA;
  filterA.append(kvp("_id", reqOid));
  filterA.append(kvp("ownerId", receiverId));
  filterA.append(kvp("status", "available"));

  document setA;
  setA.append(kvp("ownerId", requesterId));
  setA.append(kvp("status", "exchanged"));
  setA.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  auto resA = collection.update_one(filterA.view(),
                                    make_document(kvp("$set", setA.view())));
  if (!resA || resA->modified_count() == 0) {
    return false;
  }

  // Phase 2: Conditionally acquire offered book (owned by requester, must be available)
  // Transfer owner to receiverId, status to "exchanged"
  document filterB;
  filterB.append(kvp("_id", offOid));
  filterB.append(kvp("ownerId", requesterId));
  filterB.append(kvp("status", "available"));

  document setB;
  setB.append(kvp("ownerId", receiverId));
  setB.append(kvp("status", "exchanged"));
  setB.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  auto resB = collection.update_one(filterB.view(),
                                    make_document(kvp("$set", setB.view())));
  if (!resB || resB->modified_count() == 0) {
    // Rollback Phase 1 (restore requested book back to receiver with status "available")
    document rollbackFilter;
    rollbackFilter.append(kvp("_id", reqOid));
    rollbackFilter.append(kvp("ownerId", requesterId));
    rollbackFilter.append(kvp("status", "exchanged"));

    document rollbackSet;
    rollbackSet.append(kvp("ownerId", receiverId));
    rollbackSet.append(kvp("status", "available"));
    rollbackSet.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

    collection.update_one(rollbackFilter.view(),
                          make_document(kvp("$set", rollbackSet.view())));
    return false;
  }

  return true;
}

bool BookRepository::updateRating(const std::string &bookId,
                                  double averageRating, int reviewCount) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{bookId};
  } catch (...) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["books"];
  const auto now = std::chrono::system_clock::now();

  document filter;
  filter.append(kvp("_id", objectId));

  document setDoc;
  setDoc.append(kvp("averageRating", averageRating));
  setDoc.append(kvp("reviewCount", reviewCount));
  setDoc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  auto res = collection.update_one(filter.view(),
                                   make_document(kvp("$set", setDoc.view())));
  return res && res->modified_count() > 0;
}

bool BookRepository::restoreAvailability(const std::string &bookId) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{bookId};
  } catch (...) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["books"];
  const auto now = std::chrono::system_clock::now();

  document filter;
  filter.append(kvp("_id", objectId));

  document setDoc;
  setDoc.append(kvp("status", "available"));
  setDoc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  auto res = collection.update_one(filter.view(),
                                   make_document(kvp("$set", setDoc.view())));
  return res && res->modified_count() > 0;
}

bool BookRepository::markReturned(const std::string &bookId) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{bookId};
  } catch (...) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["books"];
  const auto now = std::chrono::system_clock::now();

  document filter;
  filter.append(kvp("_id", objectId));

  document setDoc;
  setDoc.append(kvp("status", "returned"));
  setDoc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  auto res = collection.update_one(filter.view(),
                                   make_document(kvp("$set", setDoc.view())));
  return res && res->modified_count() > 0;
}

bool BookRepository::existsByOwnerAndTitle(const std::string &ownerId,
                                           const std::string &title) {
  if (ownerId.empty() || title.empty()) {
    return false;
  }
  auto collection = MongoDatabase::instance().database()["books"];
  document filter;
  filter.append(kvp("ownerId", ownerId));
  filter.append(kvp("title", bsoncxx::types::b_regex{"^" + escapeRegex(title) + "$", "i"}));
  filter.append(kvp("status", make_document(kvp("$ne", "deleted"))));
  return collection.count_documents(filter.view()) > 0;
}

std::vector<std::string> BookRepository::findSuggestions(const std::string &query, int limit) {
  std::vector<std::string> suggestions;
  if (query.empty() || limit <= 0) {
    return suggestions;
  }

  auto collection = MongoDatabase::instance().database()["books"];
  document filterDoc;
  filterDoc.append(kvp("status", "available"));

  const auto escaped = escapeRegex(query);
  bsoncxx::types::b_regex searchRegex{escaped, "i"};

  array orClauses;
  orClauses.append(make_document(kvp("title", searchRegex)));
  orClauses.append(make_document(kvp("author", searchRegex)));
  filterDoc.append(kvp("$or", orClauses.view()));

  mongocxx::options::find options;
  options.limit(static_cast<std::int64_t>(limit * 3));

  document sortDoc;
  sortDoc.append(kvp("createdAt", -1));
  options.sort(sortDoc.view());

  auto cursor = collection.find(filterDoc.view(), options);
  std::unordered_set<std::string> seen;

  for (const auto &docView : cursor) {
    std::string title = getString(docView, "title");
    if (!title.empty() && seen.find(title) == seen.end()) {
      seen.insert(title);
      suggestions.push_back(title);
      if (static_cast<int>(suggestions.size()) >= limit) {
        break;
      }
    }
  }
  return suggestions;
}


