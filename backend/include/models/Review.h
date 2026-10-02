#pragma once

#include <chrono>
#include <string>

struct Review {
  std::string id;
  std::string bookId;
  std::string orderId;
  std::string reviewerId;
  std::string reviewerUsername;
  int rating{5}; // 1 - 5
  std::string comment;
  std::chrono::system_clock::time_point createdAt;
  std::chrono::system_clock::time_point updatedAt;
};
