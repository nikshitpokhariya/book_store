#pragma once

#include "models/Book.h"
#include "models/Review.h"
#include "repositories/BookRepository.h"
#include "repositories/OrderRepository.h"
#include "repositories/ReviewRepository.h"
#include "repositories/UserRepository.h"

#include <string>

class ReviewService {
public:
  ReviewService(ReviewRepository &reviews, OrderRepository &orders,
                BookRepository &books, UserRepository &users);

  Review createReview(const std::string &userId, const std::string &orderId,
                      const std::string &bookId, int rating,
                      const std::string &comment);

  PageResult<Review> getBookReviews(const std::string &bookId, int page,
                                   int limit);

  PageResult<Review> getMyReviews(const std::string &userId, int page,
                                  int limit);

  Review updateReview(const std::string &reviewId, const std::string &userId,
                      int rating, const std::string &comment);

  void deleteReview(const std::string &reviewId, const std::string &userId);

private:
  ReviewRepository &reviews_;
  OrderRepository &orders_;
  BookRepository &books_;
  UserRepository &users_;

  void syncBookRating(const std::string &bookId);
};
