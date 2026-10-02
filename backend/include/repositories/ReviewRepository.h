#pragma once

#include "models/Book.h" // For PageResult
#include "models/Review.h"

#include <optional>
#include <string>
#include <utility>

class ReviewRepository {
public:
  std::string create(const Review &review);

  std::optional<Review> findById(const std::string &id);

  std::optional<Review> findByOrderAndBook(const std::string &orderId,
                                          const std::string &bookId);

  PageResult<Review> findByBookId(const std::string &bookId, int page,
                                 int limit);

  PageResult<Review> findByReviewerId(const std::string &reviewerId, int page,
                                     int limit);

  bool update(const std::string &id, int rating, const std::string &comment);

  bool deleteById(const std::string &id);

  std::pair<double, int> calculateRatingAggregate(const std::string &bookId);
};
