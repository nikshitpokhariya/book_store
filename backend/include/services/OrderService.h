#pragma once

#include "models/Order.h"
#include "repositories/BookRepository.h"
#include "repositories/CartRepository.h"
#include "repositories/OrderRepository.h"
#include "repositories/UserRepository.h"

#include <string>

struct CheckoutRequest {
  std::string paymentMethod; // "UPI", "CARD", "COD"
  ShippingAddress shippingAddress;
  bool simulatePaymentFailure{false};
};

class OrderService {
public:
  OrderService(OrderRepository &orders, CartRepository &carts,
               BookRepository &books, UserRepository &users);

  Order checkout(const std::string &buyerId, const CheckoutRequest &request);

  PageResult<Order> getBuyerOrders(const std::string &buyerId, int page,
                                   int limit);

  PageResult<Order> getSellerSales(const std::string &sellerId, int page,
                                   int limit);

  Order getOrderDetails(const std::string &orderId, const std::string &userId);

  Order updateDeliveryStatus(const std::string &orderId,
                             const std::string &userId,
                             const std::string &newStatus,
                             const std::string &note);

  Order cancelOrder(const std::string &orderId, const std::string &userId,
                    const std::string &reason);

  Order requestReturn(const std::string &orderId, const std::string &userId,
                      const std::string &reason);

  Order processReturn(const std::string &orderId, const std::string &userId,
                      bool approve, const std::string &note);

private:
  OrderRepository &orders_;
  CartRepository &carts_;
  BookRepository &books_;
  UserRepository &users_;
};
