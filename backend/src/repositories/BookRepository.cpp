#include "repositories/BookRepository.h"
#include "db/MongoDatabase.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/types.hpp>

#include <mongocxx/options/find.hpp>

using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;

namespace {

Book documentToBook(const bsoncxx::document::view &doc) {
  Book book;

  auto id = doc["_id"];
  if (id && id.type() == bsoncxx::type::k_oid) {
    book.id = id.get_oid().value.to_string();
  }

  auto ownerId = doc["ownerId"];
  if (ownerId && ownerId.type() == bsoncxx::type::k_string) {
    book.ownerId = std::string(ownerId.get_string().value);
  }

  auto title = doc["title"];
  if (title && title.type() == bsoncxx::type::k_string) {
    book.title = std::string(title.get_string().value);
  }

  auto author = doc["author"];
  if (author && author.type() == bsoncxx::type::k_string) {
    book.author = std::string(author.get_string().value);
  }

  auto isbn = doc["isbn"];
  if (isbn && isbn.type() == bsoncxx::type::k_string) {
    book.isbn = std::string(isbn.get_string().value);
  }

  auto description = doc["description"];
  if (description && description.type() == bsoncxx::type::k_string) {
    book.description = std::string(description.get_string().value);
  }

  auto genre = doc["genre"];
  if (genre && genre.type() == bsoncxx::type::k_string) {
    book.genre = std::string(genre.get_string().value);
  }

  auto language = doc["language"];
  if (language && language.type() == bsoncxx::type::k_string) {
    book.language = std::string(language.get_string().value);
  }

  auto condition = doc["condition"];
  if (condition && condition.type() == bsoncxx::type::k_string) {
    book.condition = std::string(condition.get_string().value);
  }

  auto listingType = doc["listingType"];
  if (listingType && listingType.type() == bsoncxx::type::k_string) {
    book.listingType = std::string(listingType.get_string().value);
  }

  auto price = doc["price"];
  if (price) {
    if (price.type() == bsoncxx::type::k_double) {
      book.price = price.get_double().value;
    } else if (price.type() == bsoncxx::type::k_int32) {
      book.price = static_cast<double>(price.get_int32().value);
    } else if (price.type() == bsoncxx::type::k_int64) {
      book.price = static_cast<double>(price.get_int64().value);
    }
  }

  auto currency = doc["currency"];
  if (currency && currency.type() == bsoncxx::type::k_string) {
    book.currency = std::string(currency.get_string().value);
  }

  auto exchangePreferences = doc["exchangePreferences"];
  if (exchangePreferences &&
      exchangePreferences.type() == bsoncxx::type::k_string) {
    book.exchangePreferences =
        std::string(exchangePreferences.get_string().value);
  }

  auto city = doc["city"];
  if (city && city.type() == bsoncxx::type::k_string) {
    book.city = std::string(city.get_string().value);
  }

  auto status = doc["status"];
  if (status && status.type() == bsoncxx::type::k_string) {
    book.status = std::string(status.get_string().value);
  }

  auto images = doc["images"];
  if (images && images.type() == bsoncxx::type::k_array) {
    for (auto &&image : images.get_array().value) {
      if (image.type() == bsoncxx::type::k_string) {
        book.images.push_back(std::string(image.get_string().value));
      }
    }
  }

  auto createdAt = doc["createdAt"];
  if (createdAt && createdAt.type() == bsoncxx::type::k_date) {
    book.createdAt =
        std::chrono::system_clock::time_point{createdAt.get_date().value};
  }

  auto updatedAt = doc["updatedAt"];
  if (updatedAt && updatedAt.type() == bsoncxx::type::k_date) {
    book.updatedAt =
        std::chrono::system_clock::time_point{updatedAt.get_date().value};
  }

  return book;
}

bsoncxx::builder::basic::document bookToDocument(const Book &book) {
  document doc;

  doc.append(kvp("ownerId", book.ownerId), kvp("title", book.title),
             kvp("author", book.author), kvp("isbn", book.isbn),
             kvp("description", book.description), kvp("genre", book.genre),
             kvp("language", book.language), kvp("condition", book.condition),
             kvp("listingType", book.listingType),
             kvp("currency", book.currency),
             kvp("exchangePreferences", book.exchangePreferences),
             kvp("city", book.city), kvp("status", book.status));

  if (book.price.has_value()) {
    doc.append(kvp("price", *book.price));
  }

  bsoncxx::builder::basic::array imageArray;
  for (const auto &image : book.images) {
    imageArray.append(image);
  }
  doc.append(kvp("images", imageArray));

  if (book.createdAt != std::chrono::system_clock::time_point{}) {
    doc.append(kvp("createdAt", bsoncxx::types::b_date{book.createdAt}));
  }
  if (book.updatedAt != std::chrono::system_clock::time_point{}) {
    doc.append(kvp("updatedAt", bsoncxx::types::b_date{book.updatedAt}));
  }

  return doc;
}

} // namespace

std::string BookRepository::create(const Book &book) {
  auto collection = MongoDatabase::instance().database()["books"];

  auto doc = bookToDocument(book);

  auto result = collection.insert_one(doc.view());

  if (!result) {
    throw std::runtime_error("Failed to insert book");
  }

  return result->inserted_id().get_oid().value.to_string();
}

std::optional<Book> BookRepository::findById(const std::string &id) {
  bsoncxx::oid oid;
  try {
    oid = bsoncxx::oid{id};
  } catch (const std::exception &) {
    return std::nullopt;
  }

  auto collection = MongoDatabase::instance().database()["books"];

  document filter;
  filter.append(kvp("_id", oid));

  auto result = collection.find_one(filter.view());

  if (!result) {
    return std::nullopt;
  }

  return documentToBook(result->view());
}

std::vector<Book>
BookRepository::findAll(const std::string &search, const std::string &genre,
                        const std::string &condition,
                        const std::string &listingType, const std::string &city,
                        std::optional<double> minPrice,
                        std::optional<double> maxPrice, const std::string &sort,
                        int page, int limit) {
  auto collection = MongoDatabase::instance().database()["books"];

  document filter;

  filter.append(kvp("status", "AVAILABLE"));

  if (!search.empty()) {
    document regex;
    regex.append(kvp("$regex", search), kvp("$options", "i"));

    bsoncxx::builder::basic::array orArray;

    document titleFilter;
    titleFilter.append(kvp("title", regex.view()));
    orArray.append(titleFilter.view());

    document authorFilter;
    authorFilter.append(kvp("author", regex.view()));
    orArray.append(authorFilter.view());

    filter.append(kvp("$or", orArray));
  }

  if (!genre.empty()) {
    filter.append(kvp("genre", genre));
  }

  if (!condition.empty()) {
    filter.append(kvp("condition", condition));
  }

  if (!listingType.empty()) {
    filter.append(kvp("listingType", listingType));
  }

  if (!city.empty()) {
    filter.append(kvp("city", city));
  }

  if (minPrice.has_value() || maxPrice.has_value()) {
    document priceFilter;

    if (minPrice.has_value()) {
      priceFilter.append(kvp("$gte", *minPrice));
    }

    if (maxPrice.has_value()) {
      priceFilter.append(kvp("$lte", *maxPrice));
    }

    filter.append(kvp("price", priceFilter.view()));
  }

  mongocxx::options::find options;

  document sortDoc;
  if (sort == "price_asc") {
    sortDoc.append(kvp("price", 1));
  } else if (sort == "price_desc") {
    sortDoc.append(kvp("price", -1));
  } else {
    sortDoc.append(kvp("createdAt", -1));
  }
  options.sort(sortDoc.view());

  const int skip = (page - 1) * limit;

  options.skip(skip);
  options.limit(limit);

  std::vector<Book> books;

  auto cursor = collection.find(filter.view(), options);

  for (auto &&doc : cursor) {
    books.push_back(documentToBook(doc));
  }

  return books;
}

std::vector<Book> BookRepository::findByOwner(const std::string &ownerId,
                                              int page, int limit) {
  auto collection = MongoDatabase::instance().database()["books"];

  mongocxx::options::find options;

  options.skip((page - 1) * limit);
  options.limit(limit);

  document sortDoc;
  sortDoc.append(kvp("createdAt", -1));
  options.sort(sortDoc.view());

  std::vector<Book> books;

  document ownerFilter;
  ownerFilter.append(kvp("ownerId", ownerId));

  auto cursor = collection.find(ownerFilter.view(), options);

  for (auto &&doc : cursor) {
    books.push_back(documentToBook(doc));
  }

  return books;
}

bool BookRepository::update(const std::string &id, const Book &book) {
  bsoncxx::oid oid;
  try {
    oid = bsoncxx::oid{id};
  } catch (const std::exception &) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["books"];

  document update;
  update.append(kvp("$set", bookToDocument(book).view()));

  document idFilter;
  idFilter.append(kvp("_id", oid));

  auto result = collection.update_one(idFilter.view(), update.view());

  return result && result->modified_count() > 0;
}

bool BookRepository::remove(const std::string &id) {
  bsoncxx::oid oid;
  try {
    oid = bsoncxx::oid{id};
  } catch (const std::exception &) {
    return false;
  }

  auto collection = MongoDatabase::instance().database()["books"];

  document idFilter;
  idFilter.append(kvp("_id", oid));

  auto result = collection.delete_one(idFilter.view());

  return result && result->deleted_count() > 0;
}

long long BookRepository::count(const std::string &search,
                                const std::string &genre,
                                const std::string &condition,
                                const std::string &listingType,
                                const std::string &city,
                                std::optional<double> minPrice,
                                std::optional<double> maxPrice) {
  auto collection = MongoDatabase::instance().database()["books"];

  document filter;

  filter.append(kvp("status", "AVAILABLE"));

  if (!search.empty()) {
    document regex;
    regex.append(kvp("$regex", search), kvp("$options", "i"));

    bsoncxx::builder::basic::array orArray;

    document titleFilter;
    titleFilter.append(kvp("title", regex.view()));
    orArray.append(titleFilter.view());

    document authorFilter;
    authorFilter.append(kvp("author", regex.view()));
    orArray.append(authorFilter.view());

    filter.append(kvp("$or", orArray));
  }

  if (!genre.empty())
    filter.append(kvp("genre", genre));

  if (!condition.empty())
    filter.append(kvp("condition", condition));

  if (!listingType.empty())
    filter.append(kvp("listingType", listingType));

  if (!city.empty())
    filter.append(kvp("city", city));

  if (minPrice.has_value() || maxPrice.has_value()) {
    document price;

    if (minPrice.has_value())
      price.append(kvp("$gte", *minPrice));

    if (maxPrice.has_value())
      price.append(kvp("$lte", *maxPrice));

    filter.append(kvp("price", price.view()));
  }

  return static_cast<long long>(collection.count_documents(filter.view()));
}