#include "services/BookService.h"

#include <algorithm>
#include <stdexcept>

namespace {

bool isValidCondition(const std::string &value) {
  return value == "NEW" || value == "LIKE_NEW" || value == "VERY_GOOD" ||
         value == "GOOD" || value == "ACCEPTABLE";
}

bool isValidListingType(const std::string &value) {
  return value == "EXCHANGE" || value == "SELL" || value == "BOTH" ||
         value == "GIVEAWAY";
}

bool isValidStatus(const std::string &value) {
  return value == "AVAILABLE" || value == "RESERVED" || value == "EXCHANGED" ||
         value == "SOLD";
}

} // namespace

BookService::BookService() = default;

void BookService::validate(const Book &book) const {
  if (book.title.empty()) {
    throw std::invalid_argument("Book title is required");
  }

  if (book.author.empty()) {
    throw std::invalid_argument("Book author is required");
  }

  if (book.genre.empty()) {
    throw std::invalid_argument("Book genre is required");
  }

  if (!isValidCondition(book.condition)) {
    throw std::invalid_argument("Invalid book condition");
  }

  if (!isValidListingType(book.listingType)) {
    throw std::invalid_argument("Invalid listing type");
  }

  if (!isValidStatus(book.status)) {
    throw std::invalid_argument("Invalid book status");
  }

  if (book.price.has_value() && *book.price < 0) {
    throw std::invalid_argument("Price cannot be negative");
  }

  const bool needsPrice =
      book.listingType == "SELL" || book.listingType == "BOTH";

  if (needsPrice && !book.price.has_value()) {
    throw std::invalid_argument("Price is required for SELL or BOTH listings");
  }

  const bool needsExchange =
      book.listingType == "EXCHANGE" || book.listingType == "BOTH";

  if (needsExchange && book.exchangePreferences.empty()) {
    throw std::invalid_argument("Exchange preferences are required");
  }

  if (!book.price.has_value() && !needsPrice) {
    // Correct: exchange/giveaway may have no price.
  }

  if (!book.price.has_value() && book.currency.empty()) {
    // No currency necessary.
  }

  if (book.price.has_value() && book.currency.empty()) {
    throw std::invalid_argument("Currency is required when price is specified");
  }
}

std::string BookService::createBook(const std::string &ownerId, Book book) {
  if (ownerId.empty()) {
    throw std::invalid_argument("Owner ID is required");
  }

  book.ownerId = ownerId;

  // Client must never decide the initial status.
  book.status = "AVAILABLE";

  const auto now = std::chrono::system_clock::now();

  book.createdAt = now;
  book.updatedAt = now;

  validate(book);

  return repository_.create(book);
}

std::optional<Book> BookService::getBook(const std::string &id) {
  if (id.empty()) {
    return std::nullopt;
  }

  return repository_.findById(id);
}

BookSearchResult BookService::searchBooks(
    const std::string &search, const std::string &genre,
    const std::string &condition, const std::string &listingType,
    const std::string &city, std::optional<double> minPrice,
    std::optional<double> maxPrice, const std::string &sort, int page,
    int limit) {
  if (page < 1)
    page = 1;

  if (limit < 1)
    limit = 20;

  if (limit > 100)
    limit = 100;

  if (minPrice.has_value() && maxPrice.has_value() && *minPrice > *maxPrice) {
    throw std::invalid_argument("minPrice cannot exceed maxPrice");
  }

  auto books = repository_.findAll(search, genre, condition, listingType, city,
                                   minPrice, maxPrice, sort, page, limit);

  auto total = repository_.count(search, genre, condition, listingType, city,
                                 minPrice, maxPrice);

  return {std::move(books), total, page, limit};
}

std::vector<Book> BookService::getMyBooks(const std::string &ownerId, int page,
                                          int limit) {
  if (page < 1)
    page = 1;

  if (limit < 1)
    limit = 20;

  if (limit > 100)
    limit = 100;

  return repository_.findByOwner(ownerId, page, limit);
}

bool BookService::updateBook(const std::string &bookId,
                             const std::string &ownerId, Book book) {
  auto existing = repository_.findById(bookId);

  if (!existing) {
    return false;
  }

  if (existing->ownerId != ownerId) {
    throw std::runtime_error("You are not allowed to modify this book");
  }

  // Never allow client to change ownership.
  book.ownerId = ownerId;
  book.id = bookId;

  // Preserve status unless you explicitly implement
  // status-management endpoints.
  book.status = existing->status;

  book.createdAt = existing->createdAt;
  book.updatedAt = std::chrono::system_clock::now();

  validate(book);

  return repository_.update(bookId, book);
}

bool BookService::deleteBook(const std::string &bookId,
                             const std::string &ownerId) {
  auto existing = repository_.findById(bookId);

  if (!existing) {
    return false;
  }

  if (existing->ownerId != ownerId) {
    throw std::runtime_error("You are not allowed to delete this book");
  }

  return repository_.remove(bookId);
}