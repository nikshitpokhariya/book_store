#pragma once

#include <drogon/HttpController.h>

class CartController : public drogon::HttpController<CartController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(CartController::getCart, "/api/cart", drogon::Get,
                "AuthFilter");

  ADD_METHOD_TO(CartController::addItem, "/api/cart/items", drogon::Post,
                "AuthFilter");

  ADD_METHOD_TO(CartController::removeItem, "/api/cart/items/{1}",
                drogon::Delete, "AuthFilter");

  ADD_METHOD_TO(CartController::clearCart, "/api/cart", drogon::Delete,
                "AuthFilter");

  METHOD_LIST_END

  void getCart(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void addItem(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void
  removeItem(const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
             std::string bookId);

  void clearCart(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
