#include "config/AppConfig.h"

#include <cstdlib>
#include <stdexcept>

namespace {
std::string getEnv(const char *name, const char *defaultValue = nullptr) {
  const char *value = std::getenv(name);

  if (value)
    return value;

  if (defaultValue)
    return defaultValue;

  throw std::runtime_error(std::string("Missing environment variable: ") +
                           name);
}

std::int64_t getIntEnv(const char *name, std::int64_t defaultValue) {
  const char *value = std::getenv(name);

  if (!value)
    return defaultValue;

  return std::stoll(value);
}

bool getBoolEnv(const char *name, bool defaultValue) {
  const char *value = std::getenv(name);

  if (!value)
    return defaultValue;

  return std::string(value) == "true";
}
} // namespace

AppConfig::AppConfig()
    : mongoUri_(getEnv("MONGO_URI", "mongodb://127.0.0.1:27017")),
      mongoDatabase_(getEnv("MONGO_DATABASE", "myapp")),
      jwtSecret_(getEnv("JWT_SECRET")),
      jwtIssuer_(getEnv("JWT_ISSUER", "myapp-api")),
      accessTokenLifetime_(getIntEnv("ACCESS_TOKEN_LIFETIME", 900)),
      refreshTokenLifetime_(getIntEnv("REFRESH_TOKEN_LIFETIME", 2592000)),
      secureCookies_(getBoolEnv("SECURE_COOKIES", false)) {
  if (jwtSecret_.size() < 32) {
    throw std::runtime_error("JWT_SECRET must contain at least 32 characters");
  }
}

const AppConfig &AppConfig::instance() {
  static const AppConfig config;
  return config;
}

const std::string &AppConfig::mongoUri() const { return mongoUri_; }

const std::string &AppConfig::mongoDatabase() const { return mongoDatabase_; }

const std::string &AppConfig::jwtSecret() const { return jwtSecret_; }

const std::string &AppConfig::jwtIssuer() const { return jwtIssuer_; }

std::int64_t AppConfig::accessTokenLifetime() const {
  return accessTokenLifetime_;
}

std::int64_t AppConfig::refreshTokenLifetime() const {
  return refreshTokenLifetime_;
}

bool AppConfig::secureCookies() const { return secureCookies_; }