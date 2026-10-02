#pragma once

#include "models/Cart.h"
#include "repositories/BookRepository.h"
#include "repositories/CartRepository.h"
#include "repositories/UserRepository.h"

#include <string>

class CartService {
public:
  CartService(CartRepository &carts, BookRepository &books,
              UserRepository &users);

  Cart getCart(const std::string &userId);

  void addItem(const std::string &userId, const std::string &bookId);

  void removeItem(const std::string &userId, const std::string &bookId);

  void clearCart(const std::string &userId);

private:
  CartRepository &carts_;
  BookRepository &books_;
  UserRepository &users_;
};
