#pragma once

#include "models/Book.h"
#include "repositories/BookRepository.h"
#include "repositories/UserRepository.h"

#include <optional>
#include <string>

class BookService {
public:
  BookService(BookRepository &books, UserRepository &users);

  Book createListing(const std::string &ownerId, Book bookInput);

  std::optional<Book> getPublicBook(const std::string &id);

  Book getBookForUser(const std::string &id,
                      const std::optional<std::string> &userId);

  PageResult<Book> browseBooks(BookFilter filter, Pagination pagination);

  PageResult<Book> getMyListings(const std::string &ownerId,
                                const std::optional<std::string> &status,
                                Pagination pagination);

  Book updateListing(const std::string &id, const std::string &userId,
                     const BookUpdate &update);

  void removeListing(const std::string &id, const std::string &userId);
  std::vector<std::string> getSuggestions(const std::string &query, int limit);

private:
  BookRepository &books_;
  UserRepository &users_;

  void enrichWithOwner(Book &book);
};
