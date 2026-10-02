#pragma once

#include "models/Book.h"
#include "models/Order.h"

#include <optional>
#include <string>
#include <vector>

class OrderRepository {
public:
  std::string create(const Order &order);

  std::optional<Order> findById(const std::string &orderId);

  std::optional<Order> findByOrderNumber(const std::string &orderNumber);

  PageResult<Order> findByBuyer(const std::string &buyerId, int page,
                                int limit);

  PageResult<Order> findBySeller(const std::string &sellerId, int page,
                                 int limit);

  bool updateStatus(const std::string &orderId, const std::string &newStatus,
                    const std::string &note);

  bool updateOrderStatusAndPayment(const std::string &orderId,
                                   const std::string &newStatus,
                                   const std::string &paymentStatus,
                                   const std::string &note);
};
