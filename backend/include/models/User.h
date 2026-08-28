#pragma once

#include <string>

struct User {
  std::string id;
  std::string username;
  std::string email;
  std::string passwordHash;
};