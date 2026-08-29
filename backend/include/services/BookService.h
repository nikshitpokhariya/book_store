#pragma once

#include "models/Book.h"
#include "repositories/BookRepository.h"

#include <optional>
#include <string>
#include <vector>

struct BookSearchResult {
  std::vector<Book> books;
  long long total;
  int page;
  int limit;
};

class BookService {
public:
  BookService();

  std::string createBook(const std::string &ownerId, Book book);

  std::optional<Book> getBook(const std::string &id);

  BookSearchResult
  searchBooks(const std::string &search, const std::string &genre,
              const std::string &condition, const std::string &listingType,
              const std::string &city, std::optional<double> minPrice,
              std::optional<double> maxPrice, const std::string &sort, int page,
              int limit);

  std::vector<Book> getMyBooks(const std::string &ownerId, int page, int limit);

  bool updateBook(const std::string &bookId, const std::string &ownerId,
                  Book book);

  bool deleteBook(const std::string &bookId, const std::string &ownerId);

private:
  BookRepository repository_;

  void validate(const Book &book) const;
};