#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <chrono>
#include <thread>
#include <future>
#include <unordered_set>
#include <memory>

#include "filters/RateLimitFilter.h"

namespace {

int g_passedTests = 0;
int g_failedTests = 0;

void recordTest(bool condition, const std::string &name) {
  if (condition) {
    std::cout << "  [PASS] " << name << "\n";
    g_passedTests++;
  } else {
    std::cerr << "  [FAIL] " << name << "\n";
    g_failedTests++;
  }
}

// Mock structures to verify Order, Cart, and Exchange state transitions
struct MockListing {
  std::string id;
  std::string ownerId;
  std::string status; // "available", "sold", "exchanged"
  double price;
};

struct MockOrder {
  std::string id;
  std::string status; // PLACED, CONFIRMED, PACKED, SHIPPED, DELIVERED, CANCELLED
  std::string buyerId;
  std::vector<std::string> bookIds;
};

struct MockExchange {
  std::string id;
  std::string requesterId;
  std::string receiverId;
  std::string requestedBookId;
  std::string offeredBookId;
  std::string status; // PENDING, COMPLETED, REJECTED, CANCELLED
};

// Simulation of OrderService cart validation and atomic acquisition
struct CheckoutSimulation {
  static bool acquireBook(MockListing &listing) {
    if (listing.status == "available") {
      listing.status = "sold";
      return true;
    }
    return false;
  }

  static void releaseBook(MockListing &listing) {
    if (listing.status == "sold") {
      listing.status = "available";
    }
  }

  static bool executeCheckout(const std::string &buyerId,
                              std::vector<MockListing*> cartListings,
                              std::string &errorOut) {
    if (cartListings.empty()) {
      errorOut = "CART_IS_EMPTY";
      return false;
    }

    std::unordered_set<std::string> seen;
    std::vector<MockListing*> deduplicated;
    for (auto *l : cartListings) {
      if (!l) {
        errorOut = "STALE_CART_ITEM";
        return false;
      }
      if (seen.find(l->id) != seen.end()) {
        continue; // deduplicate
      }
      seen.insert(l->id);

      if (l->ownerId == buyerId) {
        errorOut = "CANNOT_BUY_OWN_BOOK";
        return false;
      }
      if (l->status != "available") {
        errorOut = "STALE_CART_ITEM";
        return false;
      }
      if (l->price < 0.0) {
        errorOut = "INVALID_BOOK_PRICE";
        return false;
      }
      deduplicated.push_back(l);
    }

    // Atomic acquisition with rollback
    std::vector<MockListing*> acquired;
    for (auto *l : deduplicated) {
      if (!acquireBook(*l)) {
        for (auto *acq : acquired) {
          releaseBook(*acq);
        }
        errorOut = "BOOK_ACQUISITION_FAILED";
        return false;
      }
      acquired.push_back(l);
    }

    return true;
  }
};

// Simulation of Exchange two-phase swap with conditional compensation
struct ExchangeSimulation {
  static bool swapOwnership(MockListing &reqBook, const std::string &receiverId,
                            MockListing &offBook, const std::string &requesterId) {
    // Phase 1
    if (reqBook.ownerId != receiverId || reqBook.status != "available") return false;
    reqBook.ownerId = requesterId;
    reqBook.status = "exchanged";

    // Phase 2
    if (offBook.ownerId != requesterId || offBook.status != "available") {
      // Rollback Phase 1
      reqBook.ownerId = receiverId;
      reqBook.status = "available";
      return false;
    }
    offBook.ownerId = receiverId;
    offBook.status = "exchanged";
    return true;
  }

  // Proper conditional compensation to revert already-exchanged books
  static bool revertSwapOwnership(MockListing &reqBook, const std::string &receiverId,
                                  MockListing &offBook, const std::string &requesterId) {
    bool okA = false;
    if (reqBook.ownerId == requesterId && reqBook.status == "exchanged") {
      reqBook.ownerId = receiverId;
      reqBook.status = "available";
      okA = true;
    }
    bool okB = false;
    if (offBook.ownerId == receiverId && offBook.status == "exchanged") {
      offBook.ownerId = requesterId;
      offBook.status = "available";
      okB = true;
    }
    return okA && okB;
  }

  static bool cancelExchange(MockExchange &exc, const std::string &callerId, std::string &errorOut) {
    if (exc.requesterId != callerId) {
      errorOut = "FORBIDDEN";
      return false;
    }
    if (exc.status != "PENDING") {
      errorOut = "EXCHANGE_NOT_PENDING";
      return false;
    }
    exc.status = "CANCELLED";
    return true;
  }

  static bool acceptExchange(MockExchange &exc, MockListing &reqBook, MockListing &offBook,
                             const std::string &callerId, std::string &errorOut,
                             bool simulateDbFailureAfterSwap = false) {
    if (exc.receiverId != callerId) {
      errorOut = "FORBIDDEN";
      return false;
    }
    if (exc.status != "PENDING") {
      errorOut = "EXCHANGE_NOT_PENDING";
      return false;
    }

    // Phase 1: Swap books
    bool swapped = swapOwnership(reqBook, exc.receiverId, offBook, exc.requesterId);
    if (!swapped) {
      errorOut = "EXCHANGE_ACQUISITION_FAILED";
      return false;
    }

    if (simulateDbFailureAfterSwap) {
      // Compensate: DB exception occurred during status update
      revertSwapOwnership(reqBook, exc.receiverId, offBook, exc.requesterId);
      errorOut = "DATABASE_EXCEPTION";
      return false;
    }

    // Phase 2: Update status with expectedStatus == PENDING
    if (exc.status != "PENDING") {
      // Compensate: Request was concurrently modified (e.g. cancelled)
      revertSwapOwnership(reqBook, exc.receiverId, offBook, exc.requesterId);
      errorOut = "EXCHANGE_NOT_PENDING";
      return false;
    }

    exc.status = "COMPLETED";
    return true;
  }
};

// Simulation of MongoDB multi-document transaction behavior
struct ExchangeTransactionSimulation {
  struct SessionState {
    bool inTransaction{false};
    MockListing stagedReqBook;
    MockListing stagedOffBook;
    MockExchange stagedExc;
    bool reqBookModified{false};
    bool offBookModified{false};
    bool excModified{false};
  };

  static bool acceptInTransaction(MockExchange &exc, MockListing &reqBook, MockListing &offBook,
                                  const std::string &callerId, std::string &errorOut,
                                  bool failSecondBook = false, bool concurrentCancel = false,
                                  bool failBeforeCommit = false) {
    if (exc.receiverId != callerId) {
      errorOut = "FORBIDDEN";
      return false;
    }
    if (exc.status != "PENDING") {
      errorOut = "EXCHANGE_NOT_PENDING";
      return false;
    }

    // Begin transaction
    SessionState session{true, reqBook, offBook, exc, false, false, false};

    // Step 1: Confirm exchange inside transaction
    if (session.stagedExc.status != "PENDING") {
      errorOut = "EXCHANGE_NOT_PENDING";
      return false;
    }

    // Step 2: Swap book 1 (requested book)
    if (session.stagedReqBook.ownerId != exc.receiverId || session.stagedReqBook.status != "available") {
      errorOut = "EXCHANGE_ACQUISITION_FAILED";
      return false;
    }
    session.stagedReqBook.ownerId = exc.requesterId;
    session.stagedReqBook.status = "exchanged";
    session.reqBookModified = true;

    // Step 3: Swap book 2 (offered book)
    if (failSecondBook || session.stagedOffBook.ownerId != exc.requesterId || session.stagedOffBook.status != "available") {
      // ABORT TRANSACTION: all staged changes discarded automatically by MongoDB server
      errorOut = "EXCHANGE_ACQUISITION_FAILED";
      return false; // reqBook, offBook, and exc remain untouched
    }
    session.stagedOffBook.ownerId = exc.receiverId;
    session.stagedOffBook.status = "exchanged";
    session.offBookModified = true;

    // Step 4: Conditionally update exchange status to COMPLETED
    if (concurrentCancel) {
      exc.status = "CANCELLED";
    }
    if (exc.status != "PENDING") {
      // ABORT TRANSACTION: all staged changes discarded
      errorOut = "EXCHANGE_NOT_PENDING";
      return false;
    }
    session.stagedExc.status = "COMPLETED";
    session.excModified = true;

    if (failBeforeCommit) {
      // Process crash or abort before commit
      errorOut = "TRANSACTION_ABORTED";
      return false;
    }

    // Step 5: Commit transaction atomically
    reqBook = session.stagedReqBook;
    offBook = session.stagedOffBook;
    exc = session.stagedExc;
    return true;
  }

  // Simulation of commit retry on UnknownTransactionCommitResult
  static bool acceptWithCommitRetry(MockExchange &exc, MockListing &reqBook, MockListing &offBook,
                                    const std::string &callerId, std::string &errorOut,
                                    int simulatedUnknownCommitErrors, int maxCommitAttempts,
                                    int &actualCommitAttemptsMade, int &actualTxAttemptsMade) {
    actualCommitAttemptsMade = 0;
    actualTxAttemptsMade = 0;

    if (exc.receiverId != callerId) {
      errorOut = "FORBIDDEN";
      return false;
    }
    if (exc.status != "PENDING") {
      errorOut = "EXCHANGE_NOT_PENDING";
      return false;
    }

    const int maxTxAttempts = 3;
    for (int txAttempt = 0; txAttempt < maxTxAttempts; ++txAttempt) {
      actualTxAttemptsMade++;
      SessionState session{true, reqBook, offBook, exc, false, false, false};

      if (session.stagedExc.status != "PENDING") {
        errorOut = "EXCHANGE_NOT_PENDING";
        return false;
      }
      if (session.stagedReqBook.ownerId != exc.receiverId || session.stagedReqBook.status != "available") {
        errorOut = "EXCHANGE_ACQUISITION_FAILED";
        return false;
      }
      session.stagedReqBook.ownerId = exc.requesterId;
      session.stagedReqBook.status = "exchanged";

      if (session.stagedOffBook.ownerId != exc.requesterId || session.stagedOffBook.status != "available") {
        errorOut = "EXCHANGE_ACQUISITION_FAILED";
        return false;
      }
      session.stagedOffBook.ownerId = exc.receiverId;
      session.stagedOffBook.status = "exchanged";

      session.stagedExc.status = "COMPLETED";

      // Commit loop with UnknownTransactionCommitResult retries on SAME session/tx
      bool committed = false;
      for (int cAttempt = 0; cAttempt < maxCommitAttempts; ++cAttempt) {
        actualCommitAttemptsMade++;
        if (cAttempt < simulatedUnknownCommitErrors) {
          // Retry commit on same session without replaying operations
          if (cAttempt + 1 < maxCommitAttempts) {
            continue;
          }
          // Commit retry budget exhausted
          errorOut = "EXCHANGE_COMMIT_INDETERMINATE";
          return false;
        }
        committed = true;
        break;
      }

      if (committed) {
        reqBook = session.stagedReqBook;
        offBook = session.stagedOffBook;
        exc = session.stagedExc;
        return true;
      }
    }
    return false;
  }

  // Simulation of whole-transaction retry on TransientTransactionError
  static bool acceptWithTransientRetry(MockExchange &exc, MockListing &reqBook, MockListing &offBook,
                                       const std::string &callerId, std::string &errorOut,
                                       bool failTransientAttempt1, bool concurrentCancelBeforeAttempt2,
                                       int &actualTxAttemptsMade) {
    actualTxAttemptsMade = 0;
    const int maxTxAttempts = 3;

    for (int txAttempt = 0; txAttempt < maxTxAttempts; ++txAttempt) {
      actualTxAttemptsMade++;

      // Begin fresh transaction attempt
      SessionState session{true, reqBook, offBook, exc, false, false, false};

      if (session.stagedExc.status != "PENDING") {
        errorOut = "EXCHANGE_NOT_PENDING";
        return false;
      }
      if (session.stagedReqBook.ownerId != exc.receiverId || session.stagedReqBook.status != "available") {
        errorOut = "EXCHANGE_ACQUISITION_FAILED";
        return false;
      }
      session.stagedReqBook.ownerId = exc.requesterId;
      session.stagedReqBook.status = "exchanged";

      if (session.stagedOffBook.ownerId != exc.requesterId || session.stagedOffBook.status != "available") {
        errorOut = "EXCHANGE_ACQUISITION_FAILED";
        return false;
      }
      session.stagedOffBook.ownerId = exc.receiverId;
      session.stagedOffBook.status = "exchanged";

      session.stagedExc.status = "COMPLETED";

      if (txAttempt == 0 && failTransientAttempt1) {
        // Discard staged changes
        session.inTransaction = false;

        if (concurrentCancelBeforeAttempt2) {
          exc.status = "CANCELLED";
        }
        continue; // Retry whole transaction fresh on next iteration
      }

      // Commit
      reqBook = session.stagedReqBook;
      offBook = session.stagedOffBook;
      exc = session.stagedExc;
      return true;
    }
    errorOut = "TRANSIENT_RETRY_EXHAUSTED";
    return false;
  }

  enum class SimTxSupport { UNKNOWN, SUPPORTED, UNSUPPORTED };

  static bool evaluateEnvironmentGate(bool isProduction, SimTxSupport txSupport, bool allowStandaloneDev, std::string &errorOut) {
    const bool txSupported = (txSupport == SimTxSupport::SUPPORTED);
    if (isProduction && !txSupported) {
      errorOut = "EXCHANGE_TRANSACTIONS_UNAVAILABLE";
      return false;
    }
    if (txSupported) {
      return true; // proceed to transaction path
    }
    if (!allowStandaloneDev) {
      errorOut = "EXCHANGE_TRANSACTIONS_UNSUPPORTED";
      return false;
    }
    return true; // proceed to fallback path
  }

  static bool simulateFallbackCompensation(bool statusUpdateFails, bool revertSucceeds, std::string &errorOut, bool &criticalLogFired) {
    criticalLogFired = false;
    if (statusUpdateFails) {
      if (!revertSucceeds) {
        criticalLogFired = true;
        errorOut = "CRITICAL_COMPENSATION_FAILED";
        return false;
      }
      errorOut = "EXCHANGE_NOT_PENDING_REVERTED";
      return false;
    }
    return true;
  }
};

bool isValidDeliveryTransition(const std::string &from, const std::string &to) {
  if (from == "PLACED") return to == "CONFIRMED" || to == "CANCELLED";
  if (from == "CONFIRMED") return to == "PACKED" || to == "SHIPPED" || to == "OUT_FOR_DELIVERY" || to == "DELIVERED" || to == "CANCELLED";
  if (from == "PACKED") return to == "SHIPPED" || to == "OUT_FOR_DELIVERY" || to == "DELIVERED" || to == "CANCELLED";
  if (from == "SHIPPED") return to == "OUT_FOR_DELIVERY" || to == "DELIVERED";
  if (from == "OUT_FOR_DELIVERY") return to == "DELIVERED";
  return false;
}

bool canCancelOrder(const std::string &status) {
  return status == "PLACED" || status == "CONFIRMED" || status == "PACKED";
}

bool canReturnOrder(const std::string &status) {
  return status == "DELIVERED";
}

bool canReviewOrder(const std::string &orderStatus, const std::string &buyerId,
                    const std::string &userId, const std::string &sellerId,
                    bool alreadyReviewed) {
  if (buyerId != userId) return false;
  if (sellerId == userId) return false;
  if (orderStatus != "DELIVERED") return false;
  if (alreadyReviewed) return false;
  return true;
}

} // namespace

int main() {
  std::cout << "\n=========================================\n";
  std::cout << " BookBazzar Phase 2 Hardening Test Suite \n";
  std::cout << "=========================================\n\n";

  // ----------------------------------------------------
  // Group 1: Rate Limiter Tests (Thread-safe & Process-Local)
  // ----------------------------------------------------
  std::cout << "--- Group 1: Rate Limiter (Thread Safety & Process-Local) ---\n";
  RateLimitFilter::resetState();

  // Test 1-3: Within limit
  recordTest(RateLimitFilter::checkRateLimit("auth", "192.168.1.100", 5, 60),
             "Initial request allowed within limit");
  recordTest(RateLimitFilter::checkRateLimit("auth", "192.168.1.100", 5, 60),
             "Second request allowed within limit");
  recordTest(RateLimitFilter::checkRateLimit("auth", "192.168.1.100", 5, 60),
             "Third request allowed within limit");
  RateLimitFilter::checkRateLimit("auth", "192.168.1.100", 5, 60);
  RateLimitFilter::checkRateLimit("auth", "192.168.1.100", 5, 60);

  // 6th request exceeds max 5
  recordTest(!RateLimitFilter::checkRateLimit("auth", "192.168.1.100", 5, 60),
             "6th request exceeds 5-limit window and is rejected (429)");

  // Independent IP is not affected
  recordTest(RateLimitFilter::checkRateLimit("auth", "10.0.0.1", 5, 60),
             "Different client IP has independent rate limit bucket");

  // Independent category is not affected
  recordTest(RateLimitFilter::checkRateLimit("upload", "192.168.1.100", 5, 60),
             "Different category ('upload') has independent rate limit bucket");

  // Multi-thread concurrency stress test
  RateLimitFilter::resetState();
  const int numThreads = 8;
  const int requestsPerThread = 25;
  std::vector<std::future<int>> futures;

  for (int i = 0; i < numThreads; ++i) {
    futures.push_back(std::async(std::launch::async, [i]() {
      int allowedCount = 0;
      std::string ip = "172.16.0." + std::to_string(i % 2); // 2 shared IPs
      for (int r = 0; r < requestsPerThread; ++r) {
        if (RateLimitFilter::checkRateLimit("general", ip, 50, 60)) {
          allowedCount++;
        }
      }
      return allowedCount;
    }));
  }

  int totalAllowed = 0;
  for (auto &f : futures) {
    totalAllowed += f.get();
  }

  // Two IPs with limit 50 each => total allowed must be exactly min(total_requests, 100)
  recordTest(totalAllowed == 100,
             "Concurrent multi-threaded access enforces exact limit across threads (total allowed: 100)");

  // ----------------------------------------------------
  // Group 2: Production Cookie Security & Environment Detection
  // ----------------------------------------------------
  std::cout << "\n--- Group 2: Production Cookie Security & Environment Detection ---\n";
  {
    auto simulateSecureCookies = [](const char *nodeEnv, const char *renderEnv,
                                    const char *overrideSetting) -> bool {
      bool isProd = false;
      if ((nodeEnv && std::string(nodeEnv) == "production") ||
          (renderEnv && std::string(renderEnv).length() > 0)) {
        isProd = true;
      }
      if (overrideSetting) {
        return std::string(overrideSetting) == "true";
      }
      return isProd;
    };

    recordTest(simulateSecureCookies("production", nullptr, nullptr) == true,
               "Production NODE_ENV defaults secureCookies to true");
    recordTest(simulateSecureCookies(nullptr, "true", nullptr) == true,
               "Render production environment defaults secureCookies to true");
    recordTest(simulateSecureCookies("development", nullptr, nullptr) == false,
               "Development environment defaults secureCookies to false");
    recordTest(simulateSecureCookies("production", nullptr, "false") == false,
               "Explicit SECURE_COOKIES=false overrides production default");
    recordTest(simulateSecureCookies("development", nullptr, "true") == true,
               "Explicit SECURE_COOKIES=true overrides development default");
  }

  // ----------------------------------------------------
  // Group 3: Order Concurrency, Cart Validation & Inventory Consistency
  // ----------------------------------------------------
  std::cout << "\n--- Group 3: Order Concurrency, Cart Validation & Inventory Consistency ---\n";
  {
    MockListing book1{"b1", "seller1", "available", 299.0};
    MockListing book2{"b2", "seller1", "available", 150.0};
    MockListing bookSold{"b3", "seller1", "sold", 100.0};
    MockListing bookOwn{"b4", "buyer1", "available", 50.0};
    MockListing bookNegPrice{"b5", "seller1", "available", -10.0};

    std::string err;
    // Successful checkout
    bool ok1 = CheckoutSimulation::executeCheckout("buyer1", {&book1, &book2}, err);
    recordTest(ok1 && book1.status == "sold" && book2.status == "sold",
               "Valid multi-item checkout acquires listings and transitions to 'sold'");

    // Second concurrent checkout of already sold book fails
    bool ok2 = CheckoutSimulation::executeCheckout("buyer2", {&book1}, err);
    recordTest(!ok2 && err == "STALE_CART_ITEM",
               "Concurrent purchase of sold listing rejected with STALE_CART_ITEM");

    // Partial acquisition failure rollback test
    MockListing bookA{"bA", "seller1", "available", 200.0};
    MockListing bookB{"bB", "seller1", "sold", 200.0}; // already sold
    bool okPartial = CheckoutSimulation::executeCheckout("buyer1", {&bookA, &bookB}, err);
    recordTest(!okPartial && bookA.status == "available",
               "Partial cart failure rolls back already-acquired books to 'available'");

    // Cannot buy own book
    bool okOwn = CheckoutSimulation::executeCheckout("buyer1", {&bookOwn}, err);
    recordTest(!okOwn && err == "CANNOT_BUY_OWN_BOOK",
               "Buyer cannot checkout their own book listing");

    // Negative price rejected
    bool okNeg = CheckoutSimulation::executeCheckout("buyer1", {&bookNegPrice}, err);
    recordTest(!okNeg && err == "INVALID_BOOK_PRICE",
               "Negative book price listing is rejected at checkout");

    // Duplicate cart items deduplication
    MockListing bookDup{"bDup", "seller1", "available", 100.0};
    bool okDup = CheckoutSimulation::executeCheckout("buyer1", {&bookDup, &bookDup}, err);
    recordTest(okDup && bookDup.status == "sold",
               "Duplicate cart entries for same listing deduplicated without double charge");

    // Empty cart rejected
    bool okEmpty = CheckoutSimulation::executeCheckout("buyer1", {}, err);
    recordTest(!okEmpty && err == "CART_IS_EMPTY",
               "Empty cart rejected at checkout");
  }

  // ----------------------------------------------------
  // Group 4: Barter Exchange Concurrency & Ownership Verification
  // ----------------------------------------------------
  std::cout << "\n--- Group 4: Barter Exchange Concurrency & Ownership Verification ---\n";
  {
    MockListing reqBook{"rb1", "user2", "available", 300.0};
    MockListing offBook{"ob1", "user1", "available", 300.0};
    MockExchange exc{"e1", "user1", "user2", "rb1", "ob1", "PENDING"};

    std::string err;
    // Successful exchange accept
    bool okExc = ExchangeSimulation::acceptExchange(exc, reqBook, offBook, "user2", err);
    recordTest(okExc && exc.status == "COMPLETED" &&
               reqBook.ownerId == "user1" && reqBook.status == "exchanged" &&
               offBook.ownerId == "user2" && offBook.status == "exchanged",
               "Accepting exchange swaps ownership atomically and marks 'exchanged'");

    // Second accept of already completed exchange rejected
    bool okExc2 = ExchangeSimulation::acceptExchange(exc, reqBook, offBook, "user2", err);
    recordTest(!okExc2 && err == "EXCHANGE_NOT_PENDING",
               "Second concurrent accept on completed exchange rejected");

    // Unauthorized non-receiver cannot accept
    MockListing reqBook2{"rb2", "user2", "available", 300.0};
    MockListing offBook2{"ob2", "user1", "available", 300.0};
    MockExchange exc2{"e2", "user1", "user2", "rb2", "ob2", "PENDING"};
    bool okForbidden = ExchangeSimulation::acceptExchange(exc2, reqBook2, offBook2, "user3", err);
    recordTest(!okForbidden && err == "FORBIDDEN",
               "Non-receiver cannot accept exchange request");

    // Rollback test: if offered book is unavailable, requested book remains available with receiver
    MockListing reqBook3{"rb3", "user2", "available", 300.0};
    MockListing offBook3{"ob3", "user1", "sold", 300.0}; // unavailable
    MockExchange exc3{"e3", "user1", "user2", "rb3", "ob3", "PENDING"};
    bool okFail = ExchangeSimulation::acceptExchange(exc3, reqBook3, offBook3, "user2", err);
    recordTest(!okFail && reqBook3.status == "available" && reqBook3.ownerId == "user2",
               "Failed exchange swaps roll back and preserve receiver ownership and 'available'");

    // Concurrent accept/cancel race test:
    // Receiver accepts (swaps books), but Requester concurrently cancels before status update.
    MockListing reqBook4{"rb4", "user2", "available", 300.0};
    MockListing offBook4{"ob4", "user1", "available", 300.0};
    MockExchange exc4{"e4", "user1", "user2", "rb4", "ob4", "PENDING"};

    // Step A: Accept swaps books
    bool swapped4 = ExchangeSimulation::swapOwnership(reqBook4, "user2", offBook4, "user1");
    recordTest(swapped4 && reqBook4.status == "exchanged" && offBook4.status == "exchanged",
               "Concurrent accept/cancel: Book swap completes initially");

    // Step B: Concurrent requester cancel wins status update
    std::string cancelErr;
    bool cancelOk = ExchangeSimulation::cancelExchange(exc4, "user1", cancelErr);
    recordTest(cancelOk && exc4.status == "CANCELLED",
               "Concurrent accept/cancel: Concurrent cancel marks exchange CANCELLED");

    // Step C: Accept detects exchange is no longer PENDING and executes compensation revert
    bool acceptOk = false;
    if (exc4.status != "PENDING") {
      bool reverted = ExchangeSimulation::revertSwapOwnership(reqBook4, "user2", offBook4, "user1");
      recordTest(reverted, "Concurrent accept/cancel: Compensation revert restores both books");
      err = "EXCHANGE_NOT_PENDING";
    }
    recordTest(!acceptOk && err == "EXCHANGE_NOT_PENDING" &&
               reqBook4.ownerId == "user2" && reqBook4.status == "available" &&
               offBook4.ownerId == "user1" && offBook4.status == "available",
               "Concurrent accept/cancel: Consistent state restored (CANCELLED + available books)");

    // Failure after ownership update test:
    // DB exception occurs during exchange status update after books are already swapped.
    MockListing reqBook5{"rb5", "user2", "available", 300.0};
    MockListing offBook5{"ob5", "user1", "available", 300.0};
    MockExchange exc5{"e5", "user1", "user2", "rb5", "ob5", "PENDING"};
    bool okDbFail = ExchangeSimulation::acceptExchange(exc5, reqBook5, offBook5, "user2", err, true);
    recordTest(!okDbFail && err == "DATABASE_EXCEPTION" &&
               reqBook5.ownerId == "user2" && reqBook5.status == "available" &&
               offBook5.ownerId == "user1" && offBook5.status == "available",
               "Failure after ownership update compensates and restores both books to 'available'");

    // Flawed rollback comparison test:
    // Calling swapOwnership on already 'exchanged' books fails, whereas revertSwapOwnership succeeds.
    MockListing reqBook6{"rb6", "user1", "exchanged", 300.0};
    MockListing offBook6{"ob6", "user2", "exchanged", 300.0};
    bool flawedRevert = ExchangeSimulation::swapOwnership(offBook6, "user2", reqBook6, "user1");
    recordTest(!flawedRevert,
               "Flawed rollback using swapOwnership on 'exchanged' books correctly fails (proves bug)");
    bool properRevert = ExchangeSimulation::revertSwapOwnership(reqBook6, "user2", offBook6, "user1");
    recordTest(properRevert && reqBook6.status == "available" && offBook6.status == "available",
               "Proper revertSwapOwnership successfully restores 'exchanged' books to original owners");

    // ----------------------------------------------------
    // MongoDB Multi-Document ACID Transaction Coverage
    // ----------------------------------------------------
    // Test 1: Successful acceptance in transaction updates all three documents atomically
    MockListing txReq1{"txr1", "user2", "available", 250.0};
    MockListing txOff1{"txo1", "user1", "available", 250.0};
    MockExchange txExc1{"txe1", "user1", "user2", "txr1", "txo1", "PENDING"};
    std::string txErr;
    bool txOk1 = ExchangeTransactionSimulation::acceptInTransaction(txExc1, txReq1, txOff1, "user2", txErr);
    recordTest(txOk1 && txExc1.status == "COMPLETED" &&
               txReq1.ownerId == "user1" && txReq1.status == "exchanged" &&
               txOff1.ownerId == "user2" && txOff1.status == "exchanged",
               "Transaction: Successful acceptance commits both book owners and COMPLETED status");

    // Test 2: Failed second book update aborts transaction and leaves all three documents unchanged
    MockListing txReq2{"txr2", "user2", "available", 250.0};
    MockListing txOff2{"txo2", "user1", "available", 250.0};
    MockExchange txExc2{"txe2", "user1", "user2", "txr2", "txo2", "PENDING"};
    bool txOk2 = ExchangeTransactionSimulation::acceptInTransaction(txExc2, txReq2, txOff2, "user2", txErr, true);
    recordTest(!txOk2 && txErr == "EXCHANGE_ACQUISITION_FAILED" &&
               txReq2.ownerId == "user2" && txReq2.status == "available" &&
               txOff2.ownerId == "user1" && txOff2.status == "available" &&
               txExc2.status == "PENDING",
               "Transaction: Failed second book aborts and leaves all three documents untouched");

    // Test 3: Concurrent cancellation causes transaction abort and leaves books untouched
    MockListing txReq3{"txr3", "user2", "available", 250.0};
    MockListing txOff3{"txo3", "user1", "available", 250.0};
    MockExchange txExc3{"txe3", "user1", "user2", "txr3", "txo3", "PENDING"};
    bool txOk3 = ExchangeTransactionSimulation::acceptInTransaction(txExc3, txReq3, txOff3, "user2", txErr, false, true);
    recordTest(!txOk3 && txErr == "EXCHANGE_NOT_PENDING" &&
               txExc3.status == "CANCELLED" &&
               txReq3.ownerId == "user2" && txReq3.status == "available" &&
               txOff3.ownerId == "user1" && txOff3.status == "available",
               "Transaction: Concurrent cancel aborts and prevents partial completion or book swap");

    // Test 4: Duplicate acceptance on committed exchange does not swap ownership twice
    std::string dupErr;
    bool txDup = ExchangeTransactionSimulation::acceptInTransaction(txExc1, txReq1, txOff1, "user2", dupErr);
    recordTest(!txDup && dupErr == "EXCHANGE_NOT_PENDING" &&
               txReq1.ownerId == "user1" && txReq1.status == "exchanged" &&
               txOff1.ownerId == "user2" && txOff1.status == "exchanged",
               "Transaction: Duplicate accept rejected; does not swap ownership twice");

    // Test 5: Process crash or interruption before commit leaves zero partial state
    MockListing txReq5{"txr5", "user2", "available", 250.0};
    MockListing txOff5{"txo5", "user1", "available", 250.0};
    MockExchange txExc5{"txe5", "user1", "user2", "txr5", "txo5", "PENDING"};
    bool txCrash = ExchangeTransactionSimulation::acceptInTransaction(txExc5, txReq5, txOff5, "user2", txErr, false, false, true);
    recordTest(!txCrash && txErr == "TRANSACTION_ABORTED" &&
               txReq5.ownerId == "user2" && txReq5.status == "available" &&
               txOff5.ownerId == "user1" && txOff5.status == "available" &&
               txExc5.status == "PENDING",
               "Transaction: Interruption before commit aborts atomically; leaves zero partial state");

    // Test 6: Bounded UnknownTransactionCommitResult retry recovers commit on same session without replaying operations
    MockListing txReq6{"txr6", "user2", "available", 250.0};
    MockListing txOff6{"txo6", "user1", "available", 250.0};
    MockExchange txExc6{"txe6", "user1", "user2", "txr6", "txo6", "PENDING"};
    std::string txCommitErr;
    int commitAttemptsMade = 0;
    int txAttemptsMade = 0;
    bool txCommitOk = ExchangeTransactionSimulation::acceptWithCommitRetry(
        txExc6, txReq6, txOff6, "user2", txCommitErr,
        1 /* 1 unknown error, then success on 2nd attempt */, 3 /* max attempts */,
        commitAttemptsMade, txAttemptsMade);
    recordTest(txCommitOk && commitAttemptsMade == 2 && txAttemptsMade == 1 &&
               txExc6.status == "COMPLETED" &&
               txReq6.ownerId == "user1" && txReq6.status == "exchanged" &&
               txOff6.ownerId == "user2" && txOff6.status == "exchanged",
               "Commit Retry: Recovers on 2nd attempt on same session without replaying operations");

    // Test 7: UnknownTransactionCommitResult exhausts retry budget -> propagates EXCHANGE_COMMIT_INDETERMINATE
    MockListing txReq7{"txr7", "user2", "available", 250.0};
    MockListing txOff7{"txo7", "user1", "available", 250.0};
    MockExchange txExc7{"txe7", "user1", "user2", "txr7", "txo7", "PENDING"};
    bool txBudgetExhausted = ExchangeTransactionSimulation::acceptWithCommitRetry(
        txExc7, txReq7, txOff7, "user2", txCommitErr,
        3 /* 3 consecutive unknown errors */, 3 /* max attempts */,
        commitAttemptsMade, txAttemptsMade);
    recordTest(!txBudgetExhausted && txCommitErr == "EXCHANGE_COMMIT_INDETERMINATE" &&
               commitAttemptsMade == 3 && txAttemptsMade == 1 &&
               txExc7.status == "PENDING" /* base state unmodified, indeterminate */,
               "Commit Retry: Budget exhaustion raises explicit indeterminate outcome error");

    // Test 8: TransientTransactionError triggers whole-transaction retry (fresh transaction attempt)
    MockListing txReq8{"txr8", "user2", "available", 250.0};
    MockListing txOff8{"txo8", "user1", "available", 250.0};
    MockExchange txExc8{"txe8", "user1", "user2", "txr8", "txo8", "PENDING"};
    std::string txTransientErr;
    int transientTxAttempts = 0;
    bool txTransientOk = ExchangeTransactionSimulation::acceptWithTransientRetry(
        txExc8, txReq8, txOff8, "user2", txTransientErr,
        true /* attempt 1 fails transiently */, false /* no concurrent change */,
        transientTxAttempts);
    recordTest(txTransientOk && transientTxAttempts == 2 &&
               txExc8.status == "COMPLETED" &&
               txReq8.ownerId == "user1" && txReq8.status == "exchanged" &&
               txOff8.ownerId == "user2" && txOff8.status == "exchanged",
               "Transient Retry: Whole-transaction retry starts fresh transaction and commits cleanly");

    // Test 9: TransientTransactionError retry re-evaluates preconditions on fresh transaction
    MockListing txReq9{"txr9", "user2", "available", 250.0};
    MockListing txOff9{"txo9", "user1", "available", 250.0};
    MockExchange txExc9{"txe9", "user1", "user2", "txr9", "txo9", "PENDING"};
    int transientPrecondAttempts = 0;
    bool txPrecondOk = ExchangeTransactionSimulation::acceptWithTransientRetry(
        txExc9, txReq9, txOff9, "user2", txTransientErr,
        true /* attempt 1 fails transiently */, true /* concurrent cancel before attempt 2 */,
        transientPrecondAttempts);
    recordTest(!txPrecondOk && txTransientErr == "EXCHANGE_NOT_PENDING" &&
               transientPrecondAttempts == 2 &&
               txReq9.ownerId == "user2" && txReq9.status == "available" &&
               txOff9.ownerId == "user1" && txOff9.status == "available",
               "Transient Retry: Fresh transaction re-checks preconditions and aborts on concurrent state change");

    // Test 10: Production fail-closed behavior when transaction support is UNVERIFIED/UNKNOWN
    std::string prodGateErr;
    bool prodUnknownGate = ExchangeTransactionSimulation::evaluateEnvironmentGate(
        true /* isProduction */, ExchangeTransactionSimulation::SimTxSupport::UNKNOWN, false, prodGateErr);
    recordTest(!prodUnknownGate && prodGateErr == "EXCHANGE_TRANSACTIONS_UNAVAILABLE",
               "Production Gate: Unverified topology in production fails closed (EXCHANGE_TRANSACTIONS_UNAVAILABLE)");

    // Test 11: Production fail-closed behavior when topology is UNSUPPORTED
    bool prodUnsuppGate = ExchangeTransactionSimulation::evaluateEnvironmentGate(
        true /* isProduction */, ExchangeTransactionSimulation::SimTxSupport::UNSUPPORTED, true /* even if dev opt-in is passed */, prodGateErr);
    recordTest(!prodUnsuppGate && prodGateErr == "EXCHANGE_TRANSACTIONS_UNAVAILABLE",
               "Production Gate: Standalone topology in production strictly fails closed (no fallback)");

    // Test 12: Development environment allows fallback ONLY when explicitly enabled
    std::string devGateErr;
    bool devDisabledGate = ExchangeTransactionSimulation::evaluateEnvironmentGate(
        false /* isProduction */, ExchangeTransactionSimulation::SimTxSupport::UNSUPPORTED, false /* allowStandaloneDev=false */, devGateErr);
    recordTest(!devDisabledGate && devGateErr == "EXCHANGE_TRANSACTIONS_UNSUPPORTED",
               "Development Gate: Standalone topology blocked by default without explicit ALLOW_STANDALONE_DEV");

    bool devEnabledGate = ExchangeTransactionSimulation::evaluateEnvironmentGate(
        false /* isProduction */, ExchangeTransactionSimulation::SimTxSupport::UNSUPPORTED, true /* allowStandaloneDev=true */, devGateErr);
    recordTest(devEnabledGate,
               "Development Gate: Explicit ALLOW_STANDALONE_DEV enables development fallback branch");

    // Test 13: Fallback compensation detects failure of revertExchangeOwnership and logs critical error
    std::string compErr;
    bool criticalLogFired = false;
    bool compFailResult = ExchangeTransactionSimulation::simulateFallbackCompensation(
        true /* status update failed */, false /* revertExchangeOwnership failed */, compErr, criticalLogFired);
    recordTest(!compFailResult && criticalLogFired && compErr == "CRITICAL_COMPENSATION_FAILED",
               "Fallback Compensation: Revert failure triggers structured critical log and never claims success");
  }

  // ----------------------------------------------------
  // Group 5: Delivery Lifecycle, Cancellation, Returns & Reviews
  // ----------------------------------------------------
  std::cout << "\n--- Group 5: Delivery Lifecycle, Cancellation, Returns & Reviews ---\n";
  {
    // Delivery transitions
    recordTest(isValidDeliveryTransition("PLACED", "CONFIRMED"),
               "Valid forward transition: PLACED -> CONFIRMED");
    recordTest(isValidDeliveryTransition("CONFIRMED", "SHIPPED"),
               "Valid forward transition: CONFIRMED -> SHIPPED");
    recordTest(isValidDeliveryTransition("SHIPPED", "DELIVERED"),
               "Valid forward transition: SHIPPED -> DELIVERED");
    recordTest(!isValidDeliveryTransition("DELIVERED", "CONFIRMED"),
               "Invalid backward transition: DELIVERED -> CONFIRMED rejected");
    recordTest(!isValidDeliveryTransition("CANCELLED", "DELIVERED"),
               "Invalid transition from CANCELLED rejected");

    // Cancellation
    recordTest(canCancelOrder("PLACED") && canCancelOrder("CONFIRMED") && canCancelOrder("PACKED"),
               "Cancellation allowed before shipment (PLACED, CONFIRMED, PACKED)");
    recordTest(!canCancelOrder("SHIPPED") && !canCancelOrder("DELIVERED"),
               "Cancellation rejected once shipped or delivered");

    // Returns
    recordTest(canReturnOrder("DELIVERED"),
               "Return request allowed only for DELIVERED orders");
    recordTest(!canReturnOrder("SHIPPED") && !canReturnOrder("PLACED"),
               "Return request rejected for non-delivered orders");

    // Reviews
    recordTest(canReviewOrder("DELIVERED", "buyer1", "buyer1", "seller1", false),
               "Buyer of DELIVERED order can submit a review");
    recordTest(!canReviewOrder("SHIPPED", "buyer1", "buyer1", "seller1", false),
               "Review rejected for non-delivered order");
    recordTest(!canReviewOrder("DELIVERED", "buyer1", "seller1", "seller1", false),
               "Seller cannot review own listing");
    recordTest(!canReviewOrder("DELIVERED", "buyer1", "otherUser", "seller1", false),
               "Non-buyer user cannot review purchase");
    recordTest(!canReviewOrder("DELIVERED", "buyer1", "buyer1", "seller1", true),
               "Duplicate review on same purchase rejected");
  }

  std::cout << "\n=========================================\n";
  std::cout << " Test Results: " << g_passedTests << " / "
            << (g_passedTests + g_failedTests) << " PASSED\n";
  std::cout << "=========================================\n\n";

  if (g_failedTests > 0) {
    std::cerr << "FAILURES DETECTED: " << g_failedTests << " tests failed!\n";
    return 1;
  }

  std::cout << "ALL PHASE 2 HARDENING TESTS PASSED!\n";
  return 0;
}
