#include "controllers/ReviewController.h"

#include "models/Review.h"
#include "repositories/BookRepository.h"
#include "repositories/OrderRepository.h"
#include "repositories/ReviewRepository.h"
#include "repositories/UserRepository.h"
#include "services/ReviewService.h"

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

bool isValidObjectId(const std::string &id) {
  if (id.size() != 24)
    return false;
  for (char c : id) {
    if (!std::isxdigit(static_cast<unsigned char>(c)))
      return false;
  }
  return true;
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

Json::Value reviewJson(const Review &rev) {
  Json::Value json;
  json["id"] = rev.id;
  json["bookId"] = rev.bookId;
  json["orderId"] = rev.orderId;
  json["sellerId"] = rev.sellerId;
  json["sellerUsername"] = rev.sellerUsername;
  json["reviewerId"] = rev.reviewerId;
  json["reviewerUsername"] = rev.reviewerUsername;
  json["rating"] = rev.rating;
  json["comment"] = rev.comment;

  Json::Value images(Json::arrayValue);
  for (const auto &img : rev.images) {
    images.append(img);
  }
  json["images"] = images;

  json["createdAt"] = toIsoString(rev.createdAt);
  json["updatedAt"] = toIsoString(rev.updatedAt);
  return json;
}
} // namespace

void ReviewController::createReview(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  auto json = req->getJsonObject();
  if (!json) {
    callback(jsonError(k400BadRequest, "Invalid JSON body"));
    return;
  }

  if (!json->isMember("orderId") || !(*json)["orderId"].isString() ||
      !json->isMember("bookId") || !(*json)["bookId"].isString() ||
      !json->isMember("rating") || !(*json)["rating"].isInt() ||
      !json->isMember("comment") || !(*json)["comment"].isString()) {
    callback(jsonError(
        k400BadRequest,
        "orderId (string), bookId (string), rating (1-5 int), and comment (string) are required"));
    return;
  }

  const auto orderId = (*json)["orderId"].asString();
  const auto bookId = (*json)["bookId"].asString();
  const int rating = (*json)["rating"].asInt();
  const auto comment = (*json)["comment"].asString();

  std::vector<std::string> images;
  if (json->isMember("images") && (*json)["images"].isArray()) {
    for (const auto &val : (*json)["images"]) {
      if (val.isString()) {
        images.push_back(val.asString());
      }
    }
  }

  if (!isValidObjectId(orderId) || !isValidObjectId(bookId)) {
    callback(jsonError(k400BadRequest, "Invalid orderId or bookId format"));
    return;
  }

  try {
    ReviewRepository reviews;
    OrderRepository orders;
    BookRepository books;
    UserRepository users;
    ReviewService service(reviews, orders, books, users);

    const auto review =
        service.createReview(userId, orderId, bookId, rating, comment, images);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Review submitted successfully";
    body["review"] = reviewJson(review);


    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k201Created);
    callback(response);
  } catch (const std::invalid_argument &e) {
    callback(jsonError(k400BadRequest, e.what()));
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "ORDER_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Order not found"));
      return;
    }
    if (error.rfind("FORBIDDEN", 0) == 0) {
      callback(jsonError(k403Forbidden, error));
      return;
    }
    if (error.rfind("DUPLICATE_REVIEW", 0) == 0) {
      callback(jsonError(k400BadRequest, error));
      return;
    }
    LOG_ERROR << "Create review failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Create review failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ReviewController::getBookReviews(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string bookId) {
  if (!isValidObjectId(bookId)) {
    callback(jsonError(k400BadRequest, "Invalid book ID format"));
    return;
  }

  const int page = parseIntParam(req->getParameter("page"), 1);
  const int limit = parseIntParam(req->getParameter("limit"), 10);

  try {
    ReviewRepository reviews;
    OrderRepository orders;
    BookRepository books;
    UserRepository users;
    ReviewService service(reviews, orders, books, users);

    const auto pageResult = service.getBookReviews(bookId, page, limit);

    Json::Value body;
    body["success"] = true;

    Json::Value items(Json::arrayValue);
    for (const auto &r : pageResult.items) {
      items.append(reviewJson(r));
    }
    body["reviews"] = items;

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
    LOG_ERROR << "Get book reviews failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ReviewController::getSellerReviews(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string sellerId) {
  if (!isValidObjectId(sellerId)) {
    callback(jsonError(k400BadRequest, "Invalid sellerId format"));
    return;
  }

  int page = parseIntParam(req->getParameter("page"), 1);
  int limit = parseIntParam(req->getParameter("limit"), 10);

  try {
    ReviewRepository reviews;
    OrderRepository orders;
    BookRepository books;
    UserRepository users;
    ReviewService service(reviews, orders, books, users);

    const auto pageResult = service.getSellerReviews(sellerId, page, limit);
    const auto stats = service.getSellerRating(sellerId);

    Json::Value body;
    body["success"] = true;
    body["sellerId"] = sellerId;
    body["averageRating"] = stats.first;
    body["reviewCount"] = stats.second;

    Json::Value items(Json::arrayValue);
    for (const auto &r : pageResult.items) {
      items.append(reviewJson(r));
    }
    body["reviews"] = items;

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
    LOG_ERROR << "Get seller reviews failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ReviewController::getMyReviews(

    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  const int page = parseIntParam(req->getParameter("page"), 1);
  const int limit = parseIntParam(req->getParameter("limit"), 10);

  try {
    ReviewRepository reviews;
    OrderRepository orders;
    BookRepository books;
    UserRepository users;
    ReviewService service(reviews, orders, books, users);

    const auto pageResult = service.getMyReviews(userId, page, limit);

    Json::Value body;
    body["success"] = true;

    Json::Value items(Json::arrayValue);
    for (const auto &r : pageResult.items) {
      items.append(reviewJson(r));
    }
    body["reviews"] = items;

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
    LOG_ERROR << "Get my reviews failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ReviewController::updateReview(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string reviewId) {
  if (!isValidObjectId(reviewId)) {
    callback(jsonError(k400BadRequest, "Invalid review ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  auto json = req->getJsonObject();
  if (!json || !json->isMember("rating") || !(*json)["rating"].isInt() ||
      !json->isMember("comment") || !(*json)["comment"].isString()) {
    callback(jsonError(k400BadRequest,
                       "rating (1-5 int) and comment (string) are required"));
    return;
  }

  const int rating = (*json)["rating"].asInt();
  const auto comment = (*json)["comment"].asString();

  try {
    ReviewRepository reviews;
    OrderRepository orders;
    BookRepository books;
    UserRepository users;
    ReviewService service(reviews, orders, books, users);

    const auto updated = service.updateReview(reviewId, userId, rating, comment);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Review updated successfully";
    body["review"] = reviewJson(updated);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::invalid_argument &e) {
    callback(jsonError(k400BadRequest, e.what()));
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "REVIEW_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Review not found"));
      return;
    }
    if (error.rfind("FORBIDDEN", 0) == 0) {
      callback(jsonError(k403Forbidden, error));
      return;
    }
    LOG_ERROR << "Update review failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Update review failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ReviewController::deleteReview(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string reviewId) {
  if (!isValidObjectId(reviewId)) {
    callback(jsonError(k400BadRequest, "Invalid review ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  try {
    ReviewRepository reviews;
    OrderRepository orders;
    BookRepository books;
    UserRepository users;
    ReviewService service(reviews, orders, books, users);

    service.deleteReview(reviewId, userId);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Review deleted successfully";

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "REVIEW_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Review not found"));
      return;
    }
    if (error.rfind("FORBIDDEN", 0) == 0) {
      callback(jsonError(k403Forbidden, error));
      return;
    }
    LOG_ERROR << "Delete review failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Delete review failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}
