#include "security/PasswordHasher.hpp"

#include <argon2.h>
#include <openssl/rand.h>

#include <cstdint>
#include <stdexcept>
#include <string>

std::string PasswordHasher::hash(const std::string &password) {
  constexpr std::size_t saltLength = 16;
  constexpr std::size_t hashLength = 32;

  constexpr std::uint32_t timeCost = 3;
  constexpr std::uint32_t memoryCost = 64 * 1024; // 64 MiB
  constexpr std::uint32_t parallelism = 1;

  unsigned char salt[saltLength];

  // Generate a cryptographically secure random salt via OpenSSL.
  if (RAND_bytes(salt, static_cast<int>(saltLength)) != 1) {
    throw std::runtime_error("Unable to generate password salt");
  }

  char encoded[512];

  const int result = argon2id_hash_encoded(timeCost, memoryCost, parallelism,

                                           password.data(), password.size(),

                                           salt, saltLength,

                                           hashLength,

                                           encoded, sizeof(encoded));

  if (result != ARGON2_OK) {
    throw std::runtime_error(argon2_error_message(result));
  }

  return std::string(encoded);
}

bool PasswordHasher::verify(const std::string &password,
                            const std::string &encodedHash) {

  return argon2id_verify(encodedHash.c_str(), password.data(),
                         password.size()) == ARGON2_OK;
}

