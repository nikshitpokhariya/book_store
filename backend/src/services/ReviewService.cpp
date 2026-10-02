#include "services/ReviewService.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {
std::string trim(const std::string &str) {
  const auto start = std::find_if_not(
      str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });
  const auto end =
      std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
        return std::isspace(c);
      }).base();
  return (start < end) ? std::string(start, end) : std::string{};
}
} // namespace

ReviewService::ReviewService(ReviewRepository &reviews, OrderRepository &orders,
                             BookRepository &books, UserRepository &users)
    : reviews_(reviews), orders_(orders), books_(books), users_(users) {}

Review ReviewService::createReview(const std::string &userId,
                                   const std::string &orderId,
                                   const std::string &bookId, int rating,
                                   const std::string &comment) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  if (rating < 1 || rating > 5) {
    throw std::invalid_argument("Rating must be an integer between 1 and 5");
  }

  const std::string trimmedComment = trim(comment);
  if (trimmedComment.empty()) {
    throw std::invalid_argument("Review comment cannot be empty");
  }
  if (trimmedComment.size() > 1000) {
    throw std::invalid_argument(
        "Review comment cannot exceed 1000 characters");
  }

  auto user = users_.findById(userId);
  if (!user) {
    throw std::runtime_error("USER_NOT_FOUND");
  }

  auto order = orders_.findById(orderId);
  if (!order) {
    throw std::runtime_error("ORDER_NOT_FOUND");
  }

  // Verification 1: Order must belong to this buyer
  if (order->buyerId != userId) {
    throw std::runtime_error(
        "FORBIDDEN: You can only review books from your own orders");
  }

  // Verification 2: Order must be DELIVERED
  if (order->status != "DELIVERED") {
    throw std::invalid_argument(
        "You can only review books after your order has been DELIVERED (current: " +
        order->status + ")");
  }

  // Verification 3: Book must be part of this order
  bool bookFound = false;
  std::string sellerId;
  for (const auto &item : order->items) {
    if (item.bookId == bookId) {
      bookFound = true;
      sellerId = item.sellerId;
      break;
    }
  }

  if (!bookFound) {
    throw std::invalid_argument(
        "The specified book was not found in this order");
  }

  // Verification 4: Prevent self-review (seller cannot review own book)
  if (sellerId == userId) {
    throw std::invalid_argument("Sellers cannot review their own book listings");
  }

  // Verification 5: Prevent duplicate reviews for this purchase
  auto existing = reviews_.findByOrderAndBook(orderId, bookId);
  if (existing) {
    throw std::runtime_error(
        "DUPLICATE_REVIEW: You have already submitted a review for this purchase");
  }

  Review review;
  review.bookId = bookId;
  review.orderId = orderId;
  review.reviewerId = userId;
  review.reviewerUsername = user->username;
  review.rating = rating;
  review.comment = trimmedComment;

  review.id = reviews_.create(review);

  // Synchronize aggregate rating on the book
  syncBookRating(bookId);

  return review;
}

PageResult<Review> ReviewService::getBookReviews(const std::string &bookId,
                                                int page, int limit) {
  page = std::max(1, page);
  limit = std::clamp(limit, 1, 50);
  return reviews_.findByBookId(bookId, page, limit);
}

PageResult<Review> ReviewService::getMyReviews(const std::string &userId,
                                              int page, int limit) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }
  page = std::max(1, page);
  limit = std::clamp(limit, 1, 50);
  return reviews_.findByReviewerId(userId, page, limit);
}

Review ReviewService::updateReview(const std::string &reviewId,
                                   const std::string &userId, int rating,
                                   const std::string &comment) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  if (rating < 1 || rating > 5) {
    throw std::invalid_argument("Rating must be an integer between 1 and 5");
  }

  const std::string trimmedComment = trim(comment);
  if (trimmedComment.empty()) {
    throw std::invalid_argument("Review comment cannot be empty");
  }
  if (trimmedComment.size() > 1000) {
    throw std::invalid_argument(
        "Review comment cannot exceed 1000 characters");
  }

  auto review = reviews_.findById(reviewId);
  if (!review) {
    throw std::runtime_error("REVIEW_NOT_FOUND");
  }

  if (review->reviewerId != userId) {
    throw std::runtime_error(
        "FORBIDDEN: You do not have permission to edit this review");
  }

  const bool updated = reviews_.update(reviewId, rating, trimmedComment);
  if (!updated) {
    throw std::runtime_error("REVIEW_UPDATE_FAILED");
  }

  syncBookRating(review->bookId);

  auto refreshed = reviews_.findById(reviewId);
  if (!refreshed) {
    throw std::runtime_error("REVIEW_NOT_FOUND");
  }

  return *refreshed;
}

void ReviewService::deleteReview(const std::string &reviewId,
                                 const std::string &userId) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  auto review = reviews_.findById(reviewId);
  if (!review) {
    throw std::runtime_error("REVIEW_NOT_FOUND");
  }

  if (review->reviewerId != userId) {
    throw std::runtime_error(
        "FORBIDDEN: You do not have permission to delete this review");
  }

  const std::string bookId = review->bookId;
  const bool deleted = reviews_.deleteById(reviewId);
  if (!deleted) {
    throw std::runtime_error("REVIEW_DELETE_FAILED");
  }

  syncBookRating(bookId);
}

void ReviewService::syncBookRating(const std::string &bookId) {
  const auto [avg, count] = reviews_.calculateRatingAggregate(bookId);
  books_.updateRating(bookId, avg, count);
}
