#pragma once

#include <drogon/HttpController.h>

class ExchangeController : public drogon::HttpController<ExchangeController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(ExchangeController::createRequest, "/api/exchanges",
                drogon::Post, "AuthFilter");

  ADD_METHOD_TO(ExchangeController::getSentRequests, "/api/exchanges/sent",
                drogon::Get, "AuthFilter");

  ADD_METHOD_TO(ExchangeController::getReceivedRequests,
                "/api/exchanges/received", drogon::Get, "AuthFilter");

  ADD_METHOD_TO(ExchangeController::getUserHistory, "/api/exchanges/history",
                drogon::Get, "AuthFilter");

  ADD_METHOD_TO(ExchangeController::getRequestDetails, "/api/exchanges/{1}",
                drogon::Get, "AuthFilter");

  ADD_METHOD_TO(ExchangeController::acceptExchange,
                "/api/exchanges/{1}/accept", drogon::Post, "AuthFilter");

  ADD_METHOD_TO(ExchangeController::rejectExchange,
                "/api/exchanges/{1}/reject", drogon::Post, "AuthFilter");

  ADD_METHOD_TO(ExchangeController::cancelExchange,
                "/api/exchanges/{1}/cancel", drogon::Post, "AuthFilter");

  METHOD_LIST_END

  void createRequest(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getSentRequests(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getReceivedRequests(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getUserHistory(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getRequestDetails(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string requestId);

  void acceptExchange(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string requestId);

  void rejectExchange(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string requestId);

  void cancelExchange(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string requestId);
};
