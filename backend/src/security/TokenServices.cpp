#include "security/TokenService.hpp"

#include "config/AppConfig.h"

#include <jwt-cpp/traits/open-source-parsers-jsoncpp/defaults.h>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
std::string toHex(const unsigned char *data, std::size_t size) {
  std::ostringstream output;

  output << std::hex << std::setfill('0');

  for (std::size_t i = 0; i < size; ++i) {
    output << std::setw(2) << static_cast<int>(data[i]);
  }

  return output.str();
}
} // namespace

AccessToken TokenService::createAccessToken(const std::string &userId) {
  const auto &config = AppConfig::instance();

  const auto now = std::chrono::system_clock::now();

  const auto expiration =
      now + std::chrono::seconds(config.accessTokenLifetime());

  const auto token = jwt::create()
                         .set_issuer(config.jwtIssuer())
                         .set_subject(userId)
                         .set_issued_at(now)
                         .set_expires_at(expiration)
                         .sign(jwt::algorithm::hs256{config.jwtSecret()});

  return {token, config.accessTokenLifetime()};
}

std::string TokenService::createRefreshToken() {
  constexpr std::size_t tokenSize = 32;

  unsigned char bytes[tokenSize];

  if (RAND_bytes(bytes, tokenSize) != 1) {
    throw std::runtime_error("Unable to generate refresh token");
  }

  return toHex(bytes, tokenSize);
}

std::string TokenService::hashRefreshToken(const std::string &token) {
  unsigned char digest[EVP_MAX_MD_SIZE];

  unsigned int digestLength = 0;

  EVP_MD_CTX *context = EVP_MD_CTX_new();

  if (!context) {
    throw std::runtime_error("Unable to initialize SHA-256");
  }

  if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(context, token.data(), token.size()) != 1 ||
      EVP_DigestFinal_ex(context, digest, &digestLength) != 1) {
    EVP_MD_CTX_free(context);

    throw std::runtime_error("Unable to hash refresh token");
  }

  EVP_MD_CTX_free(context);

  return toHex(digest, digestLength);
}

std::string TokenService::verifyAccessToken(const std::string &token) {
  const auto &config = AppConfig::instance();

  try {
    auto decoded = jwt::decode(token);

    auto verifier =
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{config.jwtSecret()})
            .with_issuer(config.jwtIssuer());

    verifier.verify(decoded);

    return decoded.get_subject();
  } catch (...) {
    return {};
  }
}