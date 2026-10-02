#include "controllers/BookController.h"

#include "models/Book.h"
#include "repositories/BookRepository.h"
#include "repositories/UserRepository.h"
#include "security/TokenService.hpp"
#include "services/BookService.h"

#include <drogon/drogon.h>

#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace drogon;

namespace {
HttpResponsePtr jsonError(int status, const std::string &message) {
  Json::Value body;
  body["success"] = false;
  body["message"] = message;

  auto response = HttpResponse::newHttpJsonResponse(body);
  response->setStatusCode(static_cast<HttpStatusCode>(status));
  return response;
}

std::string toIsoString(std::chrono::system_clock::time_point tp) {
  const auto time = std::chrono::system_clock::to_time_t(tp);
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &time);
#else
  gmtime_r(&time, &tm);
#endif
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buf);
}

Json::Value bookJson(const Book &book) {
  Json::Value result;
  result["id"] = book.id;
  result["title"] = book.title;
  result["author"] = book.author;
  result["isbn"] = book.isbn;
  result["description"] = book.description;
  result["category"] = book.category;
  result["price"] = book.price;
  result["condition"] = book.condition;
  result["coverImage"] = book.coverImage;
  result["status"] = book.status;
  result["averageRating"] = book.averageRating;
  result["reviewCount"] = book.reviewCount;
  result["createdAt"] = toIsoString(book.createdAt);
  result["updatedAt"] = toIsoString(book.updatedAt);

  if (book.owner) {
    Json::Value owner;
    owner["id"] = book.owner->id;
    owner["username"] = book.owner->username;
    result["owner"] = owner;
  } else {
    Json::Value owner;
    owner["id"] = book.ownerId;
    result["owner"] = owner;
  }

  return result;
}

bool isValidObjectId(const std::string &id) {
  if (id.size() != 24)
    return false;
  for (char c : id) {
    if (!std::isxdigit(static_cast<unsigned char>(c)))
      return false;
  }
  return true;
}

std::optional<double> parseDoubleParam(const std::string &val) {
  if (val.empty())
    return std::nullopt;
  try {
    return std::stod(val);
  } catch (...) {
    return std::nullopt;
  }
}

int parseIntParam(const std::string &val, int defaultValue) {
  if (val.empty())
    return defaultValue;
  try {
    return std::stoi(val);
  } catch (...) {
    return defaultValue;
  }
}

std::optional<std::string> extractUserIdFromHeader(const HttpRequestPtr &req) {
  const auto authorization = req->getHeader("Authorization");
  constexpr const char *prefix = "Bearer ";
  if (authorization.size() > 7 && authorization.compare(0, 7, prefix) == 0) {
    const auto token = authorization.substr(7);
    const auto sub = TokenService::verifyAccessToken(token);
    if (!sub.empty()) {
      return sub;
    }
  }
  return std::nullopt;
}
} // namespace

void BookController::createListing(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  auto json = req->getJsonObject();
  if (!json) {
    callback(jsonError(k400BadRequest, "Invalid JSON body"));
    return;
  }

  if (!json->isMember("title") || !json->isMember("author") ||
      !json->isMember("price") || !json->isMember("condition") ||
      !json->isMember("category")) {
    callback(jsonError(
        k400BadRequest,
        "title, author, price, condition, and category are required"));
    return;
  }

  if (!(*json)["title"].isString() || !(*json)["author"].isString() ||
      !(*json)["condition"].isString() || !(*json)["category"].isString()) {
    callback(jsonError(k400BadRequest, "Invalid field data types"));
    return;
  }

  if (!(*json)["price"].isNumeric()) {
    callback(jsonError(k400BadRequest, "Price must be a valid number"));
    return;
  }

  Book book;
  book.title = (*json)["title"].asString();
  book.author = (*json)["author"].asString();
  book.price = (*json)["price"].asDouble();
  book.condition = (*json)["condition"].asString();
  book.category = (*json)["category"].asString();

  if (json->isMember("isbn") && (*json)["isbn"].isString()) {
    book.isbn = (*json)["isbn"].asString();
  }
  if (json->isMember("description") && (*json)["description"].isString()) {
    book.description = (*json)["description"].asString();
  }
  if (json->isMember("coverImage") && (*json)["coverImage"].isString()) {
    book.coverImage = (*json)["coverImage"].asString();
  }

  try {
    BookRepository books;
    UserRepository users;
    BookService service(books, users);

    const auto created = service.createListing(userId, std::move(book));

    Json::Value body;
    body["success"] = true;
    body["message"] = "Book listing created successfully";
    body["book"] = bookJson(created);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k201Created);
    callback(response);
  } catch (const std::invalid_argument &e) {
    callback(jsonError(k400BadRequest, e.what()));
  } catch (const std::exception &e) {
    LOG_ERROR << "Create listing failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void BookController::browse(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  BookFilter filter;

  const auto search = req->getParameter("search");
  if (!search.empty()) {
    filter.search = search;
  }

  const auto category = req->getParameter("category");
  if (!category.empty()) {
    filter.category = category;
  }

  const auto condition = req->getParameter("condition");
  if (!condition.empty()) {
    filter.condition = condition;
  }

  filter.minPrice = parseDoubleParam(req->getParameter("minPrice"));
  filter.maxPrice = parseDoubleParam(req->getParameter("maxPrice"));

  const auto sort = req->getParameter("sort");
  if (!sort.empty()) {
    filter.sort = sort;
  }

  Pagination pagination;
  pagination.page = parseIntParam(req->getParameter("page"), 1);
  pagination.limit = parseIntParam(req->getParameter("limit"), 10);

  try {
    BookRepository books;
    UserRepository users;
    BookService service(books, users);

    const auto pageResult = service.browseBooks(filter, pagination);

    Json::Value body;
    body["success"] = true;

    Json::Value items(Json::arrayValue);
    for (const auto &b : pageResult.items) {
      items.append(bookJson(b));
    }
    body["books"] = items;

    Json::Value meta;
    meta["total"] = static_cast<Json::Int64>(pageResult.total);
    meta["page"] = pageResult.page;
    meta["limit"] = pageResult.limit;
    meta["totalPages"] = pageResult.totalPages;
    meta["hasNextPage"] = pageResult.hasNextPage;
    meta["hasPrevPage"] = pageResult.hasPrevPage;
    body["pagination"] = meta;

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << "Browse books failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void BookController::getMyListings(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  std::optional<std::string> status;
  const auto statusParam = req->getParameter("status");
  if (!statusParam.empty()) {
    status = statusParam;
  }

  Pagination pagination;
  pagination.page = parseIntParam(req->getParameter("page"), 1);
  pagination.limit = parseIntParam(req->getParameter("limit"), 10);

  try {
    BookRepository books;
    UserRepository users;
    BookService service(books, users);

    const auto pageResult = service.getMyListings(userId, status, pagination);

    Json::Value body;
    body["success"] = true;

    Json::Value items(Json::arrayValue);
    for (const auto &b : pageResult.items) {
      items.append(bookJson(b));
    }
    body["books"] = items;

    Json::Value meta;
    meta["total"] = static_cast<Json::Int64>(pageResult.total);
    meta["page"] = pageResult.page;
    meta["limit"] = pageResult.limit;
    meta["totalPages"] = pageResult.totalPages;
    meta["hasNextPage"] = pageResult.hasNextPage;
    meta["hasPrevPage"] = pageResult.hasPrevPage;
    body["pagination"] = meta;

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << "Get my listings failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void BookController::getById(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string bookId) {
  if (!isValidObjectId(bookId)) {
    callback(jsonError(k400BadRequest, "Invalid book ID format"));
    return;
  }

  const auto userId = extractUserIdFromHeader(req);

  try {
    BookRepository books;
    UserRepository users;
    BookService service(books, users);

    const auto book = service.getBookForUser(bookId, userId);

    Json::Value body;
    body["success"] = true;
    body["book"] = bookJson(book);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "BOOK_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Book listing not found"));
      return;
    }
    LOG_ERROR << "Get book failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Get book failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void BookController::updateListing(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string bookId) {
  if (!isValidObjectId(bookId)) {
    callback(jsonError(k400BadRequest, "Invalid book ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  auto json = req->getJsonObject();
  if (!json) {
    callback(jsonError(k400BadRequest, "Invalid JSON body"));
    return;
  }

  BookUpdate update;
  if (json->isMember("title")) {
    if (!(*json)["title"].isString()) {
      callback(jsonError(k400BadRequest, "title must be a string"));
      return;
    }
    update.title = (*json)["title"].asString();
  }
  if (json->isMember("author")) {
    if (!(*json)["author"].isString()) {
      callback(jsonError(k400BadRequest, "author must be a string"));
      return;
    }
    update.author = (*json)["author"].asString();
  }
  if (json->isMember("isbn")) {
    if (!(*json)["isbn"].isString()) {
      callback(jsonError(k400BadRequest, "isbn must be a string"));
      return;
    }
    update.isbn = (*json)["isbn"].asString();
  }
  if (json->isMember("description")) {
    if (!(*json)["description"].isString()) {
      callback(jsonError(k400BadRequest, "description must be a string"));
      return;
    }
    update.description = (*json)["description"].asString();
  }
  if (json->isMember("category")) {
    if (!(*json)["category"].isString()) {
      callback(jsonError(k400BadRequest, "category must be a string"));
      return;
    }
    update.category = (*json)["category"].asString();
  }
  if (json->isMember("price")) {
    if (!(*json)["price"].isNumeric()) {
      callback(jsonError(k400BadRequest, "price must be a number"));
      return;
    }
    update.price = (*json)["price"].asDouble();
  }
  if (json->isMember("condition")) {
    if (!(*json)["condition"].isString()) {
      callback(jsonError(k400BadRequest, "condition must be a string"));
      return;
    }
    update.condition = (*json)["condition"].asString();
  }
  if (json->isMember("coverImage")) {
    if (!(*json)["coverImage"].isString()) {
      callback(jsonError(k400BadRequest, "coverImage must be a string"));
      return;
    }
    update.coverImage = (*json)["coverImage"].asString();
  }

  try {
    BookRepository books;
    UserRepository users;
    BookService service(books, users);

    const auto updated = service.updateListing(bookId, userId, update);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Book listing updated successfully";
    body["book"] = bookJson(updated);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::invalid_argument &e) {
    callback(jsonError(k400BadRequest, e.what()));
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "BOOK_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Book listing not found"));
      return;
    }
    if (error == "FORBIDDEN") {
      callback(jsonError(k403Forbidden,
                         "You do not have permission to modify this listing"));
      return;
    }
    if (error == "CANNOT_MODIFY_CLOSED_LISTING") {
      callback(jsonError(k400BadRequest,
                         "Cannot modify a listing that is sold or exchanged"));
      return;
    }
    if (error == "CANNOT_MODIFY_REMOVED_LISTING") {
      callback(jsonError(k400BadRequest,
                         "Cannot modify a removed listing"));
      return;
    }
    LOG_ERROR << "Update listing failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Update listing failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void BookController::deleteListing(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string bookId) {
  if (!isValidObjectId(bookId)) {
    callback(jsonError(k400BadRequest, "Invalid book ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  try {
    BookRepository books;
    UserRepository users;
    BookService service(books, users);

    service.removeListing(bookId, userId);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Book listing removed successfully";

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "BOOK_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Book listing not found"));
      return;
    }
    if (error == "FORBIDDEN") {
      callback(jsonError(k403Forbidden,
                         "You do not have permission to delete this listing"));
      return;
    }
    if (error == "CANNOT_DELETE_CLOSED_LISTING") {
      callback(jsonError(k400BadRequest,
                         "Cannot delete a listing that is sold or exchanged"));
      return;
    }
    LOG_ERROR << "Delete listing failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Delete listing failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}
