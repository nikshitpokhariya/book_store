#include "config/AppConfig.h"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
void loadDotEnv(const std::string &path = ".env") {
  std::ifstream file(path);
  if (!file.is_open())
    return;

  std::string line;
  while (std::getline(file, line)) {
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    if (line.empty() || line[0] == '#')
      continue;

    auto eqPos = line.find('=');
    if (eqPos == std::string::npos)
      continue;

    std::string key = line.substr(0, eqPos);
    std::string val = line.substr(eqPos + 1);

    key.erase(key.find_last_not_of(" \t\r\n") + 1);
    val.erase(0, val.find_first_not_of(" \t\r\n"));
    val.erase(val.find_last_not_of(" \t\r\n") + 1);

    if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') ||
                            (val.front() == '\'' && val.back() == '\''))) {
      val = val.substr(1, val.size() - 2);
    }

    if (!std::getenv(key.c_str())) {
#ifdef _WIN32
      _putenv_s(key.c_str(), val.c_str());
#else
      setenv(key.c_str(), val.c_str(), 0);
#endif
    }
  }
}

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

AppConfig::AppConfig() {
  loadDotEnv();

  auto portVal = getIntEnv("PORT", 8080);
  if (portVal <= 0 || portVal > 65535) {
    portVal = 8080;
  }
  port_ = static_cast<uint16_t>(portVal);

  mongoUri_ = getEnv("MONGO_URI");
  mongoDatabase_ = getEnv("MONGO_DATABASE", "myapp");
  jwtSecret_ = getEnv("JWT_SECRET");
  jwtIssuer_ = getEnv("JWT_ISSUER", "myapp-api");
  accessTokenLifetime_ = getIntEnv("ACCESS_TOKEN_LIFETIME", 900);
  bool isProduction = false;
  const char *nodeEnv = std::getenv("NODE_ENV");
  const char *envVal = std::getenv("ENVIRONMENT");
  const char *renderEnv = std::getenv("RENDER");
  if ((nodeEnv && std::string(nodeEnv) == "production") ||
      (envVal && std::string(envVal) == "production") ||
      (renderEnv && std::string(renderEnv).length() > 0)) {
    isProduction = true;
  }
  isProduction_ = isProduction;
  secureCookies_ = getBoolEnv("SECURE_COOKIES", isProduction);

  // Standalone MongoDB fallback is strictly development-only and requires explicit opt-in.
  // In production (including Render), standalone fallback is always disabled (fails closed).
  allowStandaloneDev_ = !isProduction_ && getBoolEnv("ALLOW_STANDALONE_DEV", false);

  cloudinaryCloudName_ = getEnv("CLOUDINARY_CLOUD_NAME", "");
  cloudinaryApiKey_ = getEnv("CLOUDINARY_API_KEY", "");
  cloudinaryApiSecret_ = getEnv("CLOUDINARY_API_SECRET", "");

  if (jwtSecret_.size() < 32) {
    throw std::runtime_error("JWT_SECRET must contain at least 32 characters");
  }
}

const AppConfig &AppConfig::instance() {
  static const AppConfig config;
  return config;
}

uint16_t AppConfig::port() const { return port_; }

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

bool AppConfig::isProduction() const { return isProduction_; }

bool AppConfig::allowStandaloneDev() const { return allowStandaloneDev_; }

const std::string &AppConfig::cloudinaryCloudName() const {
  return cloudinaryCloudName_;
}

const std::string &AppConfig::cloudinaryApiKey() const {
  return cloudinaryApiKey_;
}

const std::string &AppConfig::cloudinaryApiSecret() const {
  return cloudinaryApiSecret_;
}