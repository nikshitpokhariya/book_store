#include "services/AuthService.h"

#include "security/PasswordHasher.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {
void normalizeEmail(std::string &email) {
  std::transform(email.begin(), email.end(), email.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
}

bool validEmail(const std::string &email) {
  const auto at = email.find('@');

  const auto dot = email.find('.', at);

  return at != std::string::npos && dot != std::string::npos && at > 0 &&
         dot > at + 1 && dot + 1 < email.size();
}
} // namespace

AuthService::AuthService(UserRepository &users, SessionRepository &sessions)
    : users_(users), sessions_(sessions) {}

SignupResult AuthService::signup(std::string username, std::string email,
                                 std::string password) {
  normalizeEmail(email);

  if (username.size() < 3 || username.size() > 30) {
    throw std::invalid_argument("Username must be between 3 and 30 characters");
  }

  if (!validEmail(email)) {
    throw std::invalid_argument("Invalid email address");
  }

  if (password.size() < 8 || password.size() > 128) {
    throw std::invalid_argument(
        "Password must be between 8 and 128 characters");
  }

  if (users_.existsByUsername(username)) {
    throw std::runtime_error("USERNAME_EXISTS");
  }

  if (users_.existsByEmail(email)) {
    throw std::runtime_error("EMAIL_EXISTS");
  }

  User user;

  user.username = std::move(username);
  user.email = std::move(email);

  user.passwordHash = PasswordHasher::hash(password);

  user.id = users_.create(user);

  return {std::move(user)};
}

LoginResult AuthService::login(std::string email, std::string password) {
  normalizeEmail(email);

  auto user = users_.findByEmail(email);

  if (!user) {
    throw std::runtime_error("INVALID_CREDENTIALS");
  }

  if (!PasswordHasher::verify(password, user->passwordHash)) {
    throw std::runtime_error("INVALID_CREDENTIALS");
  }

  auto accessToken = TokenService::createAccessToken(user->id);

  auto refreshToken = TokenService::createRefreshToken();

  auto refreshHash = TokenService::hashRefreshToken(refreshToken);

  sessions_.create(user->id, refreshHash);

  return {std::move(*user), std::move(accessToken), std::move(refreshToken)};
}

RefreshResult AuthService::refresh(const std::string &refreshToken) {
  if (refreshToken.empty()) {
    throw std::runtime_error("INVALID_REFRESH_TOKEN");
  }

  const auto oldHash = TokenService::hashRefreshToken(refreshToken);

  auto session = sessions_.findByHash(oldHash);

  if (!session) {
    throw std::runtime_error("INVALID_REFRESH_TOKEN");
  }

  const auto &userId = session->userId;

  auto user = users_.findById(userId);

  if (!user) {
    sessions_.remove(oldHash);

    throw std::runtime_error("INVALID_REFRESH_TOKEN");
  }

  auto newRefreshToken = TokenService::createRefreshToken();

  auto newRefreshHash = TokenService::hashRefreshToken(newRefreshToken);

  sessions_.rotate(oldHash, newRefreshHash, userId);

  auto accessToken = TokenService::createAccessToken(userId);

  return {std::move(accessToken), std::move(newRefreshToken)};
}

void AuthService::logout(const std::string &refreshToken) {
  if (refreshToken.empty())
    return;

  const auto hash = TokenService::hashRefreshToken(refreshToken);

  sessions_.remove(hash);
}

User AuthService::getUser(const std::string &userId) {
  auto user = users_.findById(userId);

  if (!user) {
    throw std::runtime_error("USER_NOT_FOUND");
  }

  return std::move(*user);
}