#include "services/CartService.h"

#include <stdexcept>

CartService::CartService(CartRepository &carts, BookRepository &books,
                         UserRepository &users)
    : carts_(carts), books_(books), users_(users) {}

Cart CartService::getCart(const std::string &userId) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  const auto rawItems = carts_.getRawItems(userId);

  Cart cart;
  cart.userId = userId;
  cart.subtotal = 0.0;
  cart.shippingCost = 0.0;
  cart.hasUnavailableItems = false;

  for (const auto &raw : rawItems) {
    CartItemDetail detail;
    detail.bookId = raw.bookId;
    detail.addedAt = raw.addedAt;

    auto book = books_.findById(raw.bookId);
    if (!book) {
      detail.title = "[Unknown Listing]";
      detail.status = "not_found";
      detail.isAvailable = false;
      cart.hasUnavailableItems = true;
    } else {
      detail.title = book->title;
      detail.author = book->author;
      detail.price = book->price;
      detail.condition = book->condition;
      detail.category = book->category;
      detail.coverImage = book->coverImage;
      detail.status = book->status;
      detail.ownerId = book->ownerId;

      auto owner = users_.findById(book->ownerId);
      if (owner) {
        detail.ownerUsername = owner->username;
      }

      if (book->status == "available") {
        detail.isAvailable = true;
        cart.subtotal += book->price;
      } else {
        detail.isAvailable = false;
        cart.hasUnavailableItems = true;
      }
    }

    cart.items.push_back(detail);
  }

  cart.total = cart.subtotal + cart.shippingCost;
  return cart;
}

void CartService::addItem(const std::string &userId,
                          const std::string &bookId) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  auto book = books_.findById(bookId);
  if (!book) {
    throw std::runtime_error("BOOK_NOT_FOUND");
  }

  if (book->ownerId == userId) {
    throw std::runtime_error("CANNOT_BUY_OWN_BOOK");
  }

  if (book->status != "available") {
    throw std::runtime_error("BOOK_NOT_AVAILABLE");
  }

  const bool added = carts_.addItem(userId, bookId);
  if (!added) {
    throw std::runtime_error("ITEM_ALREADY_IN_CART");
  }
}

void CartService::removeItem(const std::string &userId,
                             const std::string &bookId) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  carts_.removeItem(userId, bookId);
}

void CartService::clearCart(const std::string &userId) {
  if (userId.empty()) {
    throw std::runtime_error("UNAUTHORIZED");
  }

  carts_.clearCart(userId);
}
