#pragma once

#include <drogon/HttpController.h>

class ReviewController : public drogon::HttpController<ReviewController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(ReviewController::createReview, "/api/reviews", drogon::Post,
                "AuthFilter");

  ADD_METHOD_TO(ReviewController::getMyReviews, "/api/reviews/my", drogon::Get,
                "AuthFilter");

  ADD_METHOD_TO(ReviewController::getBookReviews, "/api/books/{1}/reviews",
                drogon::Get);

  ADD_METHOD_TO(ReviewController::updateReview, "/api/reviews/{1}",
                drogon::Put, "AuthFilter");

  ADD_METHOD_TO(ReviewController::deleteReview, "/api/reviews/{1}",
                drogon::Delete, "AuthFilter");

  METHOD_LIST_END

  void createReview(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getBookReviews(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string bookId);

  void getMyReviews(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void updateReview(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string reviewId);

  void deleteReview(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      std::string reviewId);
};
