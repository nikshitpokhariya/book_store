#include "services/OrderService.h"

#include "security/PaymentService.hpp"

#include <openssl/rand.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

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

std::string generateOrderNumber() {
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

  return "ORD-" + std::string(dateBuf) + "-" + generateHex(4);
}

std::string generateTrackingId() {
  return "DEMO_TRACK_" + generateHex(8);
}

void validateShippingAddress(const ShippingAddress &addr) {
  if (addr.fullName.empty() || addr.fullName.size() > 100) {
    throw std::invalid_argument(
        "Full name is required and cannot exceed 100 characters");
  }
  if (addr.addressLine.empty() || addr.addressLine.size() > 200) {
    throw std::invalid_argument(
        "Address line is required and cannot exceed 200 characters");
  }
  if (addr.city.empty() || addr.city.size() > 50) {
    throw std::invalid_argument(
        "City is required and cannot exceed 50 characters");
  }
  if (addr.state.empty() || addr.state.size() > 50) {
    throw std::invalid_argument(
        "State is required and cannot exceed 50 characters");
  }
  if (addr.postalCode.empty() || addr.postalCode.size() > 20) {
    throw std::invalid_argument(
        "Postal code is required and cannot exceed 20 characters");
  }
  if (addr.phone.empty() || addr.phone.size() > 20) {
    throw std::invalid_argument(
        "Phone number is required and cannot exceed 20 characters");
  }
}
} // namespace

OrderService::OrderService(OrderRepository &orders, CartRepository &carts,
                           BookRepository &books, UserRepository &users)
    : orders_(orders), carts_(carts), books_(books), users_(users) {}

Order OrderService::checkout(const std::string &buyerId,
                             const CheckoutRequest &request) {
  if (buyerId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  validateShippingAddress(request.shippingAddress);

  auto buyer = users_.findById(buyerId);
  if (!buyer) {
    throw std::runtime_error("BUYER_NOT_FOUND");
  }

  // 1. Load user cart items
  const auto rawItems = carts_.getRawItems(buyerId);
  if (rawItems.empty()) {
    throw std::runtime_error("CART_IS_EMPTY");
  }

  // 2. Reload latest book documents and validate availability
  std::vector<Book> booksToBuy;
  std::vector<std::string> bookIds;
  double subtotal = 0.0;

  for (const auto &raw : rawItems) {
    auto book = books_.findById(raw.bookId);
    if (!book) {
      throw std::runtime_error("STALE_CART_ITEM: Book listing not found");
    }

    if (book->ownerId == buyerId) {
      throw std::runtime_error("CANNOT_BUY_OWN_BOOK");
    }

    if (book->status != "available") {
      throw std::runtime_error("STALE_CART_ITEM: '" + book->title +
                               "' is no longer available");
    }

    subtotal += book->price;
    booksToBuy.push_back(*book);
    bookIds.push_back(book->id);
  }

  const double shippingCost = 0.0;
  const double total = subtotal + shippingCost;

  // 3. Concurrency Protection: Atomic Acquisition
  std::vector<std::string> acquiredBookIds;
  for (const auto &b : booksToBuy) {
    const bool acquired = books_.acquireForPurchase(b.id);
    if (!acquired) {
      // Rollback any already acquired books in this batch
      for (const auto &alreadyAcquiredId : acquiredBookIds) {
        books_.releasePurchase(alreadyAcquiredId);
      }
      throw std::runtime_error(
          "BOOK_ACQUISITION_FAILED: '" + b.title +
          "' was just purchased by another buyer or is no longer available");
    }
    acquiredBookIds.push_back(b.id);
  }

  // 4. Demo Payment Processing
  PaymentResult paymentResult;
  try {
    paymentResult = PaymentService::processPayment(
        request.paymentMethod, total, request.simulatePaymentFailure);
  } catch (const std::exception &e) {
    // Rollback acquired books on exception
    for (const auto &acquiredId : acquiredBookIds) {
      books_.releasePurchase(acquiredId);
    }
    throw std::runtime_error(std::string("PAYMENT_EXCEPTION: ") + e.what());
  }

  if (!paymentResult.success) {
    // Payment failed -> rollback acquired books so they stay available
    for (const auto &acquiredId : acquiredBookIds) {
      books_.releasePurchase(acquiredId);
    }
    throw std::runtime_error("PAYMENT_FAILED: " + paymentResult.message);
  }

  // 5. Create Order with Immutable Snapshots
  Order order;
  order.orderNumber = generateOrderNumber();
  order.buyerId = buyerId;
  order.buyerUsername = buyer->username;
  order.shippingAddress = request.shippingAddress;
  order.subtotal = subtotal;
  order.shippingCost = shippingCost;
  order.total = total;
  order.status = "CONFIRMED";

  order.payment.method = paymentResult.paymentMethod;
  order.payment.status = paymentResult.paymentStatus;
  order.payment.transactionId = paymentResult.transactionId;

  order.shipping.trackingId = generateTrackingId();
  order.shipping.status = "CONFIRMED";
  order.shipping.cost = 0.0;

  for (const auto &b : booksToBuy) {
    OrderItemSnapshot snapshot;
    snapshot.bookId = b.id;
    snapshot.title = b.title;
    snapshot.author = b.author;
    snapshot.sellerId = b.ownerId;
    auto seller = users_.findById(b.ownerId);
    if (seller) {
      snapshot.sellerUsername = seller->username;
    }
    snapshot.price = b.price;
    snapshot.condition = b.condition;
    snapshot.category = b.category;
    snapshot.coverImage = b.coverImage;
    snapshot.quantity = 1;

    order.items.push_back(snapshot);
  }

  try {
    order.id = orders_.create(order);
  } catch (const std::exception &e) {
    // If order creation fails, rollback books
    for (const auto &acquiredId : acquiredBookIds) {
      books_.releasePurchase(acquiredId);
    }
    throw;
  }

  // 6. Clear purchased items from buyer's cart
  carts_.removeItems(buyerId, bookIds);

  return order;
}

PageResult<Order> OrderService::getBuyerOrders(const std::string &buyerId,
                                              int page, int limit) {
  if (buyerId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  page = std::max(1, page);
  limit = std::clamp(limit, 1, 50);

  return orders_.findByBuyer(buyerId, page, limit);
}

PageResult<Order> OrderService::getSellerSales(const std::string &sellerId,
                                              int page, int limit) {
  if (sellerId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  page = std::max(1, page);
  limit = std::clamp(limit, 1, 50);

  return orders_.findBySeller(sellerId, page, limit);
}

Order OrderService::getOrderDetails(const std::string &orderId,
                                    const std::string &userId) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  auto order = orders_.findById(orderId);
  if (!order) {
    throw std::runtime_error("ORDER_NOT_FOUND");
  }

  if (order->buyerId == userId) {
    return *order;
  }

  for (const auto &item : order->items) {
    if (item.sellerId == userId) {
      return *order;
    }
  }

  throw std::runtime_error("FORBIDDEN");
}

Order OrderService::updateDeliveryStatus(const std::string &orderId,
                                        const std::string &userId,
                                        const std::string &newStatus,
                                        const std::string &note) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  static const std::unordered_set<std::string> validStatuses = {
      "PLACED", "CONFIRMED", "PACKED", "SHIPPED", "OUT_FOR_DELIVERY",
      "DELIVERED", "CANCELLED"};

  if (validStatuses.find(newStatus) == validStatuses.end()) {
    throw std::invalid_argument("Invalid order status: " + newStatus);
  }

  auto order = orders_.findById(orderId);
  if (!order) {
    throw std::runtime_error("ORDER_NOT_FOUND");
  }

  // Authorization: must be a seller of an item in this order
  bool isSeller = false;
  for (const auto &item : order->items) {
    if (item.sellerId == userId) {
      isSeller = true;
      break;
    }
  }

  if (!isSeller) {
    throw std::runtime_error(
        "FORBIDDEN: Only sellers of order items may update delivery status");
  }

  // Enforce valid forward delivery lifecycle transitions
  auto isTransitionValid = [](const std::string &from, const std::string &to) -> bool {
    if (from == "PLACED") return to == "CONFIRMED" || to == "CANCELLED";
    if (from == "CONFIRMED") return to == "PACKED" || to == "SHIPPED" || to == "OUT_FOR_DELIVERY" || to == "DELIVERED" || to == "CANCELLED";
    if (from == "PACKED") return to == "SHIPPED" || to == "OUT_FOR_DELIVERY" || to == "DELIVERED" || to == "CANCELLED";
    if (from == "SHIPPED") return to == "OUT_FOR_DELIVERY" || to == "DELIVERED";
    if (from == "OUT_FOR_DELIVERY") return to == "DELIVERED";
    return false;
  };

  if (!isTransitionValid(order->status, newStatus)) {
    throw std::invalid_argument("Invalid status transition from " +
                                order->status + " to " + newStatus);
  }

  const bool updated = orders_.updateStatus(orderId, newStatus, note);
  if (!updated) {
    throw std::runtime_error("STATUS_UPDATE_FAILED");
  }

  auto refreshed = orders_.findById(orderId);
  if (!refreshed) {
    throw std::runtime_error("ORDER_NOT_FOUND");
  }

  return *refreshed;
}

Order OrderService::cancelOrder(const std::string &orderId,
                                const std::string &userId,
                                const std::string &reason) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  auto order = orders_.findById(orderId);
  if (!order) {
    throw std::runtime_error("ORDER_NOT_FOUND");
  }

  // Authorization: buyer or seller
  bool isBuyer = (order->buyerId == userId);
  bool isSeller = false;
  for (const auto &item : order->items) {
    if (item.sellerId == userId) {
      isSeller = true;
      break;
    }
  }

  if (!isBuyer && !isSeller) {
    throw std::runtime_error(
        "FORBIDDEN: You do not have permission to cancel this order");
  }

  // Valid states for cancellation: before SHIPPED (PLACED, CONFIRMED, PACKED)
  if (order->status != "PLACED" && order->status != "CONFIRMED" &&
      order->status != "PACKED") {
    throw std::invalid_argument("Order cannot be cancelled in status: " +
                                order->status);
  }

  // Determine payment status
  std::string paymentStatus = order->payment.status;
  if (order->payment.status == "paid") {
    paymentStatus = "refunded";
  } else if (order->payment.status == "pending") {
    paymentStatus = "cancelled";
  }

  // Restore inventory availability for physical books
  for (const auto &item : order->items) {
    books_.restoreAvailability(item.bookId);
  }

  const std::string note =
      reason.empty() ? (isBuyer ? "Order cancelled by buyer"
                                : "Order cancelled by seller")
                     : ("Cancelled: " + reason);

  const bool updated = orders_.updateOrderStatusAndPayment(
      orderId, "CANCELLED", paymentStatus, note);
  if (!updated) {
    throw std::runtime_error("STATUS_UPDATE_FAILED");
  }

  auto refreshed = orders_.findById(orderId);
  if (!refreshed) {
    throw std::runtime_error("ORDER_NOT_FOUND");
  }

  return *refreshed;
}

Order OrderService::requestReturn(const std::string &orderId,
                                  const std::string &userId,
                                  const std::string &reason) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  auto order = orders_.findById(orderId);
  if (!order) {
    throw std::runtime_error("ORDER_NOT_FOUND");
  }

  // Authorization: buyer only
  if (order->buyerId != userId) {
    throw std::runtime_error(
        "FORBIDDEN: Only the buyer can request a return for this order");
  }

  // Lifecycle check: must be DELIVERED
  if (order->status != "DELIVERED") {
    throw std::invalid_argument(
        "Returns can only be requested for DELIVERED orders (current: " +
        order->status + ")");
  }

  const std::string note =
      reason.empty() ? "Return requested by buyer" : ("Return: " + reason);

  const bool updated =
      orders_.updateStatus(orderId, "RETURN_REQUESTED", note);
  if (!updated) {
    throw std::runtime_error("STATUS_UPDATE_FAILED");
  }

  auto refreshed = orders_.findById(orderId);
  if (!refreshed) {
    throw std::runtime_error("ORDER_NOT_FOUND");
  }

  return *refreshed;
}

Order OrderService::processReturn(const std::string &orderId,
                                  const std::string &userId, bool approve,
                                  const std::string &note) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  auto order = orders_.findById(orderId);
  if (!order) {
    throw std::runtime_error("ORDER_NOT_FOUND");
  }

  // Authorization: seller only
  bool isSeller = false;
  for (const auto &item : order->items) {
    if (item.sellerId == userId) {
      isSeller = true;
      break;
    }
  }

  if (!isSeller) {
    throw std::runtime_error(
        "FORBIDDEN: Only sellers of order items may approve or reject returns");
  }

  // Must be in RETURN_REQUESTED
  if (order->status != "RETURN_REQUESTED") {
    throw std::invalid_argument(
        "Order is not in RETURN_REQUESTED status (current: " + order->status +
        ")");
  }

  if (approve) {
    // Mark books as returned (conservative state - not automatically available)
    for (const auto &item : order->items) {
      books_.markReturned(item.bookId);
    }

    const std::string auditNote =
        note.empty() ? "Return approved by seller; refunded"
                     : ("Return approved: " + note);

    const bool updated = orders_.updateOrderStatusAndPayment(
        orderId, "RETURNED", "refunded", auditNote);
    if (!updated) {
      throw std::runtime_error("STATUS_UPDATE_FAILED");
    }
  } else {
    const std::string auditNote =
        note.empty() ? "Return rejected by seller" : ("Return rejected: " + note);

    const bool updated =
        orders_.updateStatus(orderId, "RETURN_REJECTED", auditNote);
    if (!updated) {
      throw std::runtime_error("STATUS_UPDATE_FAILED");
    }
  }

  auto refreshed = orders_.findById(orderId);
  if (!refreshed) {
    throw std::runtime_error("ORDER_NOT_FOUND");
  }

  return *refreshed;
}

