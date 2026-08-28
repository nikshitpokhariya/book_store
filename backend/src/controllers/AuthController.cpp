#include "controllers/AuthController.h"
#include "config/AppConfig.h"

#include "repositories/SessionRepository.h"
#include "repositories/UserRepository.h"
#include "services/AuthService.h"

#include <drogon/drogon.h>

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

Json::Value userJson(const User &user) {
  Json::Value result;

  result["id"] = user.id;
  result["username"] = user.username;
  result["email"] = user.email;

  return result;
}

std::string getRefreshToken(const HttpRequestPtr &req) {
  return req->getCookie("refreshToken");
}

void setRefreshCookie(const HttpResponsePtr &response, const std::string &token,
                      std::int64_t maxAge, bool secure) {
  std::string cookie = "refreshToken=" + token +
                       "; Max-Age=" + std::to_string(maxAge) +
                       "; Path=/api/auth" + "; HttpOnly" + "; SameSite=Lax";

  if (secure)
    cookie += "; Secure";

  response->addHeader("Set-Cookie", cookie);
}

void clearRefreshCookie(const HttpResponsePtr &response, bool secure) {
  std::string cookie = "refreshToken=;"
                       " Max-Age=0;"
                       " Path=/api/auth;"
                       " HttpOnly;"
                       " SameSite=Lax";

  if (secure)
    cookie += " Secure";

  response->addHeader("Set-Cookie", cookie);
}
} // namespace

void AuthController::signup(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  auto json = req->getJsonObject();

  if (!json) {
    callback(jsonError(k400BadRequest, "Invalid JSON"));

    return;
  }

  if (!json->isMember("username") || !json->isMember("email") ||
      !json->isMember("password")) {
    callback(
        jsonError(k400BadRequest, "Username, email and password are required"));

    return;
  }

  if (!(*json)["username"].isString() || !(*json)["email"].isString() ||
      !(*json)["password"].isString()) {
    callback(jsonError(k400BadRequest, "Invalid field types"));

    return;
  }

  const auto username = (*json)["username"].asString();

  const auto email = (*json)["email"].asString();

  const auto password = (*json)["password"].asString();

  if (username.empty() || email.empty() || password.empty()) {
    callback(jsonError(k400BadRequest, "Fields cannot be empty"));

    return;
  }

  try {
    UserRepository users;
    SessionRepository sessions;

    AuthService auth(users, sessions);

    const auto result = auth.signup(username, email, password);

    Json::Value body;

    body["success"] = true;
    body["message"] = "Signup successful";

    body["user"] = userJson(result.user);

    auto response = HttpResponse::newHttpJsonResponse(body);

    response->setStatusCode(k201Created);

    callback(response);
  } catch (const std::invalid_argument &e) {
    callback(jsonError(k400BadRequest, e.what()));
  } catch (const std::runtime_error &e) {
    const std::string error = e.what();

    if (error == "USERNAME_EXISTS") {
      callback(jsonError(k409Conflict, "Username already exists"));

      return;
    }

    if (error == "EMAIL_EXISTS") {
      callback(jsonError(k409Conflict, "Email already exists"));

      return;
    }

    LOG_ERROR << e.what();

    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void AuthController::login(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  auto json = req->getJsonObject();

  if (!json || !json->isMember("email") || !json->isMember("password")) {
    callback(jsonError(k400BadRequest, "Email and password are required"));

    return;
  }

  if (!(*json)["email"].isString() || !(*json)["password"].isString()) {
    callback(jsonError(k400BadRequest, "Invalid field types"));

    return;
  }

  const auto email = (*json)["email"].asString();

  const auto password = (*json)["password"].asString();

  if (email.empty() || password.empty()) {
    callback(jsonError(k400BadRequest, "Fields cannot be empty"));

    return;
  }

  try {
    UserRepository users;
    SessionRepository sessions;

    AuthService auth(users, sessions);

    const auto result = auth.login(email, password);

    Json::Value body;

    body["success"] = true;
    body["message"] = "Login successful";

    body["accessToken"] = result.accessToken.token;

    body["expiresIn"] = static_cast<Json::Int64>(result.accessToken.expiresIn);

    body["user"] = userJson(result.user);

    auto response = HttpResponse::newHttpJsonResponse(body);

    response->setStatusCode(k200OK);

    setRefreshCookie(response, result.refreshToken,
                     AppConfig::instance().refreshTokenLifetime(),
                     AppConfig::instance().secureCookies());

    callback(response);
  } catch (const std::runtime_error &e) {
    if (std::string(e.what()) == "INVALID_CREDENTIALS") {
      callback(jsonError(k401Unauthorized, "Invalid email or password"));

      return;
    }

    LOG_ERROR << e.what();

    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void AuthController::refresh(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto refreshToken = getRefreshToken(req);

  if (refreshToken.empty()) {
    callback(jsonError(k401Unauthorized, "Refresh token required"));

    return;
  }

  try {
    UserRepository users;
    SessionRepository sessions;

    AuthService auth(users, sessions);

    const auto result = auth.refresh(refreshToken);

    Json::Value body;

    body["success"] = true;

    body["accessToken"] = result.accessToken.token;

    body["expiresIn"] = static_cast<Json::Int64>(result.accessToken.expiresIn);

    auto response = HttpResponse::newHttpJsonResponse(body);

    response->setStatusCode(k200OK);

    setRefreshCookie(response, result.refreshToken,
                     AppConfig::instance().refreshTokenLifetime(),
                     AppConfig::instance().secureCookies());

    callback(response);
  } catch (const std::runtime_error &e) {
    if (std::string(e.what()) == "INVALID_REFRESH_TOKEN") {
      callback(jsonError(k401Unauthorized, "Invalid or expired refresh token"));

      return;
    }

    LOG_ERROR << e.what();

    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void AuthController::logout(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto refreshToken = getRefreshToken(req);

  try {
    UserRepository users;
    SessionRepository sessions;

    AuthService auth(users, sessions);

    auth.logout(refreshToken);

    Json::Value body;

    body["success"] = true;
    body["message"] = "Logged out successfully";

    auto response = HttpResponse::newHttpJsonResponse(body);

    response->setStatusCode(k200OK);

    clearRefreshCookie(response, AppConfig::instance().secureCookies());

    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << e.what();

    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}

void AuthController::me(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  const auto userId = req->attributes()->get<std::string>("userId");

  try {
    UserRepository users;
    SessionRepository sessions;

    AuthService auth(users, sessions);

    const auto user = auth.getUser(userId);

    Json::Value body;

    body["success"] = true;
    body["user"] = userJson(user);

    auto response = HttpResponse::newHttpJsonResponse(body);

    response->setStatusCode(k200OK);

    callback(response);
  } catch (const std::runtime_error &e) {
    if (std::string(e.what()) == "USER_NOT_FOUND") {
      callback(jsonError(k404NotFound, "User not found"));

      return;
    }

    LOG_ERROR << e.what();

    callback(jsonError(k500InternalServerError, "Internal server error"));
  }
}