#include "security/PaymentService.hpp"

#include <openssl/rand.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
std::string generateHexId(std::size_t numBytes) {
  std::vector<unsigned char> bytes(numBytes);
  if (RAND_bytes(bytes.data(), static_cast<int>(numBytes)) != 1) {
    throw std::runtime_error("Unable to generate random bytes for payment");
  }

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char b : bytes) {
    oss << std::setw(2) << static_cast<int>(b);
  }
  return oss.str();
}

std::string normalizeMethod(std::string method) {
  std::transform(method.begin(), method.end(), method.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return method;
}
} // namespace

PaymentResult PaymentService::processPayment(const std::string &paymentMethod,
                                             double amount,
                                             bool simulateFailure) {
  PaymentResult result;
  result.paymentMethod = normalizeMethod(paymentMethod);

  if (result.paymentMethod != "UPI" && result.paymentMethod != "CARD" &&
      result.paymentMethod != "COD") {
    result.success = false;
    result.paymentStatus = "failed";
    result.message =
        "Unsupported payment method. Supported methods: UPI, CARD, COD";
    return result;
  }

  if (amount < 0.0) {
    result.success = false;
    result.paymentStatus = "failed";
    result.message = "Invalid payment amount";
    return result;
  }

  result.transactionId = "DEMO_TXN_" + generateHexId(12);

  if (simulateFailure) {
    result.success = false;
    result.paymentStatus = "failed";
    result.message = "Demo payment simulation rejected by user request";
    return result;
  }

  if (result.paymentMethod == "COD") {
    result.success = true;
    result.paymentStatus = "pending";
    result.message = "Cash on delivery selected. Payment due upon arrival.";
  } else {
    result.success = true;
    result.paymentStatus = "paid";
    result.message = "Simulated demo payment completed successfully.";
  }

  return result;
}
