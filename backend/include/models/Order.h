#pragma once

#include <chrono>
#include <string>
#include <vector>

struct OrderItemSnapshot {
  std::string bookId;
  std::string title;
  std::string author;
  std::string sellerId;
  std::string sellerUsername;
  double price{0.0};
  std::string condition;
  std::string category;
  std::string coverImage;
  int quantity{1};
};

struct ShippingAddress {
  std::string fullName;
  std::string addressLine;
  std::string city;
  std::string state;
  std::string postalCode;
  std::string phone;
};

struct PaymentInfo {
  std::string method;        // "UPI", "CARD", "COD"
  std::string status;        // "paid", "pending", "failed"
  std::string transactionId; // "DEMO_TXN_..."
  std::chrono::system_clock::time_point paidAt;
};

struct ShippingInfo {
  std::string trackingId; // "DEMO_TRACK_..."
  std::string status;     // "PLACED", "CONFIRMED", "PACKED", "SHIPPED", "OUT_FOR_DELIVERY", "DELIVERED"
  double cost{0.0};       // 0.0
  std::chrono::system_clock::time_point updatedAt;
};

struct OrderStatusHistory {
  std::string status;
  std::chrono::system_clock::time_point timestamp;
  std::string note;
};

struct Order {
  std::string id;
  std::string orderNumber;
  std::string buyerId;
  std::string buyerUsername;
  std::vector<OrderItemSnapshot> items;
  ShippingAddress shippingAddress;
  double subtotal{0.0};
  double shippingCost{0.0};
  double total{0.0};
  PaymentInfo payment;
  ShippingInfo shipping;
  std::string status{"PLACED"};
  std::chrono::system_clock::time_point createdAt;
  std::chrono::system_clock::time_point updatedAt;
  std::vector<OrderStatusHistory> history;
};
