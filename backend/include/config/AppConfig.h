#pragma once

#include <cstdint>
#include <string>

class AppConfig {
public:
  static const AppConfig &instance();

  uint16_t port() const;

  const std::string &mongoUri() const;
  const std::string &mongoDatabase() const;

  const std::string &jwtSecret() const;
  const std::string &jwtIssuer() const;

  std::int64_t accessTokenLifetime() const;
  std::int64_t refreshTokenLifetime() const;

  bool secureCookies() const;
  bool isProduction() const;
  bool allowStandaloneDev() const;

  const std::string &cloudinaryCloudName() const;
  const std::string &cloudinaryApiKey() const;
  const std::string &cloudinaryApiSecret() const;

private:
  AppConfig();

  uint16_t port_{8080};
  std::string mongoUri_;
  std::string mongoDatabase_;

  std::string jwtSecret_;
  std::string jwtIssuer_;

  std::int64_t accessTokenLifetime_;
  std::int64_t refreshTokenLifetime_;

  bool secureCookies_;
  bool isProduction_{false};
  bool allowStandaloneDev_{false};

  std::string cloudinaryCloudName_;
  std::string cloudinaryApiKey_;
  std::string cloudinaryApiSecret_;
};