#pragma once

#include "models/Book.h"

#include <optional>
#include <string>
#include <vector>

class BookRepository {
public:
  std::string create(const Book &book);

  std::optional<Book> findById(const std::string &id);

  PageResult<Book> findPublic(const BookFilter &filter,
                              const Pagination &pagination);

  PageResult<Book> findByOwner(const std::string &ownerId,
                               const std::optional<std::string> &status,
                               const Pagination &pagination);

  bool update(const std::string &id, const BookUpdate &update);

  bool updateStatus(const std::string &id, const std::string &status);

  bool acquireForPurchase(const std::string &bookId);

  bool releasePurchase(const std::string &bookId);

  bool exchangeOwnership(const std::string &requestedBookId,
                         const std::string &receiverId,
                         const std::string &offeredBookId,
                         const std::string &requesterId);

  bool updateRating(const std::string &bookId, double averageRating,
                    int reviewCount);

  bool restoreAvailability(const std::string &bookId);

  bool markReturned(const std::string &bookId);
};
