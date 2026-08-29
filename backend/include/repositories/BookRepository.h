#pragma once

#include "models/Book.h"

#include <optional>
#include <string>
#include <vector>

class BookRepository {
public:
  BookRepository() = default;

  std::string create(const Book &book);

  std::optional<Book> findById(const std::string &id);

  std::vector<Book> findAll(const std::string &search, const std::string &genre,
                            const std::string &condition,
                            const std::string &listingType,
                            const std::string &city,
                            std::optional<double> minPrice,
                            std::optional<double> maxPrice,
                            const std::string &sort, int page, int limit);

  std::vector<Book> findByOwner(const std::string &ownerId, int page,
                                int limit);

  bool update(const std::string &id, const Book &book);

  bool remove(const std::string &id);

  long long count(const std::string &search, const std::string &genre,
                  const std::string &condition, const std::string &listingType,
                  const std::string &city, std::optional<double> minPrice,
                  std::optional<double> maxPrice);
};