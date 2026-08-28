#pragma once

#include "../models/Session.h"

#include <optional>
#include <string>

class SessionRepository {
public:
  void create(const std::string &userId, const std::string &refreshTokenHash);

  bool exists(const std::string &refreshTokenHash);

  std::optional<Session> findByHash(const std::string &refreshTokenHash);

  void remove(const std::string &refreshTokenHash);

  void rotate(const std::string &oldRefreshTokenHash,
              const std::string &newRefreshTokenHash,
              const std::string &userId);
};