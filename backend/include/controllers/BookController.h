#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class BookController : public drogon::HttpController<BookController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(BookController::create, "/api/books", Post, "AuthFilter");
  ADD_METHOD_TO(BookController::list, "/api/books", Get);
  ADD_METHOD_TO(BookController::myBooks, "/api/books/my", Get, "AuthFilter");
  ADD_METHOD_TO(BookController::get, "/api/books/{1}", Get);
  ADD_METHOD_TO(BookController::update, "/api/books/{1}", Patch, "AuthFilter");
  ADD_METHOD_TO(BookController::remove, "/api/books/{1}", Delete, "AuthFilter");

  METHOD_LIST_END

  void create(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);

  void list(const HttpRequestPtr &req,
            std::function<void(const HttpResponsePtr &)> &&callback);

  void get(const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback,
           std::string id);

  void myBooks(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback);

  void update(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback,
              std::string id);

  void remove(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback,
              std::string id);
};