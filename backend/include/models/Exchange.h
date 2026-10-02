#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

struct ExchangeBookSnapshot {
  std::string bookId;
  std::string title;
  std::string author;
  std::string condition;
  std::string category;
  double price{0.0};
  std::string coverImage;
  std::string originalOwnerId;
  std::string originalOwnerUsername;
};

struct ExchangeRequest {
  std::string id;
  std::string exchangeNumber;
  std::string requesterId;
  std::string requesterUsername;
  std::string receiverId;
  std::string receiverUsername;
  ExchangeBookSnapshot requestedBook;
  ExchangeBookSnapshot offeredBook;
  std::string status{"PENDING"}; // "PENDING", "COMPLETED", "REJECTED", "CANCELLED"
  std::string message;
  std::string statusNote;
  std::chrono::system_clock::time_point createdAt;
  std::chrono::system_clock::time_point updatedAt;
  std::optional<std::chrono::system_clock::time_point> completedAt;
};
