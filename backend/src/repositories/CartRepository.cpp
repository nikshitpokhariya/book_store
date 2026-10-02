#include "repositories/CartRepository.h"

#include "db/MongoDatabase.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/options/update.hpp>

using bsoncxx::builder::basic::array;
using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

std::vector<CartItem> CartRepository::getRawItems(const std::string &userId) {
  auto collection = MongoDatabase::instance().database()["carts"];

  document filter;
  filter.append(kvp("userId", userId));

  auto result = collection.find_one(filter.view());
  if (!result) {
    return {};
  }

  std::vector<CartItem> items;
  auto view = result->view();
  if (view["items"] && view["items"].type() == bsoncxx::type::k_array) {
    for (const auto &element : view["items"].get_array().value) {
      if (element.type() == bsoncxx::type::k_document) {
        auto doc = element.get_document().value;
        CartItem item;
        if (doc["bookId"] && doc["bookId"].type() == bsoncxx::type::k_string) {
          item.bookId = std::string(doc["bookId"].get_string().value.data(),
                                    doc["bookId"].get_string().value.size());
        }
        if (doc["addedAt"] && doc["addedAt"].type() == bsoncxx::type::k_date) {
          item.addedAt = std::chrono::system_clock::time_point{
              doc["addedAt"].get_date().value};
        }
        if (!item.bookId.empty()) {
          items.push_back(item);
        }
      }
    }
  }

  return items;
}

bool CartRepository::addItem(const std::string &userId,
                             const std::string &bookId) {
  auto collection = MongoDatabase::instance().database()["carts"];

  // Check if item already exists in user's cart
  document checkFilter;
  checkFilter.append(kvp("userId", userId));
  checkFilter.append(kvp("items.bookId", bookId));

  auto existing = collection.find_one(checkFilter.view());
  if (existing) {
    return false; // Already in cart (single physical copy limit)
  }

  const auto now = std::chrono::system_clock::now();

  document itemDoc;
  itemDoc.append(kvp("bookId", bookId));
  itemDoc.append(kvp("addedAt", bsoncxx::types::b_date{now}));

  document filter;
  filter.append(kvp("userId", userId));

  document pushDoc;
  pushDoc.append(kvp("items", itemDoc.view()));

  document setDoc;
  setDoc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  document updateDoc;
  updateDoc.append(kvp("$push", pushDoc.view()));
  updateDoc.append(kvp("$set", setDoc.view()));

  mongocxx::options::update options;
  options.upsert(true);

  auto result = collection.update_one(filter.view(), updateDoc.view(), options);
  return result.has_value();
}

bool CartRepository::removeItem(const std::string &userId,
                                const std::string &bookId) {
  auto collection = MongoDatabase::instance().database()["carts"];

  const auto now = std::chrono::system_clock::now();

  document pullCond;
  pullCond.append(kvp("bookId", bookId));

  document pullDoc;
  pullDoc.append(kvp("items", pullCond.view()));

  document setDoc;
  setDoc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  document updateDoc;
  updateDoc.append(kvp("$pull", pullDoc.view()));
  updateDoc.append(kvp("$set", setDoc.view()));

  document filter;
  filter.append(kvp("userId", userId));

  auto result = collection.update_one(filter.view(), updateDoc.view());
  return result && result->modified_count() > 0;
}

bool CartRepository::removeItems(const std::string &userId,
                                 const std::vector<std::string> &bookIds) {
  if (bookIds.empty()) {
    return true;
  }

  auto collection = MongoDatabase::instance().database()["carts"];

  const auto now = std::chrono::system_clock::now();

  array inArray;
  for (const auto &id : bookIds) {
    inArray.append(id);
  }

  document inDoc;
  inDoc.append(kvp("$in", inArray.view()));

  document pullCond;
  pullCond.append(kvp("bookId", inDoc.view()));

  document pullDoc;
  pullDoc.append(kvp("items", pullCond.view()));

  document setDoc;
  setDoc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  document updateDoc;
  updateDoc.append(kvp("$pull", pullDoc.view()));
  updateDoc.append(kvp("$set", setDoc.view()));

  document filter;
  filter.append(kvp("userId", userId));

  auto result = collection.update_one(filter.view(), updateDoc.view());
  return result.has_value();
}

bool CartRepository::clearCart(const std::string &userId) {
  auto collection = MongoDatabase::instance().database()["carts"];

  const auto now = std::chrono::system_clock::now();

  array emptyArray;

  document setDoc;
  setDoc.append(kvp("items", emptyArray.view()));
  setDoc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  document updateDoc;
  updateDoc.append(kvp("$set", setDoc.view()));

  document filter;
  filter.append(kvp("userId", userId));

  auto result = collection.update_one(filter.view(), updateDoc.view());
  return result.has_value();
}
