#pragma once

#include <chrono>
#include <string>
#include <vector>

struct CartItem {
  std::string bookId;
  std::chrono::system_clock::time_point addedAt;
};

struct CartItemDetail {
  std::string bookId;
  std::string title;
  std::string author;
  double price{0.0};
  std::string condition;
  std::string category;
  std::string coverImage;
  std::string status;      // "available", "sold", "exchanged", "removed"
  bool isAvailable{false}; // true only if status == "available"
  std::string ownerId;
  std::string ownerUsername;
  std::chrono::system_clock::time_point addedAt;
};

struct Cart {
  std::string id;
  std::string userId;
  std::vector<CartItemDetail> items;
  double subtotal{0.0};
  double shippingCost{0.0}; // Always 0.0
  double total{0.0};
  bool hasUnavailableItems{false};
  std::chrono::system_clock::time_point updatedAt;
};
