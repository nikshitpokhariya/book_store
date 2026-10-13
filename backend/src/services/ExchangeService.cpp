#include "services/ExchangeService.h"
#include "db/MongoDatabase.h"
#include "config/AppConfig.h"

#include <openssl/rand.h>
#include <mongocxx/exception/operation_exception.hpp>
#include <trantor/utils/Logger.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <thread>
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

  Pagination userBooksPagination{1, 1};
  auto userBooks = books_.findByOwner(requesterId, std::optional<std::string>("available"), userBooksPagination);
  if (userBooks.total == 0) {
    throw std::invalid_argument(
        "You must have at least one active listed book to propose an exchange");
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

  // Pre-validate existence, authorization, and status outside transaction to fail fast
  auto initialReq = exchanges_.findById(requestId);
  if (!initialReq) {
    throw std::runtime_error("EXCHANGE_NOT_FOUND");
  }

  if (initialReq->receiverId != receiverId) {
    throw std::runtime_error(
        "FORBIDDEN: Only the receiver of the exchange request can accept it");
  }

  if (initialReq->status != "PENDING") {
    throw std::runtime_error("EXCHANGE_NOT_PENDING: Request is already " +
                             initialReq->status);
  }

  const bool isProd = AppConfig::instance().isProduction();
  const auto txSupport = MongoDatabase::instance().transactionSupportStatus();
  const bool txSupported = (txSupport == MongoTransactionSupport::SUPPORTED);

  if (isProd && !txSupported) {
    LOG_ERROR << "[ExchangeService] CRITICAL: Transaction support unverified or unavailable in production for requestId: "
              << requestId << " (status: " << (txSupport == MongoTransactionSupport::UNKNOWN ? "UNKNOWN/UNVERIFIED" : "UNSUPPORTED")
              << "). Failing closed.";
    throw std::runtime_error(
        "EXCHANGE_TRANSACTIONS_UNAVAILABLE: Multi-document transactions are required in production, "
        "but database transaction support cannot be verified or is unsupported");
  }

  if (txSupported) {
    // Multi-Document Transaction Path (ACID compliant on replica sets / MongoDB Atlas)
    auto session = MongoDatabase::instance().startSession();
    const int maxTxAttempts = 3;
    const int maxCommitAttempts = 3;

    for (int txAttempt = 0; txAttempt < maxTxAttempts; ++txAttempt) {
      try {
        session.start_transaction();

        // 1. Confirm exchange request inside transaction snapshot
        auto req = exchanges_.findById(session, requestId);
        if (!req) {
          try { session.abort_transaction(); } catch (...) {}
          throw std::runtime_error("EXCHANGE_NOT_FOUND");
        }

        if (req->receiverId != receiverId) {
          try { session.abort_transaction(); } catch (...) {}
          throw std::runtime_error(
              "FORBIDDEN: Only the receiver of the exchange request can accept it");
        }

        if (req->status != "PENDING") {
          try { session.abort_transaction(); } catch (...) {}
          throw std::runtime_error("EXCHANGE_NOT_PENDING: Request is already " +
                                   req->status);
        }

        // 2. Atomically swap physical book ownership inside the transaction
        const bool swapped = books_.exchangeOwnership(
            session, req->requestedBook.bookId, req->receiverId,
            req->offeredBook.bookId, req->requesterId);

        if (!swapped) {
          try { session.abort_transaction(); } catch (...) {}
          throw std::runtime_error(
              "EXCHANGE_ACQUISITION_FAILED: One or both books are no longer "
              "available or their ownership has changed");
        }

        // 3. Conditionally update exchange request from PENDING to COMPLETED in transaction
        const bool statusUpdated = exchanges_.updateStatus(
            session, requestId, "COMPLETED",
            "Exchange accepted and physical book ownership successfully swapped",
            true, "PENDING");

        if (!statusUpdated) {
          try { session.abort_transaction(); } catch (...) {}
          auto current = exchanges_.findById(session, requestId);
          const std::string currentStatus = current ? current->status : "UNKNOWN";
          throw std::runtime_error(
              "EXCHANGE_NOT_PENDING: Request was already processed or cancelled (status: " +
              currentStatus + ")");
        }

        // 4. Commit transaction with dedicated bounded retry logic on UnknownTransactionCommitResult
        bool committed = false;
        for (int commitAttempt = 0; commitAttempt < maxCommitAttempts; ++commitAttempt) {
          try {
            session.commit_transaction();
            committed = true;
            break;
          } catch (const mongocxx::operation_exception &commitEx) {
            if (commitEx.has_error_label("UnknownTransactionCommitResult")) {
              if (commitAttempt + 1 < maxCommitAttempts) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50 * (commitAttempt + 1)));
                continue; // Retry commit_transaction on the same session and transaction
              }
              // Exhausted commit retry budget
              LOG_ERROR << "[ExchangeService] CRITICAL: Transaction commit outcome unresolved for exchange requestId: "
                        << requestId << " after " << maxCommitAttempts << " attempts. Manual reconciliation required.";
              throw std::runtime_error(
                  "EXCHANGE_COMMIT_INDETERMINATE: Transaction commit status indeterminate for requestId " +
                  requestId + " (UnknownTransactionCommitResult retry budget exhausted)");
            }
            // If commit threw without UnknownTransactionCommitResult (e.g. TransientTransactionError), rethrow
            throw;
          }
        }

        if (committed) {
          break; // Successfully committed transaction
        }
      } catch (const mongocxx::operation_exception &opEx) {
        try {
          session.abort_transaction();
        } catch (...) {}

        if (opEx.has_error_label("TransientTransactionError") && txAttempt + 1 < maxTxAttempts) {
          std::this_thread::sleep_for(std::chrono::milliseconds(50 * (txAttempt + 1)));
          continue; // Whole-transaction retry: fresh transaction starts on next iteration
        }
        throw;
      } catch (const std::runtime_error &re) {
        if (std::string(re.what()).rfind("EXCHANGE_COMMIT_INDETERMINATE", 0) == 0) {
          // Commit outcome is indeterminate. Do not abort or modify transaction state.
          throw;
        }
        try {
          session.abort_transaction();
        } catch (...) {}
        throw;
      } catch (...) {
        try {
          session.abort_transaction();
        } catch (...) {}
        throw;
      }
    }
  } else {
    // Non-transactional fallback for standalone MongoDB environments (DEVELOPMENT ONLY)
    if (!AppConfig::instance().allowStandaloneDev()) {
      LOG_ERROR << "[ExchangeService] Standalone MongoDB non-transactional fallback blocked for requestId: "
                << requestId << ". Set ALLOW_STANDALONE_DEV=true in non-production environments to enable.";
      throw std::runtime_error(
          "EXCHANGE_TRANSACTIONS_UNSUPPORTED: Multi-document transactions are not supported by the current MongoDB deployment. "
          "Set ALLOW_STANDALONE_DEV=true for local development fallback.");
    }

    LOG_WARN << "[ExchangeService] WARNING: Executing non-transactional exchange acceptance for requestId: "
             << requestId << " (Development standalone mode only. NOT crash-atomic).";

    const bool swapped = books_.exchangeOwnership(
        initialReq->requestedBook.bookId, initialReq->receiverId,
        initialReq->offeredBook.bookId, initialReq->requesterId);

    if (!swapped) {
      throw std::runtime_error(
          "EXCHANGE_ACQUISITION_FAILED: One or both books are no longer "
          "available or their ownership has changed");
    }

    bool statusUpdated = false;
    try {
      statusUpdated = exchanges_.updateStatus(
          requestId, "COMPLETED",
          "Exchange accepted and physical book ownership successfully swapped",
          true, "PENDING");
    } catch (const std::exception &e) {
      const bool reverted = books_.revertExchangeOwnership(
          initialReq->requestedBook.bookId, initialReq->receiverId,
          initialReq->offeredBook.bookId, initialReq->requesterId);
      if (!reverted) {
        LOG_FATAL << "[ExchangeService] CRITICAL COMPENSATION FAILURE: Failed to revert book ownership for requestId: "
                  << requestId << " after updateStatus exception (" << e.what()
                  << "). Inconsistent state across books collection; manual intervention required.";
      }
      throw;
    } catch (...) {
      const bool reverted = books_.revertExchangeOwnership(
          initialReq->requestedBook.bookId, initialReq->receiverId,
          initialReq->offeredBook.bookId, initialReq->requesterId);
      if (!reverted) {
        LOG_FATAL << "[ExchangeService] CRITICAL COMPENSATION FAILURE: Failed to revert book ownership for requestId: "
                  << requestId << " after unknown updateStatus exception. Inconsistent state across books collection; manual intervention required.";
      }
      throw;
    }

    if (!statusUpdated) {
      const bool reverted = books_.revertExchangeOwnership(
          initialReq->requestedBook.bookId, initialReq->receiverId,
          initialReq->offeredBook.bookId, initialReq->requesterId);
      if (!reverted) {
        LOG_FATAL << "[ExchangeService] CRITICAL COMPENSATION FAILURE: Failed to revert book ownership for requestId: "
                  << requestId << " when updateStatus returned false. Inconsistent state across books collection; manual intervention required.";
      }

      auto current = exchanges_.findById(requestId);
      const std::string currentStatus = current ? current->status : "UNKNOWN";
      throw std::runtime_error(
          "EXCHANGE_NOT_PENDING: Request was already processed or cancelled (status: " +
          currentStatus + (reverted ? ", book ownership reverted" : ", CRITICAL: compensation revert failed") + ")");
    }
  }

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

  const bool statusUpdated = exchanges_.updateStatus(
      requestId, "REJECTED",
      note.empty() ? "Exchange request declined" : note,
      false, "PENDING");

  if (!statusUpdated) {
    throw std::runtime_error("EXCHANGE_NOT_PENDING: Request is no longer pending");
  }

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

  const bool statusUpdated = exchanges_.updateStatus(
      requestId, "CANCELLED",
      note.empty() ? "Exchange request cancelled by requester" : note,
      false, "PENDING");

  if (!statusUpdated) {
    throw std::runtime_error("EXCHANGE_NOT_PENDING: Request is no longer pending");
  }

  auto updated = exchanges_.findById(requestId);
  if (!updated) {
    throw std::runtime_error("EXCHANGE_NOT_FOUND");
  }

  return *updated;
}
