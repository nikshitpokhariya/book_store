#pragma once

#include <drogon/HttpFilter.h>

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

/**
 * @brief Thread-safe, process-local rate limiter filter for BookBazzar API.
 *
 * Enforces request limits per client IP within a 60-second window:
 * - Auth (login, signup): 15 requests / 60s
 * - Uploads (/api/upload): 20 requests / 60s
 * - Expensive public read (/api/books browse & suggestions): 120 requests / 60s
 * - Other endpoints: 300 requests / 60s default
 *
 * Thread Safety: Fully thread-safe across all Drogon worker threads using std::mutex.
 * Scope: Process-local in-memory state. In multi-instance deployments behind a load
 *        balancer, a distributed Redis-based limiter is recommended.
 */
class RateLimitFilter : public drogon::HttpFilter<RateLimitFilter> {
public:
  RateLimitFilter() = default;

  void doFilter(const drogon::HttpRequestPtr &req,
                drogon::FilterCallback &&callback,
                drogon::FilterChainCallback &&chainCallback) override;

  static void resetState();
  static bool checkRateLimit(const std::string &category, const std::string &clientIp,
                            int maxRequests, int windowSeconds);

private:
  static std::string getClientIp(const drogon::HttpRequestPtr &req);
};
