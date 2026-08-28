#pragma once

#include "../models/User.h"

#include <optional>
#include <string>

class UserRepository {
public:
  std::optional<User> findById(const std::string &id);

  std::optional<User> findByEmail(const std::string &email);

  std::optional<User> findByUsername(const std::string &username);

  bool existsByEmail(const std::string &email);

  bool existsByUsername(const std::string &username);

  std::string create(const User &user);
};