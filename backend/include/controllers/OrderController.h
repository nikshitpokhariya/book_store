#pragma once

#include <drogon/HttpController.h>

class OrderController : public drogon::HttpController<OrderController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(OrderController::checkout, "/api/checkout", drogon::Post,
                "AuthFilter");

  ADD_METHOD_TO(OrderController::getBuyerOrders, "/api/orders", drogon::Get,
                "AuthFilter");

  ADD_METHOD_TO(OrderController::getSellerSales, "/api/orders/seller/history",
                drogon::Get, "AuthFilter");

  ADD_METHOD_TO(OrderController::getOrderDetails, "/api/orders/{1}",
                drogon::Get, "AuthFilter");

  ADD_METHOD_TO(OrderController::updateStatus, "/api/orders/{1}/status",
                drogon::Patch, "AuthFilter");

  ADD_METHOD_TO(OrderController::cancelOrder, "/api/orders/{1}/cancel",
                drogon::Post, "AuthFilter");

  ADD_METHOD_TO(OrderController::requestReturn, "/api/orders/{1}/return",
                drogon::Post, "AuthFilter");

  ADD_METHOD_TO(OrderController::processReturn,
                "/api/orders/{1}/return/process", drogon::Post, "AuthFilter");

  METHOD_LIST_END

  void checkout(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getBuyerOrders(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getSellerSales(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getOrderDetails(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string orderId);

  void updateStatus(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string orderId);

  void cancelOrder(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string orderId);

  void requestReturn(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string orderId);

  void processReturn(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string orderId);
};
