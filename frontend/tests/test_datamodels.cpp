#include "test_datamodels.h"
#include <QTest>
#include "models/DataModels.h"

void TestDataModels::testBookModelFromJson()
{
    QJsonObject ownerObj;
    ownerObj["id"] = QStringLiteral("60d5ec49f1b2c8b1f8e4e1a1");
    ownerObj["username"] = QStringLiteral("alice_seller");

    QJsonObject bookObj;
    bookObj["id"] = QStringLiteral("60d5ec49f1b2c8b1f8e4e1a2");
    bookObj["title"] = QStringLiteral("Design Patterns: Elements of Reusable Object-Oriented Software");
    bookObj["author"] = QStringLiteral("Erich Gamma");
    bookObj["isbn"] = QStringLiteral("9780201633610");
    bookObj["description"] = QStringLiteral("Classic gang of four software engineering book");
    bookObj["category"] = QStringLiteral("Academic");
    bookObj["price"] = 450.0;
    bookObj["condition"] = QStringLiteral("Like New");
    bookObj["coverImage"] = QStringLiteral("http://localhost:8080/uploads/cover.jpg");
    bookObj["status"] = QStringLiteral("available");
    bookObj["averageRating"] = 4.8;
    bookObj["reviewCount"] = 15;
    bookObj["owner"] = ownerObj;
    bookObj["createdAt"] = QStringLiteral("2026-09-15T10:00:00Z");
    bookObj["updatedAt"] = QStringLiteral("2026-09-16T12:00:00Z");

    BookModel book = BookModel::fromJson(bookObj);

    QCOMPARE(book.id, QStringLiteral("60d5ec49f1b2c8b1f8e4e1a2"));
    QCOMPARE(book.title, QStringLiteral("Design Patterns: Elements of Reusable Object-Oriented Software"));
    QCOMPARE(book.author, QStringLiteral("Erich Gamma"));
    QCOMPARE(book.isbn, QStringLiteral("9780201633610"));
    QCOMPARE(book.category, QStringLiteral("Academic"));
    QCOMPARE(book.price, 450.0);
    QCOMPARE(book.condition, QStringLiteral("Like New"));
    QCOMPARE(book.coverImage, QStringLiteral("http://localhost:8080/uploads/cover.jpg"));
    QCOMPARE(book.status, QStringLiteral("available"));
    QCOMPARE(book.averageRating, 4.8);
    QCOMPARE(book.reviewCount, 15);
    QCOMPARE(book.owner.id, QStringLiteral("60d5ec49f1b2c8b1f8e4e1a1"));
    QCOMPARE(book.owner.username, QStringLiteral("alice_seller"));
    QVERIFY(book.createdAt.isValid());
    QVERIFY(book.updatedAt.isValid());
}

void TestDataModels::testBookModelDefaults()
{
    QJsonObject emptyObj;
    BookModel book = BookModel::fromJson(emptyObj);

    QVERIFY(book.id.isEmpty());
    QVERIFY(book.title.isEmpty());
    QCOMPARE(book.price, 0.0);
    QCOMPARE(book.averageRating, 0.0);
    QCOMPARE(book.reviewCount, 0);
    QVERIFY(book.owner.id.isEmpty());
}

void TestDataModels::testCartItemModelFromJson()
{
    QJsonObject itemObj;
    itemObj["bookId"] = QStringLiteral("60d5ec49f1b2c8b1f8e4e1b1");
    itemObj["title"] = QStringLiteral("Clean Architecture");
    itemObj["author"] = QStringLiteral("Robert C. Martin");
    itemObj["price"] = 350.0;
    itemObj["condition"] = QStringLiteral("Good");
    itemObj["category"] = QStringLiteral("Academic");
    itemObj["coverImage"] = QStringLiteral("");
    itemObj["status"] = QStringLiteral("available");
    itemObj["isAvailable"] = true;
    itemObj["quantity"] = 1;
    itemObj["ownerId"] = QStringLiteral("60d5ec49f1b2c8b1f8e4e1a1");
    itemObj["ownerUsername"] = QStringLiteral("seller_bob");

    CartItemModel item = CartItemModel::fromJson(itemObj);

    QCOMPARE(item.bookId, QStringLiteral("60d5ec49f1b2c8b1f8e4e1b1"));
    QCOMPARE(item.title, QStringLiteral("Clean Architecture"));
    QCOMPARE(item.price, 350.0);
    QCOMPARE(item.isAvailable, true);
    QCOMPARE(item.quantity, 1);
    QCOMPARE(item.owner.id, QStringLiteral("60d5ec49f1b2c8b1f8e4e1a1"));
    QCOMPARE(item.owner.username, QStringLiteral("seller_bob"));
}

void TestDataModels::testCartModelCalculation()
{
    QJsonObject cartObj;
    cartObj["userId"] = QStringLiteral("60d5ec49f1b2c8b1f8e4e1c1");
    cartObj["subtotal"] = 700.0;
    cartObj["shippingCost"] = 0.0;
    cartObj["total"] = 700.0;
    cartObj["hasUnavailableItems"] = false;

    QJsonArray itemsArr;
    QJsonObject item1;
    item1["bookId"] = QStringLiteral("b1");
    item1["title"] = QStringLiteral("Book 1");
    item1["price"] = 300.0;
    item1["isAvailable"] = true;
    itemsArr.append(item1);

    QJsonObject item2;
    item2["bookId"] = QStringLiteral("b2");
    item2["title"] = QStringLiteral("Book 2");
    item2["price"] = 400.0;
    item2["isAvailable"] = true;
    itemsArr.append(item2);

    cartObj["items"] = itemsArr;

    CartModel cart = CartModel::fromJson(cartObj);

    QCOMPARE(cart.userId, QStringLiteral("60d5ec49f1b2c8b1f8e4e1c1"));
    QCOMPARE(cart.subtotal, 700.0);
    QCOMPARE(cart.shippingCost, 0.0);
    QCOMPARE(cart.total, 700.0);
    QCOMPARE(cart.hasUnavailableItems, false);
    QCOMPARE(cart.items.size(), 2);
    QCOMPARE(cart.items[0].title, QStringLiteral("Book 1"));
    QCOMPARE(cart.items[1].title, QStringLiteral("Book 2"));
}

void TestDataModels::testOrderModelFromJson()
{
    QJsonObject orderObj;
    orderObj["id"] = QStringLiteral("60d5ec49f1b2c8b1f8e4e1d1");
    orderObj["orderNumber"] = QStringLiteral("ORD-20261002-12345");
    orderObj["buyerId"] = QStringLiteral("60d5ec49f1b2c8b1f8e4e1c1");
    orderObj["buyerUsername"] = QStringLiteral("charlie_buyer");
    orderObj["subtotal"] = 500.0;
    orderObj["shippingCost"] = 0.0;
    orderObj["total"] = 500.0;
    orderObj["status"] = QStringLiteral("CONFIRMED");
    orderObj["createdAt"] = QStringLiteral("2026-10-02T08:30:00Z");

    QJsonObject addrObj;
    addrObj["fullName"] = QStringLiteral("Charlie Buyer");
    addrObj["addressLine"] = QStringLiteral("42 Park Street");
    addrObj["city"] = QStringLiteral("Bengaluru");
    addrObj["state"] = QStringLiteral("Karnataka");
    addrObj["postalCode"] = QStringLiteral("560001");
    addrObj["phone"] = QStringLiteral("9876543210");
    orderObj["shippingAddress"] = addrObj;

    QJsonObject payObj;
    payObj["method"] = QStringLiteral("UPI");
    payObj["status"] = QStringLiteral("PAID");
    orderObj["payment"] = payObj;

    QJsonObject shipObj;
    shipObj["trackingId"] = QStringLiteral("TRK-987654");
    shipObj["status"] = QStringLiteral("STANDARD");
    orderObj["shipping"] = shipObj;

    QJsonArray histArr;
    QJsonObject h1;
    h1["status"] = QStringLiteral("CONFIRMED");
    h1["timestamp"] = QStringLiteral("2026-10-02T08:30:00Z");
    h1["note"] = QStringLiteral("Order placed via UPI");
    histArr.append(h1);
    orderObj["history"] = histArr;

    QJsonArray itemsArr;
    QJsonObject it1;
    it1["bookId"] = QStringLiteral("b1");
    it1["title"] = QStringLiteral("Operating Systems");
    it1["author"] = QStringLiteral("Silberschatz");
    it1["sellerId"] = QStringLiteral("s1");
    it1["sellerUsername"] = QStringLiteral("alice_seller");
    it1["price"] = 500.0;
    it1["condition"] = QStringLiteral("Very Good");
    it1["quantity"] = 1;
    itemsArr.append(it1);
    orderObj["items"] = itemsArr;

    OrderModel order = OrderModel::fromJson(orderObj);

    QCOMPARE(order.id, QStringLiteral("60d5ec49f1b2c8b1f8e4e1d1"));
    QCOMPARE(order.orderNumber, QStringLiteral("ORD-20261002-12345"));
    QCOMPARE(order.buyerUsername, QStringLiteral("charlie_buyer"));
    QCOMPARE(order.status, QStringLiteral("CONFIRMED"));
    QCOMPARE(order.total, 500.0);
    QCOMPARE(order.paymentMethod, QStringLiteral("UPI"));
    QCOMPARE(order.paymentStatus, QStringLiteral("PAID"));
    QCOMPARE(order.trackingId, QStringLiteral("TRK-987654"));
    QCOMPARE(order.shippingAddress.fullName, QStringLiteral("Charlie Buyer"));
    QCOMPARE(order.shippingAddress.city, QStringLiteral("Bengaluru"));
    QCOMPARE(order.history.size(), 1);
    QCOMPARE(order.history[0].status, QStringLiteral("CONFIRMED"));
    QCOMPARE(order.items.size(), 1);
    QCOMPARE(order.items[0].title, QStringLiteral("Operating Systems"));
}

void TestDataModels::testShippingAddressModelRoundtrip()
{
    ShippingAddressModel original;
    original.fullName = QStringLiteral("Dev User");
    original.addressLine = QStringLiteral("123 Tech Park");
    original.city = QStringLiteral("Pune");
    original.state = QStringLiteral("Maharashtra");
    original.postalCode = QStringLiteral("411001");
    original.phone = QStringLiteral("9123456789");

    QJsonObject json = original.toJson();
    ShippingAddressModel restored = ShippingAddressModel::fromJson(json);

    QCOMPARE(restored.fullName, original.fullName);
    QCOMPARE(restored.addressLine, original.addressLine);
    QCOMPARE(restored.city, original.city);
    QCOMPARE(restored.state, original.state);
    QCOMPARE(restored.postalCode, original.postalCode);
    QCOMPARE(restored.phone, original.phone);
}

void TestDataModels::testExchangeModelFromJson()
{
    QJsonObject exObj;
    exObj["id"] = QStringLiteral("60d5ec49f1b2c8b1f8e4e1e1");
    exObj["exchangeNumber"] = QStringLiteral("EX-20261002-9999");
    exObj["requesterId"] = QStringLiteral("u1");
    exObj["requesterUsername"] = QStringLiteral("alice");
    exObj["receiverId"] = QStringLiteral("u2");
    exObj["receiverUsername"] = QStringLiteral("bob");
    exObj["status"] = QStringLiteral("PENDING");
    exObj["message"] = QStringLiteral("Would love to swap books!");
    exObj["statusNote"] = QStringLiteral("Awaiting response");
    exObj["createdAt"] = QStringLiteral("2026-10-02T09:00:00Z");

    QJsonObject reqBook;
    reqBook["bookId"] = QStringLiteral("rb1");
    reqBook["title"] = QStringLiteral("Introduction to Algorithms");
    reqBook["author"] = QStringLiteral("CLRS");
    reqBook["condition"] = QStringLiteral("Very Good");
    reqBook["originalOwnerId"] = QStringLiteral("u2");
    reqBook["originalOwnerUsername"] = QStringLiteral("bob");
    exObj["requestedBook"] = reqBook;

    QJsonObject offBook;
    offBook["bookId"] = QStringLiteral("ob1");
    offBook["title"] = QStringLiteral("Artificial Intelligence: A Modern Approach");
    offBook["author"] = QStringLiteral("Russell & Norvig");
    offBook["condition"] = QStringLiteral("Like New");
    offBook["originalOwnerId"] = QStringLiteral("u1");
    offBook["originalOwnerUsername"] = QStringLiteral("alice");
    exObj["offeredBook"] = offBook;

    ExchangeModel ex = ExchangeModel::fromJson(exObj);

    QCOMPARE(ex.id, QStringLiteral("60d5ec49f1b2c8b1f8e4e1e1"));
    QCOMPARE(ex.exchangeNumber, QStringLiteral("EX-20261002-9999"));
    QCOMPARE(ex.requesterUsername, QStringLiteral("alice"));
    QCOMPARE(ex.receiverUsername, QStringLiteral("bob"));
    QCOMPARE(ex.status, QStringLiteral("PENDING"));
    QCOMPARE(ex.message, QStringLiteral("Would love to swap books!"));
    QCOMPARE(ex.requestedBook.title, QStringLiteral("Introduction to Algorithms"));
    QCOMPARE(ex.offeredBook.title, QStringLiteral("Artificial Intelligence: A Modern Approach"));
}

void TestDataModels::testReviewModelFromJson()
{
    QJsonObject revObj;
    revObj["id"] = QStringLiteral("60d5ec49f1b2c8b1f8e4e1f1");
    revObj["bookId"] = QStringLiteral("b1");
    revObj["reviewerId"] = QStringLiteral("u1");
    revObj["reviewerUsername"] = QStringLiteral("charlie");
    revObj["rating"] = 5;
    revObj["comment"] = QStringLiteral("Fantastic condition, arrived quickly!");
    revObj["createdAt"] = QStringLiteral("2026-10-02T10:00:00Z");

    ReviewModel rev = ReviewModel::fromJson(revObj);

    QCOMPARE(rev.id, QStringLiteral("60d5ec49f1b2c8b1f8e4e1f1"));
    QCOMPARE(rev.reviewerUsername, QStringLiteral("charlie"));
    QCOMPARE(rev.rating, 5);
    QCOMPARE(rev.comment, QStringLiteral("Fantastic condition, arrived quickly!"));
    QVERIFY(rev.createdAt.isValid());
}

void TestDataModels::testPaginationMetaFromJson()
{
    QJsonObject pagObj;
    pagObj["total"] = 45;
    pagObj["page"] = 2;
    pagObj["limit"] = 10;
    pagObj["totalPages"] = 5;
    pagObj["hasNextPage"] = true;
    pagObj["hasPrevPage"] = true;

    PaginationMeta meta = PaginationMeta::fromJson(pagObj);

    QCOMPARE(meta.total, 45);
    QCOMPARE(meta.page, 2);
    QCOMPARE(meta.limit, 10);
    QCOMPARE(meta.totalPages, 5);
    QCOMPARE(meta.hasNextPage, true);
    QCOMPARE(meta.hasPrevPage, true);
}
