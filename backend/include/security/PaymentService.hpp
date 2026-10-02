#pragma once

#include <string>

struct PaymentResult {
  bool success{false};
  std::string transactionId;
  std::string paymentStatus; // "paid", "pending", "failed"
  std::string paymentMethod; // "UPI", "CARD", "COD"
  std::string message;
};

class PaymentService {
public:
  static PaymentResult processPayment(const std::string &paymentMethod,
                                      double amount,
                                      bool simulateFailure = false);
};
