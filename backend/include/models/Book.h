#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

struct Book {
  std::string id;
  std::string ownerId;

  std::string title;
  std::string author;
  std::string isbn;
  std::string description;

  std::string genre;
  std::string language;
  std::string condition;

  // EXCHANGE / SELL / BOTH / GIVEAWAY
  std::string listingType;

  std::optional<double> price;
  std::string currency;

  std::string exchangePreferences;

  std::vector<std::string> images;

  std::string city;

  // AVAILABLE / RESERVED / EXCHANGED / SOLD
  std::string status;

  std::chrono::system_clock::time_point createdAt;
  std::chrono::system_clock::time_point updatedAt;
};