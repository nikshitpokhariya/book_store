#include "filters/AuthFilter.h"

#include "security/TokenService.hpp"

#include <string>

using namespace drogon;

void AuthFilter::doFilter(const HttpRequestPtr &req,
                          drogon::FilterCallback &&callback,
                          drogon::FilterChainCallback &&chainCallback) {
  const auto authorization = req->getHeader("Authorization");

  constexpr const char *prefix = "Bearer ";

  if (authorization.size() <= 7 || authorization.compare(0, 7, prefix) != 0) {
    Json::Value body;

    body["success"] = false;
    body["message"] = "Authentication required";

    auto response = HttpResponse::newHttpJsonResponse(body);

    response->setStatusCode(k401Unauthorized);

    callback(response);
    return;
  }

  const auto token = authorization.substr(7);

  const auto userId = TokenService::verifyAccessToken(token);

  if (userId.empty()) {
    Json::Value body;

    body["success"] = false;
    body["message"] = "Invalid or expired access token";

    auto response = HttpResponse::newHttpJsonResponse(body);

    response->setStatusCode(k401Unauthorized);

    callback(response);
    return;
  }

  req->attributes()->insert("userId", userId);

  chainCallback();
}