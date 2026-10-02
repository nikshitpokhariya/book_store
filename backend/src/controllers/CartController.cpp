#include "controllers/CartController.h"

#include "models/Cart.h"
#include "repositories/BookRepository.h"
#include "repositories/CartRepository.h"
#include "repositories/UserRepository.h"
#include "services/CartService.h"

#include <drogon/drogon.h>

#include <cctype>

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

bool isValidObjectId(const std::string &id) {
  if (id.size() != 24)
    return false;
  for (char c : id) {
    if (!std::isxdigit(static_cast<unsigned char>(c)))
      return false;
  }
  return true;
}

Json::Value cartJson(const Cart &cart) {
  Json::Value result;
  result["userId"] = cart.userId;
  result["subtotal"] = cart.subtotal;
  result["shippingCost"] = cart.shippingCost;
  result["total"] = cart.total;
  result["hasUnavailableItems"] = cart.hasUnavailableItems;

  Json::Value items(Json::arrayValue);
  for (const auto &item : cart.items) {
    Json::Value itemJson;
    itemJson["bookId"] = item.bookId;
    itemJson["title"] = item.title;
    itemJson["author"] = item.author;
    itemJson["price"] = item.price;
    itemJson["condition"] = item.condition;
    itemJson["category"] = item.category;
    itemJson["coverImage"] = item.coverImage;
    itemJson["status"] = item.status;
    itemJson["isAvailable"] = item.isAvailable;
    itemJson["quantity"] = 1;

    Json::Value ownerJson;
    ownerJson["id"] = item.ownerId;
    ownerJson["username"] = item.ownerUsername;
    itemJson["owner"] = ownerJson;

    items.append(itemJson);
  }
  result["items"] = items;

  return result;
}
} // namespace

void CartController::getCart(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  try {
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    CartService service(carts, books, users);

    const auto cart = service.getCart(userId);

    Json::Value body;
    body["success"] = true;
    body["cart"] = cartJson(cart);

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << "Get cart failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void CartController::addItem(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  auto json = req->getJsonObject();
  if (!json || !json->isMember("bookId") || !(*json)["bookId"].isString()) {
    callback(jsonError(k400BadRequest, "bookId is required and must be a string"));
    return;
  }

  const auto bookId = (*json)["bookId"].asString();
  if (!isValidObjectId(bookId)) {
    callback(jsonError(k400BadRequest, "Invalid book ID format"));
    return;
  }

  if (json->isMember("quantity")) {
    if (!(*json)["quantity"].isInt() || (*json)["quantity"].asInt() != 1) {
      callback(jsonError(
          k400BadRequest,
          "Quantity must be exactly 1 for a physical book listing"));
      return;
    }
  }

  try {
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    CartService service(carts, books, users);

    service.addItem(userId, bookId);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Book added to cart successfully";

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k201Created);
    callback(response);
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();
    if (error == "BOOK_NOT_FOUND") {
      callback(jsonError(k404NotFound, "Book listing not found"));
      return;
    }
    if (error == "CANNOT_BUY_OWN_BOOK") {
      callback(jsonError(k400BadRequest, "Cannot add your own listing to cart"));
      return;
    }
    if (error == "BOOK_NOT_AVAILABLE") {
      callback(jsonError(k400BadRequest,
                         "Book listing is not available for purchase"));
      return;
    }
    if (error == "ITEM_ALREADY_IN_CART") {
      callback(jsonError(k400BadRequest,
                         "Book is already in cart. Quantity cannot exceed 1."));
      return;
    }
    LOG_ERROR << "Add to cart failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  } catch (const std::exception &e) {
    LOG_ERROR << "Add to cart failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void CartController::removeItem(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string bookId) {
  if (!isValidObjectId(bookId)) {
    callback(jsonError(k400BadRequest, "Invalid book ID format"));
    return;
  }

  const auto userId = req->attributes()->get<std::string>("userId");

  try {
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    CartService service(carts, books, users);

    service.removeItem(userId, bookId);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Item removed from cart";

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << "Remove cart item failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void CartController::clearCart(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  try {
    CartRepository carts;
    BookRepository books;
    UserRepository users;
    CartService service(carts, books, users);

    service.clearCart(userId);

    Json::Value body;
    body["success"] = true;
    body["message"] = "Cart cleared successfully";

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k200OK);
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << "Clear cart failed: " << e.what();
    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}
