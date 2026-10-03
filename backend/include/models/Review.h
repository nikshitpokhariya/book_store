#pragma once

#include <chrono>
#include <string>
#include <vector>

struct Review {
  std::string id;
  std::string bookId;
  std::string orderId;
  std::string sellerId;
  std::string sellerUsername;
  std::string reviewerId;
  std::string reviewerUsername;
  int rating{5}; // 1 - 5
  std::string comment;
  std::vector<std::string> images; // photos of received book quality
  std::chrono::system_clock::time_point createdAt;
  std::chrono::system_clock::time_point updatedAt;
};

