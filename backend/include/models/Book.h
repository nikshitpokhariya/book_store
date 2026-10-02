#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct BookOwnerInfo {
  std::string id;
  std::string username;
};

struct Book {
  std::string id;
  std::string ownerId;
  std::string title;
  std::string author;
  std::string isbn;
  std::string description;
  std::string category;
  double price{0.0};
  std::string condition;
  std::string coverImage;
  std::string status{"available"};
  double averageRating{0.0};
  int reviewCount{0};
  std::chrono::system_clock::time_point createdAt;
  std::chrono::system_clock::time_point updatedAt;
  std::optional<BookOwnerInfo> owner;
};

struct BookFilter {
  std::optional<std::string> search;
  std::optional<std::string> category;
  std::optional<std::string> condition;
  std::optional<double> minPrice;
  std::optional<double> maxPrice;
  std::string sort{"newest"};
};

struct Pagination {
  int page{1};
  int limit{10};
};

template <typename T> struct PageResult {
  std::vector<T> items;
  std::int64_t total{0};
  int page{1};
  int limit{10};
  int totalPages{0};
  bool hasNextPage{false};
  bool hasPrevPage{false};
};

struct BookUpdate {
  std::optional<std::string> title;
  std::optional<std::string> author;
  std::optional<std::string> isbn;
  std::optional<std::string> description;
  std::optional<std::string> category;
  std::optional<double> price;
  std::optional<std::string> condition;
  std::optional<std::string> coverImage;
};
