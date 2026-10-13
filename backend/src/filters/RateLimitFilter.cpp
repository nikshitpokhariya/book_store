#include "filters/RateLimitFilter.h"

#include <drogon/drogon.h>

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

using namespace drogon;

namespace {

struct RateLimitEntry {
  int count{0};
  std::chrono::steady_clock::time_point windowStart;
};

std::mutex s_mutex;
std::unordered_map<std::string, RateLimitEntry> s_limits;

void cleanExpiredEntries(std::chrono::steady_clock::time_point now) {
  if (s_limits.size() < 1000) {
    return;
  }
  for (auto it = s_limits.begin(); it != s_limits.end();) {
    if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.windowStart).count() > 120) {
      it = s_limits.erase(it);
    } else {
      ++it;
    }
  }
}

} // namespace

void RateLimitFilter::resetState() {
  std::lock_guard<std::mutex> lock(s_mutex);
  s_limits.clear();
}

bool RateLimitFilter::checkRateLimit(const std::string &category,
                                    const std::string &clientIp,
                                    int maxRequests, int windowSeconds) {
  std::lock_guard<std::mutex> lock(s_mutex);
  const auto now = std::chrono::steady_clock::now();
  cleanExpiredEntries(now);

  const std::string key = category + ":" + clientIp;
  auto &entry = s_limits[key];

  if (entry.count == 0) {
    entry.count = 1;
    entry.windowStart = now;
    return true;
  }

  const auto elapsedSeconds =
      std::chrono::duration_cast<std::chrono::seconds>(now - entry.windowStart).count();

  if (elapsedSeconds >= windowSeconds) {
    entry.count = 1;
    entry.windowStart = now;
    return true;
  }

  if (entry.count < maxRequests) {
    entry.count++;
    return true;
  }

  return false;
}

std::string RateLimitFilter::getClientIp(const HttpRequestPtr &req) {
  std::string forwarded = req->getHeader("X-Forwarded-For");
  if (!forwarded.empty()) {
    const auto comma = forwarded.find(',');
    if (comma != std::string::npos) {
      forwarded = forwarded.substr(0, comma);
    }
    const auto start = forwarded.find_first_not_of(" \t\r\n");
    const auto end = forwarded.find_last_not_of(" \t\r\n");
    if (start != std::string::npos && end != std::string::npos) {
      return forwarded.substr(start, end - start + 1);
    }
  }

  std::string peer = req->getPeerAddr().toIp();
  if (peer.empty() || peer == "0.0.0.0") {
    return "127.0.0.1";
  }
  return peer;
}

void RateLimitFilter::doFilter(const HttpRequestPtr &req,
                               FilterCallback &&callback,
                               FilterChainCallback &&chainCallback) {
  const std::string path = req->path();
  const std::string clientIp = getClientIp(req);

  std::string category = "general";
  int maxRequests = 300;
  int windowSeconds = 60;

  if (path.find("/api/auth/login") != std::string::npos ||
      path.find("/api/auth/signup") != std::string::npos) {
    category = "auth";
    maxRequests = 15;
    windowSeconds = 60;
  } else if (path.find("/api/upload") != std::string::npos) {
    category = "upload";
    maxRequests = 20;
    windowSeconds = 60;
  } else if (path.find("/api/books") != std::string::npos && req->method() == drogon::Get) {
    category = "public_books";
    maxRequests = 120;
    windowSeconds = 60;
  }

  const bool allowed = checkRateLimit(category, clientIp, maxRequests, windowSeconds);

  if (!allowed) {
    Json::Value body;
    body["success"] = false;
    body["message"] = "Too many requests. Please slow down and try again later.";

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k429TooManyRequests);
    response->addHeader("Retry-After", std::to_string(windowSeconds));
    callback(response);
    return;
  }

  chainCallback();
}
