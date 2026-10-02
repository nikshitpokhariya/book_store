#pragma once

#include "models/Cart.h"

#include <string>
#include <vector>

class CartRepository {
public:
  std::vector<CartItem> getRawItems(const std::string &userId);

  bool addItem(const std::string &userId, const std::string &bookId);

  bool removeItem(const std::string &userId, const std::string &bookId);

  bool removeItems(const std::string &userId,
                   const std::vector<std::string> &bookIds);

  bool clearCart(const std::string &userId);
};
