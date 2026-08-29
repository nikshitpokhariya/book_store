#include "controllers/BookController.h"
#include "services/BookService.h"

#include <drogon/drogon.h>

#include <iomanip>
#include <optional>
#include <sstream>

using namespace drogon;

namespace {

std::string timePointToIso8601(const std::chrono::system_clock::time_point &tp) {
  auto time = std::chrono::system_clock::to_time_t(tp);
  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm, &time);
#else
  gmtime_r(&time, &tm);
#endif
  std::ostringstream ss;
  ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return ss.str();
}

Json::Value bookToJson(const Book &book) {
  Json::Value json;

  json["id"] = book.id;
  json["ownerId"] = book.ownerId;

  json["title"] = book.title;
  json["author"] = book.author;
  json["isbn"] = book.isbn;
  json["description"] = book.description;

  json["genre"] = book.genre;
  json["language"] = book.language;
  json["condition"] = book.condition;

  json["listingType"] = book.listingType;

  if (book.price.has_value()) {
    json["price"] = *book.price;
  } else {
    json["price"] = Json::nullValue;
  }

  json["currency"] = book.currency;
  json["exchangePreferences"] = book.exchangePreferences;

  Json::Value images(Json::arrayValue);
  for (const auto &image : book.images) {
    images.append(image);
  }
  json["images"] = images;

  json["city"] = book.city;
  json["status"] = book.status;

  if (book.createdAt != std::chrono::system_clock::time_point{}) {
    json["createdAt"] = timePointToIso8601(book.createdAt);
  }
  if (book.updatedAt != std::chrono::system_clock::time_point{}) {
    json["updatedAt"] = timePointToIso8601(book.updatedAt);
  }

  return json;
}

Book jsonToBook(const Json::Value &json) {
  Book book;

  if (json.isMember("title") && json["title"].isString())
    book.title = json["title"].asString();

  if (json.isMember("author") && json["author"].isString())
    book.author = json["author"].asString();

  if (json.isMember("isbn") && json["isbn"].isString())
    book.isbn = json["isbn"].asString();

  if (json.isMember("description") && json["description"].isString())
    book.description = json["description"].asString();

  if (json.isMember("genre") && json["genre"].isString())
    book.genre = json["genre"].asString();

  if (json.isMember("language") && json["language"].isString())
    book.language = json["language"].asString();

  if (json.isMember("condition") && json["condition"].isString())
    book.condition = json["condition"].asString();

  if (json.isMember("listingType") && json["listingType"].isString())
    book.listingType = json["listingType"].asString();

  if (json.isMember("price") && !json["price"].isNull() && json["price"].isNumeric()) {
    book.price = json["price"].asDouble();
  }

  if (json.isMember("currency") && json["currency"].isString())
    book.currency = json["currency"].asString();

  if (json.isMember("exchangePreferences") && json["exchangePreferences"].isString())
    book.exchangePreferences = json["exchangePreferences"].asString();

  if (json.isMember("city") && json["city"].isString())
    book.city = json["city"].asString();

  if (json.isMember("images") && json["images"].isArray()) {
    for (const auto &image : json["images"]) {
      if (image.isString()) {
        book.images.push_back(image.asString());
      }
    }
  }

  return book;
}

int getIntQuery(const HttpRequestPtr &req, const std::string &name,
                int defaultValue) {
  auto value = req->getParameter(name);

  if (value.empty())
    return defaultValue;

  try {
    return std::stoi(value);
  } catch (...) {
    return defaultValue;
  }
}

std::optional<double> getDoubleQuery(const HttpRequestPtr &req,
                                     const std::string &name) {
  auto value = req->getParameter(name);

  if (value.empty())
    return std::nullopt;

  try {
    return std::stod(value);
  } catch (...) {
    return std::nullopt;
  }
}

struct BookControllerRegistrar {
  BookControllerRegistrar() {
    BookController::initPathRouting();
  }
} bookRegistrar;

} // namespace

void BookController::create(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    const auto userId = req->attributes()->get<std::string>("userId");

    auto json = req->getJsonObject();
    if (!json) {
      Json::Value body;
      body["error"] = "Invalid JSON body";
      auto response = HttpResponse::newHttpJsonResponse(body);
      response->setStatusCode(k400BadRequest);
      callback(response);
      return;
    }

    Book book = jsonToBook(*json);

    BookService service;
    const std::string id = service.createBook(userId, std::move(book));

    Json::Value body;
    body["success"] = true;
    body["id"] = id;
    body["message"] = "Book created successfully";

    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k201Created);
    callback(response);
  } catch (const std::invalid_argument &e) {
    Json::Value body;
    body["error"] = e.what();
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k400BadRequest);
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << e.what();
    Json::Value body;
    body["error"] = "Internal server error";
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k500InternalServerError);
    callback(response);
  }
}

void BookController::list(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    const auto search = req->getParameter("search");
    const auto genre = req->getParameter("genre");
    const auto condition = req->getParameter("condition");
    const auto listingType = req->getParameter("listingType");
    const auto city = req->getParameter("city");
    const auto sort = req->getParameter("sort");

    const int page = getIntQuery(req, "page", 1);
    const int limit = getIntQuery(req, "limit", 20);

    auto minPrice = getDoubleQuery(req, "minPrice");
    auto maxPrice = getDoubleQuery(req, "maxPrice");

    BookService service;

    auto result =
        service.searchBooks(search, genre, condition, listingType, city,
                            minPrice, maxPrice, sort, page, limit);

    Json::Value books(Json::arrayValue);
    for (const auto &book : result.books) {
      books.append(bookToJson(book));
    }

    Json::Value responseJson;
    responseJson["books"] = books;
    responseJson["page"] = result.page;
    responseJson["limit"] = result.limit;
    responseJson["total"] = Json::Int64(result.total);

    auto response = HttpResponse::newHttpJsonResponse(responseJson);
    callback(response);
  } catch (const std::invalid_argument &e) {
    Json::Value body;
    body["error"] = e.what();
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k400BadRequest);
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << e.what();
    Json::Value body;
    body["error"] = "Internal server error";
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k500InternalServerError);
    callback(response);
  }
}

void BookController::get(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string id) {
  try {
    BookService service;
    auto book = service.getBook(id);

    if (!book) {
      Json::Value body;
      body["error"] = "Book not found";
      auto response = HttpResponse::newHttpJsonResponse(body);
      response->setStatusCode(k404NotFound);
      callback(response);
      return;
    }

    auto response = HttpResponse::newHttpJsonResponse(bookToJson(*book));
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << e.what();
    Json::Value body;
    body["error"] = "Internal server error";
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k500InternalServerError);
    callback(response);
  }
}

void BookController::myBooks(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  try {
    const auto userId = req->attributes()->get<std::string>("userId");

    const int page = getIntQuery(req, "page", 1);
    const int limit = getIntQuery(req, "limit", 20);

    BookService service;
    auto books = service.getMyBooks(userId, page, limit);

    Json::Value responseJson;
    Json::Value booksJson(Json::arrayValue);
    for (const auto &book : books) {
      booksJson.append(bookToJson(book));
    }

    responseJson["books"] = booksJson;
    responseJson["page"] = page;
    responseJson["limit"] = limit;

    callback(HttpResponse::newHttpJsonResponse(responseJson));
  } catch (const std::exception &e) {
    LOG_ERROR << e.what();
    Json::Value body;
    body["error"] = "Internal server error";
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k500InternalServerError);
    callback(response);
  }
}

void BookController::update(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string id) {
  try {
    const auto userId = req->attributes()->get<std::string>("userId");

    auto json = req->getJsonObject();
    if (!json) {
      Json::Value body;
      body["error"] = "Invalid JSON body";
      auto response = HttpResponse::newHttpJsonResponse(body);
      response->setStatusCode(k400BadRequest);
      callback(response);
      return;
    }

    Book book = jsonToBook(*json);

    BookService service;
    const bool updated = service.updateBook(id, userId, std::move(book));

    if (!updated) {
      Json::Value body;
      body["error"] = "Book not found or not modified";
      auto response = HttpResponse::newHttpJsonResponse(body);
      response->setStatusCode(k404NotFound);
      callback(response);
      return;
    }

    Json::Value body;
    body["message"] = "Book updated successfully";
    callback(HttpResponse::newHttpJsonResponse(body));
  } catch (const std::runtime_error &e) {
    Json::Value body;
    body["error"] = e.what();
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k403Forbidden);
    callback(response);
  } catch (const std::invalid_argument &e) {
    Json::Value body;
    body["error"] = e.what();
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k400BadRequest);
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << e.what();
    Json::Value body;
    body["error"] = "Internal server error";
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k500InternalServerError);
    callback(response);
  }
}

void BookController::remove(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string id) {
  try {
    const auto userId = req->attributes()->get<std::string>("userId");

    BookService service;
    const bool deleted = service.deleteBook(id, userId);

    if (!deleted) {
      Json::Value body;
      body["error"] = "Book not found";
      auto response = HttpResponse::newHttpJsonResponse(body);
      response->setStatusCode(k404NotFound);
      callback(response);
      return;
    }

    Json::Value body;
    body["message"] = "Book deleted successfully";
    callback(HttpResponse::newHttpJsonResponse(body));
  } catch (const std::runtime_error &e) {
    Json::Value body;
    body["error"] = e.what();
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k403Forbidden);
    callback(response);
  } catch (const std::exception &e) {
    LOG_ERROR << e.what();
    Json::Value body;
    body["error"] = "Internal server error";
    auto response = HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(k500InternalServerError);
    callback(response);
  }
}