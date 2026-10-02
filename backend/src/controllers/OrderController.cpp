#include "controllers/OrderController.h"

#include "models/Order.h"
#include "repositories/BookRepository.h"
#include "repositories/CartRepository.h"
#include "repositories/OrderRepository.h"
#include "repositories/UserRepository.h"
#include "services/OrderService.h"

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

Json::Value orderJson(const Order &order) {
  Json::Value result;
  result["id"] = order.id;
  result["orderNumber"] = order.orderNumber;
  result["buyerId"] = order.buyerId;
  result["buyerUsername"] = order.buyerUsername;
  result["subtotal"] = order.subtotal;
  result["shippingCost"] = order.shippingCost;
  result["total"] = order.total;
  result["status"] = order.status;
  result["createdAt"] = toIsoString(order.createdAt);
  result["updatedAt"] = toIsoString(order.updatedAt);

  // Items
  Json::Value items(Json::arrayValue);
  for (const auto &item : order.items) {
    Json::Value itemJson;
    itemJson["bookId"] = item.bookId;
    itemJson["title"] = item.title;
    itemJson["author"] = item.author;
    itemJson["sellerId"] = item.sellerId;
    itemJson["sellerUsername"] = item.sellerUsername;
    itemJson["price"] = item.price;
    itemJson["condition"] = item.condition;
    itemJson["category"] = item.category;
    itemJson["coverImage"] = item.coverImage;
    itemJson["quantity"] = item.quantity;
    items.append(itemJson);
  }
  result["items"] = items;

  // Shipping Address
  Json::Value addrJson;
  addrJson["fullName"] = order.shippingAddress.fullName;
  addrJson["addressLine"] = order.shippingAddress.addressLine;
  addrJson["city"] = order.shippingAddress.city;
  addrJson["state"] = order.shippingAddress.state;
  addrJson["postalCode"] = order.shippingAddress.postalCode;
  addrJson["phone"] = order.shippingAddress.phone;
  result["shippingAddress"] = addrJson;

  // Payment
  Json::Value payJson;
  payJson["method"] = order.payment.method;
  payJson["status"] = order.payment.status;
  payJson["transactionId"] = order.payment.transactionId;
  payJson["paidAt"] = toIsoString(order.payment.paidAt);
  result["payment"] = payJson;

  // Shipping Info
  Json::Value shipJson;
  shipJson["trackingId"] = order.shipping.trackingId;
  shipJson["status"] = order.shipping.status;
  shipJson["cost"] = order.shipping.cost;
  shipJson["updatedAt"] = toIsoString(order.shipping.updatedAt);
  result["shipping"] = shipJson;

  // History
  Json::Value history(Json::arrayValue);
  for (const auto &h : order.history) {
    Json::Value hJson;
    hJson["status"] = h.status;
    hJson["timestamp"] = toIsoString(h.timestamp);
    hJson["note"] = h.note;
    history.append(hJson);
  }
  result["history"] = history;

  return result;
}
} // namespace

void OrderController::checkout(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  auto json = req->getJsonObject();
  if (!json) {
    callback(jsonError(k400BadRequest, "Invalid JSON body"));
    return;
  }

  if (!json->isMember("paymentMethod") || !(*json)["paymentMethod"].isString()) {
    callback(jsonError(k400BadRequest,
                       "paymentMethod is required (UPI, CARD, COD)"));
    return;
  }

  if (!json->isMember("shippingAddress") ||
      !(*json)["shippingAddress"].isObject()) {
    callback(jsonError(k400BadRequest, "shippingAddress object is required"));
    return;
  }

  const auto addrObj = (*json)["shippingAddress"];
  ShippingAddress address;
  if (addrObj.isMember("fullName") && addrObj["fullName"].isString()) {
    address.fullName = addrObj["fullName"].asString();
  }
  if (addrObj.isMember("addressLine") && addrObj["addressLine"].isString()) {
    address.addressLine = addrObj["addressLine"].asString();
  }
  if (addrObj.isMember("city") && addrObj["city"].isString()) {
    address.city = addrObj["city"].asString();
  }
  if (addrObj.isMember("state") && addrObj["state"].isString()) {
    address.state = addrObj["state"].asString();
  }
  if (addrObj.isMember("postalCode") && addrObj["postalCode"].isString()) {
    address.postalCode = addrObj["postalCode"].asString();
  }
  if (addrObj.isMember("phone") && addrObj["phone"].isString()) {
    address.phone = addrObj["phone"].asString();
  }

  CheckoutRequest checkReq;
  checkReq.paymentMethod = (*json)["paymentMethod"].asString();
  checkReq.shippingAddress = address;
  if (json->isMember("simulatePaymentFailure") &&
      (*json)["simulatePaymentFailure"].isBool()) {
    checkReq.simulatePaymentFailure =
        (*json)["simulatePaymentFailure"].asBool();
  }

  try {
    OrderRepository orders;
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    OrderService service(orders, carts, books, users);

    const auto order = service.checkout(userId, checkReq);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Order placed successfully";
    body["order"] = orderJson(order);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k201Created);
    callback(response);
  } catch (const std::invalid_argument &e) {
    callback(jsonError(k400BadRequest, e.what()));
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "CART_IS_EMPTY") {
      callback(jsonError(k400BadRequest, "Your cart is empty"));
      return;
    }
    if (error == "CANNOT_BUY_OWN_BOOK") {
      callback(jsonError(k400BadRequest,
                         "Cannot checkout your own book listing"));
      return;
    }
    if (error.rfind("STALE_CART_ITEM", 0) == 0) {
      callback(jsonError(k409Conflict, error));
      return;
    }
    if (error.rfind("BOOK_ACQUISITION_FAILED", 0) == 0) {
      callback(jsonError(k409Conflict, error));
      return;
    }
    if (error.rfind("PAYMENT_FAILED", 0) == 0) {
      callback(jsonError(k400BadRequest, error));
      return;
    }
    LOG_ERROR << "Checkout failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Checkout failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void OrderController::getBuyerOrders(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  const int page = parseIntParam(req->getParameter("page"), 1);
  const int limit = parseIntParam(req->getParameter("limit"), 10);

  try {
    OrderRepository orders;
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    OrderService service(orders, carts, books, users);

    const auto pageResult = service.getBuyerOrders(userId, page, limit);

    Json::Value body;
    body["success"] = true;

    Json::Value items(Json::arrayValue);
    for (const auto &o : pageResult.items) {
      items.append(orderJson(o));
    }
    body["orders"] = items;

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
    LOG_ERROR << "Get buyer orders failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void OrderController::getSellerSales(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  const int page = parseIntParam(req->getParameter("page"), 1);
  const int limit = parseIntParam(req->getParameter("limit"), 10);

  try {
    OrderRepository orders;
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    OrderService service(orders, carts, books, users);

    const auto pageResult = service.getSellerSales(userId, page, limit);

    Json::Value body;
    body["success"] = true;

    Json::Value items(Json::arrayValue);
    for (const auto &o : pageResult.items) {
      items.append(orderJson(o));
    }
    body["orders"] = items;

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
    LOG_ERROR << "Get seller sales failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void OrderController::getOrderDetails(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string orderId) {
  if (!isValidObjectId(orderId)) {
    callback(jsonError(k400BadRequest, "Invalid order ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  try {
    OrderRepository orders;
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    OrderService service(orders, carts, books, users);

    const auto order = service.getOrderDetails(orderId, userId);

    Json::Value body;
    body["success"] = true;
    body["order"] = orderJson(order);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "ORDER_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Order not found"));
      return;
    }
    if (error == "FORBIDDEN") {
      callback(jsonError(k403Forbidden,
                         "You do not have permission to view this order"));
      return;
    }
    LOG_ERROR << "Get order details failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Get order details failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void OrderController::updateStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string orderId) {
  if (!isValidObjectId(orderId)) {
    callback(jsonError(k400BadRequest, "Invalid order ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  auto json = req->getJsonObject();
  if (!json || !json->isMember("status") || !(*json)["status"].isString()) {
    callback(jsonError(k400BadRequest, "status is required and must be a string"));
    return;
  }

  const auto newStatus = (*json)["status"].asString();
  std::string note;
  if (json->isMember("note") && (*json)["note"].isString()) {
    note = (*json)["note"].asString();
  }

  try {
    OrderRepository orders;
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    OrderService service(orders, carts, books, users);

    const auto updated =
        service.updateDeliveryStatus(orderId, userId, newStatus, note);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Order delivery status updated successfully";
    body["order"] = orderJson(updated);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
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
      callback(jsonError(k403Forbidden,
                         "Only the seller of order items may update delivery status"));
      return;
    }
    LOG_ERROR << "Update order status failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Update order status failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void OrderController::cancelOrder(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string orderId) {
  if (!isValidObjectId(orderId)) {
    callback(jsonError(k400BadRequest, "Invalid order ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  std::string reason;
  auto json = req->getJsonObject();
  if (json && json->isMember("reason") && (*json)["reason"].isString()) {
    reason = (*json)["reason"].asString();
  }

  try {
    OrderRepository orders;
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    OrderService service(orders, carts, books, users);

    const auto updated = service.cancelOrder(orderId, userId, reason);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Order cancelled successfully";
    body["order"] = orderJson(updated);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
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
    LOG_ERROR << "Cancel order failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Cancel order failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void OrderController::requestReturn(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string orderId) {
  if (!isValidObjectId(orderId)) {
    callback(jsonError(k400BadRequest, "Invalid order ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  std::string reason;
  auto json = req->getJsonObject();
  if (json && json->isMember("reason") && (*json)["reason"].isString()) {
    reason = (*json)["reason"].asString();
  }

  try {
    OrderRepository orders;
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    OrderService service(orders, carts, books, users);

    const auto updated = service.requestReturn(orderId, userId, reason);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Return requested successfully";
    body["order"] = orderJson(updated);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
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
    LOG_ERROR << "Request return failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Request return failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void OrderController::processReturn(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string orderId) {
  if (!isValidObjectId(orderId)) {
    callback(jsonError(k400BadRequest, "Invalid order ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  auto json = req->getJsonObject();
  if (!json || !json->isMember("approve") || !(*json)["approve"].isBool()) {
    callback(jsonError(k400BadRequest, "approve boolean is required"));
    return;
  }

  const bool approve = (*json)["approve"].asBool();
  std::string note;
  if (json->isMember("note") && (*json)["note"].isString()) {
    note = (*json)["note"].asString();
  }

  try {
    OrderRepository orders;
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    OrderService service(orders, carts, books, users);

    const auto updated = service.processReturn(orderId, userId, approve, note);

    Json::Value body;
    body["success"] = true;
    body["message"] = approve ? "Return approved successfully"
                              : "Return rejected successfully";
    body["order"] = orderJson(updated);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
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
    LOG_ERROR << "Process return failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Process return failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

