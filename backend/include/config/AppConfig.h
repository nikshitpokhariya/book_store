#pragma once

#include <cstdint>
#include <string>

class AppConfig {
public:
  static const AppConfig &instance();

  const std::string &mongoUri() const;
  const std::string &mongoDatabase() const;

  const std::string &jwtSecret() const;
  const std::string &jwtIssuer() const;

  std::int64_t accessTokenLifetime() const;
  std::int64_t refreshTokenLifetime() const;

  bool secureCookies() const;

private:
  AppConfig();

  std::string mongoUri_;
  std::string mongoDatabase_;

  std::string jwtSecret_;
  std::string jwtIssuer_;

  std::int64_t accessTokenLifetime_;
  std::int64_t refreshTokenLifetime_;

  bool secureCookies_;
};