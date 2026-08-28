#pragma once

#include <string>

struct Session {
  std::string id;
  std::string userId;
  std::string refreshTokenHash;
};