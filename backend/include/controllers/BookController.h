#pragma once

#include <drogon/HttpController.h>

class BookController : public drogon::HttpController<BookController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(BookController::createListing, "/api/books", drogon::Post,
                "AuthFilter");

  ADD_METHOD_TO(BookController::browse, "/api/books", drogon::Get);

  ADD_METHOD_TO(BookController::getMyListings, "/api/books/my", drogon::Get,
                "AuthFilter");

  ADD_METHOD_TO(BookController::getById, "/api/books/{1}", drogon::Get);

  ADD_METHOD_TO(BookController::updateListing, "/api/books/{1}", drogon::Put,
                "AuthFilter");

  ADD_METHOD_TO(BookController::deleteListing, "/api/books/{1}",
                drogon::Delete, "AuthFilter");

  METHOD_LIST_END

  void createListing(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void browse(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getMyListings(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getById(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback,
               std::string bookId);

  void
  updateListing(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                std::string bookId);

  void
  deleteListing(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                std::string bookId);
};
