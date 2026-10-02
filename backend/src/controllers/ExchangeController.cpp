#include "controllers/ExchangeController.h"

#include "models/Exchange.h"
#include "repositories/BookRepository.h"
#include "repositories/ExchangeRepository.h"
#include "repositories/UserRepository.h"
#include "services/ExchangeService.h"

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

Json::Value snapshotJson(const ExchangeBookSnapshot &snap) {
  Json::Value json;
  json["bookId"] = snap.bookId;
  json["title"] = snap.title;
  json["author"] = snap.author;
  json["condition"] = snap.condition;
  json["category"] = snap.category;
  json["price"] = snap.price;
  json["coverImage"] = snap.coverImage;
  json["originalOwnerId"] = snap.originalOwnerId;
  json["originalOwnerUsername"] = snap.originalOwnerUsername;
  return json;
}

Json::Value exchangeJson(const ExchangeRequest &req) {
  Json::Value json;
  json["id"] = req.id;
  json["exchangeNumber"] = req.exchangeNumber;
  json["requesterId"] = req.requesterId;
  json["requesterUsername"] = req.requesterUsername;
  json["receiverId"] = req.receiverId;
  json["receiverUsername"] = req.receiverUsername;
  json["status"] = req.status;
  json["message"] = req.message;
  json["statusNote"] = req.statusNote;
  json["requestedBook"] = snapshotJson(req.requestedBook);
  json["offeredBook"] = snapshotJson(req.offeredBook);
  json["createdAt"] = toIsoString(req.createdAt);
  json["updatedAt"] = toIsoString(req.updatedAt);
  if (req.completedAt.has_value()) {
    json["completedAt"] = toIsoString(*req.completedAt);
  }
  return json;
}
} // namespace

void ExchangeController::createRequest(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  auto json = req->getJsonObject();
  if (!json) {
    callback(jsonError(k400BadRequest, "Invalid JSON body"));
    return;
  }

  if (!json->isMember("requestedBookId") ||
      !(*json)["requestedBookId"].isString() ||
      !json->isMember("offeredBookId") ||
      !(*json)["offeredBookId"].isString()) {
    callback(jsonError(
        k400BadRequest,
        "requestedBookId and offeredBookId are required strings"));
    return;
  }

  const auto requestedBookId = (*json)["requestedBookId"].asString();
  const auto offeredBookId = (*json)["offeredBookId"].asString();

  if (!isValidObjectId(requestedBookId) || !isValidObjectId(offeredBookId)) {
    callback(jsonError(k400BadRequest, "Invalid book ID format"));
    return;
  }

  std::string message;
  if (json->isMember("message") && (*json)["message"].isString()) {
    message = (*json)["message"].asString();
  }

  try {
    ExchangeRepository exchanges;
    BookRepository books;
    UserRepository users;
    ExchangeService service(exchanges, books, users);

    const auto created =
        service.createRequest(userId, requestedBookId, offeredBookId, message);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Exchange request created successfully";
    body["exchange"] = exchangeJson(created);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k201Created);
    callback(response);
  } catch (const std::invalid_argument &e) {
    callback(jsonError(k400BadRequest, e.what()));
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "REQUESTED_BOOK_NOT_FOUND" ||
        error == "OFFERED_BOOK_NOT_FOUND") {
      callback(jsonError(k404NotFound, error));
      return;
    }
    if (error == "REQUESTED_BOOK_NOT_AVAILABLE" ||
        error == "OFFERED_BOOK_NOT_AVAILABLE") {
      callback(jsonError(k400BadRequest, error));
      return;
    }
    LOG_ERROR << "Create exchange failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Create exchange failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ExchangeController::getSentRequests(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  const int page = parseIntParam(req->getParameter("page"), 1);
  const int limit = parseIntParam(req->getParameter("limit"), 10);

  try {
    ExchangeRepository exchanges;
    BookRepository books;
    UserRepository users;
    ExchangeService service(exchanges, books, users);

    const auto pageResult = service.getSentRequests(userId, page, limit);

    Json::Value body;
    body["success"] = true;

    Json::Value items(Json::arrayValue);
    for (const auto &ex : pageResult.items) {
      items.append(exchangeJson(ex));
    }
    body["exchanges"] = items;

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
    LOG_ERROR << "Get sent exchanges failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ExchangeController::getReceivedRequests(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  const int page = parseIntParam(req->getParameter("page"), 1);
  const int limit = parseIntParam(req->getParameter("limit"), 10);

  try {
    ExchangeRepository exchanges;
    BookRepository books;
    UserRepository users;
    ExchangeService service(exchanges, books, users);

    const auto pageResult = service.getReceivedRequests(userId, page, limit);

    Json::Value body;
    body["success"] = true;

    Json::Value items(Json::arrayValue);
    for (const auto &ex : pageResult.items) {
      items.append(exchangeJson(ex));
    }
    body["exchanges"] = items;

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
    LOG_ERROR << "Get received exchanges failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ExchangeController::getUserHistory(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  const int page = parseIntParam(req->getParameter("page"), 1);
  const int limit = parseIntParam(req->getParameter("limit"), 10);

  try {
    ExchangeRepository exchanges;
    BookRepository books;
    UserRepository users;
    ExchangeService service(exchanges, books, users);

    const auto pageResult = service.getUserHistory(userId, page, limit);

    Json::Value body;
    body["success"] = true;

    Json::Value items(Json::arrayValue);
    for (const auto &ex : pageResult.items) {
      items.append(exchangeJson(ex));
    }
    body["exchanges"] = items;

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
    LOG_ERROR << "Get exchange history failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ExchangeController::getRequestDetails(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string requestId) {
  if (!isValidObjectId(requestId)) {
    callback(jsonError(k400BadRequest, "Invalid exchange ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  try {
    ExchangeRepository exchanges;
    BookRepository books;
    UserRepository users;
    ExchangeService service(exchanges, books, users);

    const auto ex = service.getRequestDetails(requestId, userId);

    Json::Value body;
    body["success"] = true;
    body["exchange"] = exchangeJson(ex);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "EXCHANGE_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Exchange request not found"));
      return;
    }
    if (error == "FORBIDDEN") {
      callback(jsonError(
          k403Forbidden,
          "You are not authorized to view this exchange request"));
      return;
    }
    LOG_ERROR << "Get exchange details failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Get exchange details failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ExchangeController::acceptExchange(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string requestId) {
  if (!isValidObjectId(requestId)) {
    callback(jsonError(k400BadRequest, "Invalid exchange ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  try {
    ExchangeRepository exchanges;
    BookRepository books;
    UserRepository users;
    ExchangeService service(exchanges, books, users);

    const auto updated = service.acceptExchange(requestId, userId);

    Json::Value body;
    body["success"] = true;
    body["message"] =
        "Exchange accepted and physical book ownership successfully swapped";
    body["exchange"] = exchangeJson(updated);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "EXCHANGE_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Exchange request not found"));
      return;
    }
    if (error.rfind("FORBIDDEN", 0) == 0) {
      callback(jsonError(k403Forbidden, error));
      return;
    }
    if (error.rfind("EXCHANGE_NOT_PENDING", 0) == 0) {
      callback(jsonError(k400BadRequest, error));
      return;
    }
    if (error.rfind("EXCHANGE_ACQUISITION_FAILED", 0) == 0) {
      callback(jsonError(k409Conflict, error));
      return;
    }
    LOG_ERROR << "Accept exchange failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Accept exchange failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ExchangeController::rejectExchange(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string requestId) {
  if (!isValidObjectId(requestId)) {
    callback(jsonError(k400BadRequest, "Invalid exchange ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  std::string note;
  auto json = req->getJsonObject();
  if (json && json->isMember("note") && (*json)["note"].isString()) {
    note = (*json)["note"].asString();
  }

  try {
    ExchangeRepository exchanges;
    BookRepository books;
    UserRepository users;
    ExchangeService service(exchanges, books, users);

    const auto updated = service.rejectExchange(requestId, userId, note);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Exchange request rejected";
    body["exchange"] = exchangeJson(updated);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "EXCHANGE_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Exchange request not found"));
      return;
    }
    if (error.rfind("FORBIDDEN", 0) == 0) {
      callback(jsonError(k403Forbidden, error));
      return;
    }
    if (error.rfind("EXCHANGE_NOT_PENDING", 0) == 0) {
      callback(jsonError(k400BadRequest, error));
      return;
    }
    LOG_ERROR << "Reject exchange failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Reject exchange failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void ExchangeController::cancelExchange(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string requestId) {
  if (!isValidObjectId(requestId)) {
    callback(jsonError(k400BadRequest, "Invalid exchange ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  std::string note;
  auto json = req->getJsonObject();
  if (json && json->isMember("note") && (*json)["note"].isString()) {
    note = (*json)["note"].asString();
  }

  try {
    ExchangeRepository exchanges;
    BookRepository books;
    UserRepository users;
    ExchangeService service(exchanges, books, users);

    const auto updated = service.cancelExchange(requestId, userId, note);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Exchange request cancelled";
    body["exchange"] = exchangeJson(updated);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "EXCHANGE_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Exchange request not found"));
      return;
    }
    if (error.rfind("FORBIDDEN", 0) == 0) {
      callback(jsonError(k403Forbidden, error));
      return;
    }
    if (error.rfind("EXCHANGE_NOT_PENDING", 0) == 0) {
      callback(jsonError(k400BadRequest, error));
      return;
    }
    LOG_ERROR << "Cancel exchange failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Cancel exchange failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}
