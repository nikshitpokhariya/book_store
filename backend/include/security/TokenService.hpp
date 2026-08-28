#pragma once

#include <string>

struct AccessToken {
  std::string token;
  std::int64_t expiresIn;
};

class TokenService {
public:
  static AccessToken createAccessToken(const std::string &userId);

  static std::string createRefreshToken();

  static std::string hashRefreshToken(const std::string &token);

  static std::string verifyAccessToken(const std::string &token);
};