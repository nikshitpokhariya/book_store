#pragma once

#include "../models/User.h"
#include "../repositories/SessionRepository.h"
#include "../repositories/UserRepository.h"
#include "../security/TokenService.hpp"

#include <string>

struct SignupResult {
  User user;
};

struct LoginResult {
  User user;
  AccessToken accessToken;
  std::string refreshToken;
};

struct RefreshResult {
  AccessToken accessToken;
  std::string refreshToken;
};

class AuthService {
public:
  AuthService(UserRepository &users, SessionRepository &sessions);

  SignupResult signup(std::string username, std::string email,
                      std::string password);

  LoginResult login(std::string email, std::string password);

  RefreshResult refresh(const std::string &refreshToken);

  void logout(const std::string &refreshToken);

  User getUser(const std::string &userId);

private:
  UserRepository &users_;
  SessionRepository &sessions_;
};