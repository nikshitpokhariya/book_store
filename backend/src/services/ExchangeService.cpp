#include "services/ExchangeService.h"

#include <openssl/rand.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
std::string generateHex(std::size_t numBytes) {
  std::vector<unsigned char> bytes(numBytes);
  if (RAND_bytes(bytes.data(), static_cast<int>(numBytes)) != 1) {
    throw std::runtime_error("Random generation failed");
  }

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char b : bytes) {
    oss << std::setw(2) << static_cast<int>(b);
  }
  return oss.str();
}

std::string generateExchangeNumber() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &time);
#else
  gmtime_r(&time, &tm);
#endif
  char dateBuf[16];
  std::strftime(dateBuf, sizeof(dateBuf), "%Y%m%d", &tm);

  return "EXC-" + std::string(dateBuf) + "-" + generateHex(4);
}
} // namespace

ExchangeService::ExchangeService(ExchangeRepository &exchanges,
                                 BookRepository &books, UserRepository &users)
    : exchanges_(exchanges), books_(books), users_(users) {}

ExchangeRequest ExchangeService::createRequest(
    const std::string &requesterId, const std::string &requestedBookId,
    const std::string &offeredBookId, const std::string &message) {
  if (requesterId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  if (requestedBookId == offeredBookId) {
    throw std::invalid_argument("Cannot exchange a book for itself");
  }

  if (message.size() > 500) {
    throw std::invalid_argument("Message cannot exceed 500 characters");
  }

  auto requester = users_.findById(requesterId);
  if (!requester) {
    throw std::runtime_error("REQUESTER_NOT_FOUND");
  }

  auto requestedBook = books_.findById(requestedBookId);
  if (!requestedBook) {
    throw std::runtime_error("REQUESTED_BOOK_NOT_FOUND");
  }

  if (requestedBook->ownerId == requesterId) {
    throw std::invalid_argument(
        "Cannot request an exchange for your own book listing");
  }

  if (requestedBook->status != "available") {
    throw std::runtime_error("REQUESTED_BOOK_NOT_AVAILABLE");
  }

  auto receiver = users_.findById(requestedBook->ownerId);
  if (!receiver) {
    throw std::runtime_error("RECEIVER_NOT_FOUND");
  }

  auto offeredBook = books_.findById(offeredBookId);
  if (!offeredBook) {
    throw std::runtime_error("OFFERED_BOOK_NOT_FOUND");
  }

  if (offeredBook->ownerId != requesterId) {
    throw std::invalid_argument(
        "You can only offer a book listing that you own");
  }

  if (offeredBook->status != "available") {
    throw std::runtime_error("OFFERED_BOOK_NOT_AVAILABLE");
  }

  ExchangeRequest req;
  req.exchangeNumber = generateExchangeNumber();
  req.requesterId = requesterId;
  req.requesterUsername = requester->username;
  req.receiverId = receiver->id;
  req.receiverUsername = receiver->username;
  req.status = "PENDING";
  req.message = message;

  // Snapshot requested book
  req.requestedBook.bookId = requestedBook->id;
  req.requestedBook.title = requestedBook->title;
  req.requestedBook.author = requestedBook->author;
  req.requestedBook.condition = requestedBook->condition;
  req.requestedBook.category = requestedBook->category;
  req.requestedBook.price = requestedBook->price;
  req.requestedBook.coverImage = requestedBook->coverImage;
  req.requestedBook.originalOwnerId = receiver->id;
  req.requestedBook.originalOwnerUsername = receiver->username;

  // Snapshot offered book
  req.offeredBook.bookId = offeredBook->id;
  req.offeredBook.title = offeredBook->title;
  req.offeredBook.author = offeredBook->author;
  req.offeredBook.condition = offeredBook->condition;
  req.offeredBook.category = offeredBook->category;
  req.offeredBook.price = offeredBook->price;
  req.offeredBook.coverImage = offeredBook->coverImage;
  req.offeredBook.originalOwnerId = requesterId;
  req.offeredBook.originalOwnerUsername = requester->username;

  req.id = exchanges_.create(req);
  return req;
}

PageResult<ExchangeRequest>
ExchangeService::getSentRequests(const std::string &requesterId, int page,
                                 int limit) {
  if (requesterId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  page = std::max(1, page);
  limit = std::clamp(limit, 1, 50);

  return exchanges_.findByRequester(requesterId, page, limit);
}

PageResult<ExchangeRequest>
ExchangeService::getReceivedRequests(const std::string &receiverId, int page,
                                     int limit) {
  if (receiverId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  page = std::max(1, page);
  limit = std::clamp(limit, 1, 50);

  return exchanges_.findByReceiver(receiverId, page, limit);
}

PageResult<ExchangeRequest>
ExchangeService::getUserHistory(const std::string &userId, int page,
                                int limit) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  page = std::max(1, page);
  limit = std::clamp(limit, 1, 50);

  return exchanges_.findUserHistory(userId, page, limit);
}

ExchangeRequest
ExchangeService::getRequestDetails(const std::string &requestId,
                                   const std::string &userId) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  auto req = exchanges_.findById(requestId);
  if (!req) {
    throw std::runtime_error("EXCHANGE_NOT_FOUND");
  }

  if (req->requesterId != userId && req->receiverId != userId) {
    throw std::runtime_error("FORBIDDEN");
  }

  return *req;
}

ExchangeRequest ExchangeService::acceptExchange(const std::string &requestId,
                                                const std::string &receiverId) {
  if (receiverId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  auto req = exchanges_.findById(requestId);
  if (!req) {
    throw std::runtime_error("EXCHANGE_NOT_FOUND");
  }

  if (req->receiverId != receiverId) {
    throw std::runtime_error(
        "FORBIDDEN: Only the receiver of the exchange request can accept it");
  }

  if (req->status != "PENDING") {
    throw std::runtime_error("EXCHANGE_NOT_PENDING: Request is already " +
                             req->status);
  }

  // Atomically swap ownership of both physical books
  const bool swapped = books_.exchangeOwnership(
      req->requestedBook.bookId, req->receiverId, req->offeredBook.bookId,
      req->requesterId);

  if (!swapped) {
    throw std::runtime_error(
        "EXCHANGE_ACQUISITION_FAILED: One or both books are no longer "
        "available or their ownership has changed");
  }

  exchanges_.updateStatus(
      requestId, "COMPLETED",
      "Exchange accepted and physical book ownership successfully swapped",
      true);

  auto updated = exchanges_.findById(requestId);
  if (!updated) {
    throw std::runtime_error("EXCHANGE_NOT_FOUND");
  }

  return *updated;
}

ExchangeRequest ExchangeService::rejectExchange(const std::string &requestId,
                                                const std::string &receiverId,
                                                const std::string &note) {
  if (receiverId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  auto req = exchanges_.findById(requestId);
  if (!req) {
    throw std::runtime_error("EXCHANGE_NOT_FOUND");
  }

  if (req->receiverId != receiverId) {
    throw std::runtime_error(
        "FORBIDDEN: Only the receiver of the exchange request can reject it");
  }

  if (req->status != "PENDING") {
    throw std::runtime_error("EXCHANGE_NOT_PENDING: Request is already " +
                             req->status);
  }

  exchanges_.updateStatus(requestId, "REJECTED",
                          note.empty() ? "Exchange request declined" : note);

  auto updated = exchanges_.findById(requestId);
  if (!updated) {
    throw std::runtime_error("EXCHANGE_NOT_FOUND");
  }

  return *updated;
}

ExchangeRequest ExchangeService::cancelExchange(const std::string &requestId,
                                                const std::string &requesterId,
                                                const std::string &note) {
  if (requesterId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  auto req = exchanges_.findById(requestId);
  if (!req) {
    throw std::runtime_error("EXCHANGE_NOT_FOUND");
  }

  if (req->requesterId != requesterId) {
    throw std::runtime_error(
        "FORBIDDEN: Only the requester can cancel their exchange request");
  }

  if (req->status != "PENDING") {
    throw std::runtime_error("EXCHANGE_NOT_PENDING: Request is already " +
                             req->status);
  }

  exchanges_.updateStatus(
      requestId, "CANCELLED",
      note.empty() ? "Exchange request cancelled by requester" : note);

  auto updated = exchanges_.findById(requestId);
  if (!updated) {
    throw std::runtime_error("EXCHANGE_NOT_FOUND");
  }

  return *updated;
}
