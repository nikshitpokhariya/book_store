#include "repositories/OrderRepository.h"

#include "db/MongoDatabase.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/options/find.hpp>

#include <cmath>

using bsoncxx::builder::basic::array;
using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace {
std::string getString(const bsoncxx::document::view &view, const char *key) {
  if (view[key] && view[key].type() == bsoncxx::type::k_string) {
    return std::string(view[key].get_string().value.data(),
                       view[key].get_string().value.size());
  }
  return "";
}

double getDouble(const bsoncxx::document::view &view, const char *key) {
  if (!view[key])
    return 0.0;
  if (view[key].type() == bsoncxx::type::k_double) {
    return view[key].get_double().value;
  }
  if (view[key].type() == bsoncxx::type::k_int64) {
    return static_cast<double>(view[key].get_int64().value);
  }
  if (view[key].type() == bsoncxx::type::k_int32) {
    return static_cast<double>(view[key].get_int32().value);
  }
  return 0.0;
}

Order toOrder(const bsoncxx::document::view &view) {
  Order order;
  order.id = view["_id"].get_oid().value.to_string();
  order.orderNumber = getString(view, "orderNumber");
  order.buyerId = getString(view, "buyerId");
  order.buyerUsername = getString(view, "buyerUsername");
  order.subtotal = getDouble(view, "subtotal");
  order.shippingCost = getDouble(view, "shippingCost");
  order.total = getDouble(view, "total");
  order.status = getString(view, "status");

  if (view["createdAt"] && view["createdAt"].type() == bsoncxx::type::k_date) {
    order.createdAt = std::chrono::system_clock::time_point{
        view["createdAt"].get_date().value};
  }
  if (view["updatedAt"] && view["updatedAt"].type() == bsoncxx::type::k_date) {
    order.updatedAt = std::chrono::system_clock::time_point{
        view["updatedAt"].get_date().value};
  }

  // Items
  if (view["items"] && view["items"].type() == bsoncxx::type::k_array) {
    for (const auto &elem : view["items"].get_array().value) {
      if (elem.type() == bsoncxx::type::k_document) {
        auto doc = elem.get_document().value;
        OrderItemSnapshot item;
        item.bookId = getString(doc, "bookId");
        item.title = getString(doc, "title");
        item.author = getString(doc, "author");
        item.sellerId = getString(doc, "sellerId");
        item.sellerUsername = getString(doc, "sellerUsername");
        item.price = getDouble(doc, "price");
        item.condition = getString(doc, "condition");
        item.category = getString(doc, "category");
        item.coverImage = getString(doc, "coverImage");
        item.quantity = 1;
        order.items.push_back(item);
      }
    }
  }

  // Shipping Address
  if (view["shippingAddress"] &&
      view["shippingAddress"].type() == bsoncxx::type::k_document) {
    auto addrDoc = view["shippingAddress"].get_document().value;
    order.shippingAddress.fullName = getString(addrDoc, "fullName");
    order.shippingAddress.addressLine = getString(addrDoc, "addressLine");
    order.shippingAddress.city = getString(addrDoc, "city");
    order.shippingAddress.state = getString(addrDoc, "state");
    order.shippingAddress.postalCode = getString(addrDoc, "postalCode");
    order.shippingAddress.phone = getString(addrDoc, "phone");
  }

  // Payment
  if (view["payment"] && view["payment"].type() == bsoncxx::type::k_document) {
    auto payDoc = view["payment"].get_document().value;
    order.payment.method = getString(payDoc, "method");
    order.payment.status = getString(payDoc, "status");
    order.payment.transactionId = getString(payDoc, "transactionId");
    if (payDoc["paidAt"] && payDoc["paidAt"].type() == bsoncxx::type::k_date) {
      order.payment.paidAt = std::chrono::system_clock::time_point{
          payDoc["paidAt"].get_date().value};
    }
  }

  // Shipping Info
  if (view["shipping"] &&
      view["shipping"].type() == bsoncxx::type::k_document) {
    auto shipDoc = view["shipping"].get_document().value;
    order.shipping.trackingId = getString(shipDoc, "trackingId");
    order.shipping.status = getString(shipDoc, "status");
    order.shipping.cost = getDouble(shipDoc, "cost");
    if (shipDoc["updatedAt"] &&
        shipDoc["updatedAt"].type() == bsoncxx::type::k_date) {
      order.shipping.updatedAt = std::chrono::system_clock::time_point{
          shipDoc["updatedAt"].get_date().value};
    }
  }

  // History
  if (view["history"] && view["history"].type() == bsoncxx::type::k_array) {
    for (const auto &elem : view["history"].get_array().value) {
      if (elem.type() == bsoncxx::type::k_document) {
        auto doc = elem.get_document().value;
        OrderStatusHistory h;
        h.status = getString(doc, "status");
        h.note = getString(doc, "note");
        if (doc["timestamp"] &&
            doc["timestamp"].type() == bsoncxx::type::k_date) {
          h.timestamp = std::chrono::system_clock::time_point{
              doc["timestamp"].get_date().value};
        }
        order.history.push_back(h);
      }
    }
  }

  return order;
}
} // namespace

std::string OrderRepository::create(const Order &order) {
  auto collection = MongoDatabase::instance().database()["orders"];

  const auto now = std::chrono::system_clock::now();

  document doc;
  doc.append(kvp("orderNumber", order.orderNumber));
  doc.append(kvp("buyerId", order.buyerId));
  doc.append(kvp("buyerUsername", order.buyerUsername));
  doc.append(kvp("subtotal", order.subtotal));
  doc.append(kvp("shippingCost", order.shippingCost));
  doc.append(kvp("total", order.total));
  doc.append(kvp("status", order.status.empty() ? "PLACED" : order.status));
  doc.append(kvp("createdAt", bsoncxx::types::b_date{now}));
  doc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  // Items
  array itemsArray;
  for (const auto &item : order.items) {
    document itemDoc;
    itemDoc.append(kvp("bookId", item.bookId));
    itemDoc.append(kvp("title", item.title));
    itemDoc.append(kvp("author", item.author));
    itemDoc.append(kvp("sellerId", item.sellerId));
    itemDoc.append(kvp("sellerUsername", item.sellerUsername));
    itemDoc.append(kvp("price", item.price));
    itemDoc.append(kvp("condition", item.condition));
    itemDoc.append(kvp("category", item.category));
    itemDoc.append(kvp("coverImage", item.coverImage));
    itemDoc.append(kvp("quantity", 1));
    itemsArray.append(itemDoc.view());
  }
  doc.append(kvp("items", itemsArray.view()));

  // Shipping Address
  document addrDoc;
  addrDoc.append(kvp("fullName", order.shippingAddress.fullName));
  addrDoc.append(kvp("addressLine", order.shippingAddress.addressLine));
  addrDoc.append(kvp("city", order.shippingAddress.city));
  addrDoc.append(kvp("state", order.shippingAddress.state));
  addrDoc.append(kvp("postalCode", order.shippingAddress.postalCode));
  addrDoc.append(kvp("phone", order.shippingAddress.phone));
  doc.append(kvp("shippingAddress", addrDoc.view()));

  // Payment
  document payDoc;
  payDoc.append(kvp("method", order.payment.method));
  payDoc.append(kvp("status", order.payment.status));
  payDoc.append(kvp("transactionId", order.payment.transactionId));
  payDoc.append(kvp("paidAt", bsoncxx::types::b_date{now}));
  doc.append(kvp("payment", payDoc.view()));

  // Shipping Info
  document shipDoc;
  shipDoc.append(kvp("trackingId", order.shipping.trackingId));
  shipDoc.append(kvp("status", order.status.empty() ? "PLACED" : order.status));
  shipDoc.append(kvp("cost", 0.0));
  shipDoc.append(kvp("updatedAt", bsoncxx::types::b_date{now}));
  doc.append(kvp("shipping", shipDoc.view()));

  // Initial history record
  array histArray;
  document initHist;
  initHist.append(kvp("status", order.status.empty() ? "PLACED" : order.status));
  initHist.append(kvp("timestamp", bsoncxx::types::b_date{now}));
  initHist.append(kvp("note", "Order placed"));
  histArray.append(initHist.view());
  doc.append(kvp("history", histArray.view()));

  auto result = collection.insert_one(doc.view());
  if (!result) {
    throw std::runtime_error("ORDER_CREATE_FAILED");
  }

  return result->inserted_id().get_oid().value.to_string();
}

std::optional<Order> OrderRepository::findById(const std::string &orderId) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{orderId};
  } catch (const std::exception &) {
    return std::nullopt;
  }

  auto collection = MongoDatabase::instance().database()["orders"];

  document filter;
  filter.append(kvp("_id", objectId));

  auto result = collection.find_one(filter.view());
  if (!result) {
    return std::nullopt;
  }

  return toOrder(result->view());
}

std::optional<Order>
OrderRepository::findByOrderNumber(const std::string &orderNumber) {
  auto collection = MongoDatabase::instance().database()["orders"];

  document filter;
  filter.append(kvp("orderNumber", orderNumber));

  auto result = collection.find_one(filter.view());
  if (!result) {
    return std::nullopt;
  }

  return toOrder(result->view());
}

PageResult<Order> OrderRepository::findByBuyer(const std::string &buyerId,
                                              int page, int limit) {
  auto collection = MongoDatabase::instance().database()["orders"];

  document filter;
  filter.append(kvp("buyerId", buyerId));

  const std::int64_t total = collection.count_documents(filter.view());

  mongocxx::options::find options;
  const auto skipCount = static_cast<std::int64_t>((page - 1) * limit);
  options.skip(skipCount);
  options.limit(static_cast<std::int64_t>(limit));

  document sortDoc;
  sortDoc.append(kvp("createdAt", -1));
  options.sort(sortDoc.view());

  auto cursor = collection.find(filter.view(), options);

  PageResult<Order> result;
  result.total = total;
  result.page = page;
  result.limit = limit;
  result.totalPages =
      limit > 0 ? static_cast<int>(std::ceil(static_cast<double>(total) / limit))
                : 0;
  result.hasNextPage = result.page < result.totalPages;
  result.hasPrevPage = result.page > 1 && result.totalPages > 0;

  for (const auto &docView : cursor) {
    result.items.push_back(toOrder(docView));
  }

  return result;
}

PageResult<Order> OrderRepository::findBySeller(const std::string &sellerId,
                                               int page, int limit) {
  auto collection = MongoDatabase::instance().database()["orders"];

  document filter;
  filter.append(kvp("items.sellerId", sellerId));

  const std::int64_t total = collection.count_documents(filter.view());

  mongocxx::options::find options;
  const auto skipCount = static_cast<std::int64_t>((page - 1) * limit);
  options.skip(skipCount);
  options.limit(static_cast<std::int64_t>(limit));

  document sortDoc;
  sortDoc.append(kvp("createdAt", -1));
  options.sort(sortDoc.view());

  auto cursor = collection.find(filter.view(), options);

  PageResult<Order> result;
  result.total = total;
  result.page = page;
  result.limit = limit;
  result.totalPages =
      limit > 0 ? static_cast<int>(std::ceil(static_cast<double>(total) / limit))
                : 0;
  result.hasNextPage = result.page < result.totalPages;
  result.hasPrevPage = result.page > 1 && result.totalPages > 0;

  for (const auto &docView : cursor) {
    result.items.push_back(toOrder(docView));
  }

  return result;
}

bool OrderRepository::updateStatus(const std::string &orderId,
                                  const std::string &newStatus,
                                  const std::string &note) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{orderId};
  } catch (const std::exception &) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["orders"];

  const auto now = std::chrono::system_clock::now();

  document histEntry;
  histEntry.append(kvp("status", newStatus));
  histEntry.append(kvp("timestamp", bsoncxx::types::b_date{now}));
  histEntry.append(kvp("note", note));

  document setFields;
  setFields.append(kvp("status", newStatus));
  setFields.append(kvp("shipping.status", newStatus));
  setFields.append(kvp("shipping.updatedAt", bsoncxx::types::b_date{now}));
  setFields.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  document updateDoc;
  updateDoc.append(kvp("$set", setFields.view()));
  updateDoc.append(kvp("$push", make_document(kvp("history", histEntry.view()))));

  document filter;
  filter.append(kvp("_id", objectId));

  auto result = collection.update_one(filter.view(), updateDoc.view());
  return result && result->modified_count() > 0;
}

bool OrderRepository::updateOrderStatusAndPayment(
    const std::string &orderId, const std::string &newStatus,
    const std::string &paymentStatus, const std::string &note) {
  bsoncxx::oid objectId;
  try {
    objectId = bsoncxx::oid{orderId};
  } catch (const std::exception &) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["orders"];

  const auto now = std::chrono::system_clock::now();

  document histEntry;
  histEntry.append(kvp("status", newStatus));
  histEntry.append(kvp("timestamp", bsoncxx::types::b_date{now}));
  histEntry.append(kvp("note", note));

  document setFields;
  setFields.append(kvp("status", newStatus));
  setFields.append(kvp("shipping.status", newStatus));
  setFields.append(kvp("shipping.updatedAt", bsoncxx::types::b_date{now}));
  setFields.append(kvp("payment.status", paymentStatus));
  setFields.append(kvp("updatedAt", bsoncxx::types::b_date{now}));

  document updateDoc;
  updateDoc.append(kvp("$set", setFields.view()));
  updateDoc.append(
      kvp("$push", make_document(kvp("history", histEntry.view()))));

  document filter;
  filter.append(kvp("_id", objectId));

  auto result = collection.update_one(filter.view(), updateDoc.view());
  return result && result->modified_count() > 0;
}

