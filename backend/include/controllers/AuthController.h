#pragma once

#include <drogon/HttpController.h>

class AuthController : public drogon::HttpController<AuthController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(AuthController::signup, "/api/auth/signup", drogon::Post);

  ADD_METHOD_TO(AuthController::login, "/api/auth/login", drogon::Post);

  ADD_METHOD_TO(AuthController::refresh, "/api/auth/refresh", drogon::Post);

  ADD_METHOD_TO(AuthController::logout, "/api/auth/logout", drogon::Post);

  ADD_METHOD_TO(AuthController::me, "/api/auth/me", drogon::Get, "AuthFilter");

  METHOD_LIST_END

  void signup(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void login(const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void refresh(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void logout(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void me(const drogon::HttpRequestPtr &req,
          std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};