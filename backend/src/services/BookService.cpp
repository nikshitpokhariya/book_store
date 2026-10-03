#include "services/BookService.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace {
std::string trim(const std::string &str) {
  const auto start = str.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) {
    return "";
  }
  const auto end = str.find_last_not_of(" \t\n\r");
  return str.substr(start, end - start + 1);
}

std::string normalizeCondition(std::string condition) {
  condition = trim(condition);
  std::string lower = condition;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  if (lower == "new")
    return "New";
  if (lower == "like new" || lower == "likenew")
    return "Like New";
  if (lower == "very good" || lower == "verygood")
    return "Very Good";
  if (lower == "good")
    return "Good";
  if (lower == "acceptable")
    return "Acceptable";

  throw std::invalid_argument(
      "Invalid condition. Allowed: New, Like New, Very Good, Good, Acceptable");
}

bool isValidIsbn(const std::string &isbn) {
  if (isbn.empty())
    return true;
  if (isbn.size() < 9 || isbn.size() > 20)
    return false;

  for (char c : isbn) {
    if (!std::isdigit(static_cast<unsigned char>(c)) && c != '-' && c != 'X' &&
        c != 'x' && c != ' ') {
      return false;
    }
  }
  return true;
}

void validatePrice(double price) {
  if (std::isnan(price) || std::isinf(price)) {
    throw std::invalid_argument("Price must be a valid number");
  }
  if (price < 0.0) {
    throw std::invalid_argument("Price cannot be negative");
  }
  if (price > 1000000.0) {
    throw std::invalid_argument("Price exceeds maximum allowed value");
  }
}

void validateBookInput(const Book &book) {
  if (book.title.empty() || book.title.size() > 200) {
    throw std::invalid_argument("Title must be between 1 and 200 characters");
  }
  if (book.author.empty() || book.author.size() > 100) {
    throw std::invalid_argument("Author must be between 1 and 100 characters");
  }
  if (book.category.empty() || book.category.size() > 50) {
    throw std::invalid_argument("Category must be between 1 and 50 characters");
  }
  if (book.description.size() > 2000) {
    throw std::invalid_argument(
        "Description cannot exceed 2000 characters");
  }
  if (!isValidIsbn(book.isbn)) {
    throw std::invalid_argument("Invalid ISBN format");
  }
  if (book.coverImage.size() > 500) {
    throw std::invalid_argument("Cover image URL cannot exceed 500 characters");
  }
  validatePrice(book.price);
}
} // namespace

BookService::BookService(BookRepository &books, UserRepository &users)
    : books_(books), users_(users) {}

void BookService::enrichWithOwner(Book &book) {
  if (book.ownerId.empty()) {
    return;
  }
  auto user = users_.findById(book.ownerId);
  if (user) {
    BookOwnerInfo owner;
    owner.id = user->id;
    owner.username = user->username;
    book.owner = owner;
  }
}

Book BookService::createListing(const std::string &ownerId, Book bookInput) {
  if (ownerId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  bookInput.title = trim(bookInput.title);
  bookInput.author = trim(bookInput.author);
  bookInput.category = trim(bookInput.category);
  bookInput.description = trim(bookInput.description);
  bookInput.isbn = trim(bookInput.isbn);
  bookInput.coverImage = trim(bookInput.coverImage);
  bookInput.condition = normalizeCondition(bookInput.condition);
  bookInput.ownerId = ownerId;
  bookInput.status = "available";

  if (books_.existsByOwnerAndTitle(ownerId, bookInput.title)) {
    throw std::invalid_argument("You have already listed a book with this title");
  }

  if (bookInput.images.size() > 6) {
    throw std::invalid_argument("Maximum 6 images allowed");
  }
  if (!bookInput.images.empty() && bookInput.images.size() < 4) {
    throw std::invalid_argument("Minimum 4 images required");
  }
  if (bookInput.coverImage.empty() && !bookInput.images.empty()) {
    bookInput.coverImage = bookInput.images.front();
  }
  if (bookInput.images.empty() && !bookInput.coverImage.empty()) {
    bookInput.images.push_back(bookInput.coverImage);
  }

  validateBookInput(bookInput);

  bookInput.id = books_.create(bookInput);
  enrichWithOwner(bookInput);
  return bookInput;
}

std::optional<Book> BookService::getPublicBook(const std::string &id) {
  auto book = books_.findById(id);
  if (!book || book->status != "available") {
    return std::nullopt;
  }
  enrichWithOwner(*book);
  return book;
}

Book BookService::getBookForUser(const std::string &id,
                                 const std::optional<std::string> &userId) {
  auto book = books_.findById(id);
  if (!book) {
    throw std::runtime_error("BOOK_NOT_FOUND");
  }

  // If available, anyone can see it
  if (book->status == "available") {
    enrichWithOwner(*book);
    return *book;
  }

  // If unavailable, only the owner can view historical listing
  if (userId && *userId == book->ownerId) {
    enrichWithOwner(*book);
    return *book;
  }

  throw std::runtime_error("BOOK_NOT_FOUND");
}

PageResult<Book> BookService::browseBooks(BookFilter filter,
                                         Pagination pagination) {
  pagination.page = std::max(1, pagination.page);
  pagination.limit = std::clamp(pagination.limit, 1, 50);

  if (filter.search) {
    *filter.search = trim(*filter.search);
  }
  if (filter.category) {
    *filter.category = trim(*filter.category);
  }
  if (filter.condition) {
    *filter.condition = trim(*filter.condition);
  }
  if (filter.minPrice && *filter.minPrice < 0.0) {
    filter.minPrice = 0.0;
  }
  if (filter.maxPrice && *filter.maxPrice < 0.0) {
    filter.maxPrice = 0.0;
  }

  static const std::unordered_set<std::string> allowedSorts = {
      "newest", "oldest", "price_asc", "price_desc"};
  if (allowedSorts.find(filter.sort) == allowedSorts.end()) {
    filter.sort = "newest";
  }

  auto result = books_.findPublic(filter, pagination);
  for (auto &item : result.items) {
    enrichWithOwner(item);
  }
  return result;
}

PageResult<Book>
BookService::getMyListings(const std::string &ownerId,
                          const std::optional<std::string> &status,
                          Pagination pagination) {
  if (ownerId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  pagination.page = std::max(1, pagination.page);
  pagination.limit = std::clamp(pagination.limit, 1, 50);

  auto result = books_.findByOwner(ownerId, status, pagination);
  for (auto &item : result.items) {
    enrichWithOwner(item);
  }
  return result;
}

Book BookService::updateListing(const std::string &id, const std::string &userId,
                                const BookUpdate &update) {
  auto book = books_.findById(id);
  if (!book) {
    throw std::runtime_error("BOOK_NOT_FOUND");
  }

  if (book->ownerId != userId) {
    throw std::runtime_error("FORBIDDEN");
  }

  if (book->status == "sold" || book->status == "exchanged") {
    throw std::runtime_error("CANNOT_MODIFY_CLOSED_LISTING");
  }

  if (book->status == "removed") {
    throw std::runtime_error("CANNOT_MODIFY_REMOVED_LISTING");
  }

  BookUpdate sanitized = update;
  if (sanitized.title) {
    *sanitized.title = trim(*sanitized.title);
    if (sanitized.title->empty() || sanitized.title->size() > 200) {
      throw std::invalid_argument("Title must be between 1 and 200 characters");
    }
  }
  if (sanitized.author) {
    *sanitized.author = trim(*sanitized.author);
    if (sanitized.author->empty() || sanitized.author->size() > 100) {
      throw std::invalid_argument("Author must be between 1 and 100 characters");
    }
  }
  if (sanitized.category) {
    *sanitized.category = trim(*sanitized.category);
    if (sanitized.category->empty() || sanitized.category->size() > 50) {
      throw std::invalid_argument(
          "Category must be between 1 and 50 characters");
    }
  }
  if (sanitized.description) {
    *sanitized.description = trim(*sanitized.description);
    if (sanitized.description->size() > 2000) {
      throw std::invalid_argument(
          "Description cannot exceed 2000 characters");
    }
  }
  if (sanitized.isbn) {
    *sanitized.isbn = trim(*sanitized.isbn);
    if (!isValidIsbn(*sanitized.isbn)) {
      throw std::invalid_argument("Invalid ISBN format");
    }
  }
  if (sanitized.coverImage) {
    *sanitized.coverImage = trim(*sanitized.coverImage);
    if (sanitized.coverImage->size() > 500) {
      throw std::invalid_argument(
          "Cover image URL cannot exceed 500 characters");
    }
  }
  if (sanitized.condition) {
    *sanitized.condition = normalizeCondition(*sanitized.condition);
  }
  if (sanitized.price) {
    validatePrice(*sanitized.price);
  }

  books_.update(id, sanitized);

  auto updatedBook = books_.findById(id);
  if (!updatedBook) {
    throw std::runtime_error("BOOK_NOT_FOUND");
  }
  enrichWithOwner(*updatedBook);
  return *updatedBook;
}

void BookService::removeListing(const std::string &id,
                                const std::string &userId) {
  auto book = books_.findById(id);
  if (!book) {
    throw std::runtime_error("BOOK_NOT_FOUND");
  }

  if (book->ownerId != userId) {
    throw std::runtime_error("FORBIDDEN");
  }

  if (book->status == "sold" || book->status == "exchanged") {
    throw std::runtime_error("CANNOT_DELETE_CLOSED_LISTING");
  }

  if (book->status == "removed") {
    return; // Already removed
  }

  books_.updateStatus(id, "removed");
}

std::vector<std::string> BookService::getSuggestions(const std::string &query, int limit) {
  const auto trimmed = trim(query);
  if (trimmed.empty()) {
    return {};
  }
  return books_.findSuggestions(trimmed, limit);
}
