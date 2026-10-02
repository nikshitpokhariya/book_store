#ifndef DATAMODELS_H
#define DATAMODELS_H

#include <QString>
#include <QVector>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

// =========================================================
// BookOwner
// =========================================================
struct BookOwner {
    QString id;
    QString username;

    static BookOwner fromJson(const QJsonObject &obj) {
        return {
            obj.value(QStringLiteral("id")).toString(),
            obj.value(QStringLiteral("username")).toString()
        };
    }
};

// =========================================================
// BookModel
// =========================================================
struct BookModel {
    QString id;
    QString title;
    QString author;
    QString isbn;
    QString description;
    QString category;
    double price{0.0};
    QString condition;
    QString coverImage;
    QString status;
    double averageRating{0.0};
    int reviewCount{0};
    BookOwner owner;
    QDateTime createdAt;
    QDateTime updatedAt;

    static BookModel fromJson(const QJsonObject &obj) {
        BookModel b;
        b.id = obj.value(QStringLiteral("id")).toString();
        b.title = obj.value(QStringLiteral("title")).toString();
        b.author = obj.value(QStringLiteral("author")).toString();
        b.isbn = obj.value(QStringLiteral("isbn")).toString();
        b.description = obj.value(QStringLiteral("description")).toString();
        b.category = obj.value(QStringLiteral("category")).toString();
        b.price = obj.value(QStringLiteral("price")).toDouble(0.0);
        b.condition = obj.value(QStringLiteral("condition")).toString();
        b.coverImage = obj.value(QStringLiteral("coverImage")).toString();
        b.status = obj.value(QStringLiteral("status")).toString();
        b.averageRating = obj.value(QStringLiteral("averageRating")).toDouble(0.0);
        b.reviewCount = obj.value(QStringLiteral("reviewCount")).toInt(0);

        if (obj.contains(QStringLiteral("owner")) && obj[QStringLiteral("owner")].isObject()) {
            b.owner = BookOwner::fromJson(obj[QStringLiteral("owner")].toObject());
        }

        b.createdAt = QDateTime::fromString(obj.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
        b.updatedAt = QDateTime::fromString(obj.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate);
        return b;
    }
};

// =========================================================
// CartItemModel & CartModel
// =========================================================
struct CartItemModel {
    QString bookId;
    QString title;
    QString author;
    double price{0.0};
    QString condition;
    QString category;
    QString coverImage;
    QString status;
    bool isAvailable{true};
    int quantity{1};
    BookOwner owner;

    static CartItemModel fromJson(const QJsonObject &obj) {
        CartItemModel item;
        item.bookId = obj.value(QStringLiteral("bookId")).toString();
        item.title = obj.value(QStringLiteral("title")).toString();
        item.author = obj.value(QStringLiteral("author")).toString();
        item.price = obj.value(QStringLiteral("price")).toDouble(0.0);
        item.condition = obj.value(QStringLiteral("condition")).toString();
        item.category = obj.value(QStringLiteral("category")).toString();
        item.coverImage = obj.value(QStringLiteral("coverImage")).toString();
        item.status = obj.value(QStringLiteral("status")).toString();
        item.isAvailable = obj.value(QStringLiteral("isAvailable")).toBool(true);
        item.quantity = obj.value(QStringLiteral("quantity")).toInt(1);

        if (obj.contains(QStringLiteral("owner")) && obj[QStringLiteral("owner")].isObject()) {
            item.owner = BookOwner::fromJson(obj[QStringLiteral("owner")].toObject());
        } else {
            item.owner.id = obj.value(QStringLiteral("ownerId")).toString();
            item.owner.username = obj.value(QStringLiteral("ownerUsername")).toString();
        }
        return item;
    }
};

struct CartModel {
    QString userId;
    double subtotal{0.0};
    double shippingCost{0.0};
    double total{0.0};
    bool hasUnavailableItems{false};
    QVector<CartItemModel> items;

    static CartModel fromJson(const QJsonObject &obj) {
        CartModel c;
        c.userId = obj.value(QStringLiteral("userId")).toString();
        c.subtotal = obj.value(QStringLiteral("subtotal")).toDouble(0.0);
        c.shippingCost = obj.value(QStringLiteral("shippingCost")).toDouble(0.0);
        c.total = obj.value(QStringLiteral("total")).toDouble(0.0);
        c.hasUnavailableItems = obj.value(QStringLiteral("hasUnavailableItems")).toBool(false);

        const QJsonArray arr = obj.value(QStringLiteral("items")).toArray();
        for (const auto &val : arr) {
            c.items.append(CartItemModel::fromJson(val.toObject()));
        }
        return c;
    }
};

// =========================================================
// ShippingAddressModel
// =========================================================
struct ShippingAddressModel {
    QString fullName;
    QString addressLine;
    QString city;
    QString state;
    QString postalCode;
    QString phone;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj[QStringLiteral("fullName")] = fullName;
        obj[QStringLiteral("addressLine")] = addressLine;
        obj[QStringLiteral("city")] = city;
        obj[QStringLiteral("state")] = state;
        obj[QStringLiteral("postalCode")] = postalCode;
        obj[QStringLiteral("phone")] = phone;
        return obj;
    }

    static ShippingAddressModel fromJson(const QJsonObject &obj) {
        return {
            obj.value(QStringLiteral("fullName")).toString(),
            obj.value(QStringLiteral("addressLine")).toString(),
            obj.value(QStringLiteral("city")).toString(),
            obj.value(QStringLiteral("state")).toString(),
            obj.value(QStringLiteral("postalCode")).toString(),
            obj.value(QStringLiteral("phone")).toString()
        };
    }
};

// =========================================================
// OrderItemSnapshot
// =========================================================
struct OrderItemSnapshot {
    QString bookId;
    QString title;
    QString author;
    QString sellerId;
    QString sellerUsername;
    double price{0.0};
    QString condition;
    QString category;
    QString coverImage;
    int quantity{1};

    static OrderItemSnapshot fromJson(const QJsonObject &obj) {
        return {
            obj.value(QStringLiteral("bookId")).toString(),
            obj.value(QStringLiteral("title")).toString(),
            obj.value(QStringLiteral("author")).toString(),
            obj.value(QStringLiteral("sellerId")).toString(),
            obj.value(QStringLiteral("sellerUsername")).toString(),
            obj.value(QStringLiteral("price")).toDouble(0.0),
            obj.value(QStringLiteral("condition")).toString(),
            obj.value(QStringLiteral("category")).toString(),
            obj.value(QStringLiteral("coverImage")).toString(),
            obj.value(QStringLiteral("quantity")).toInt(1)
        };
    }
};

// =========================================================
// OrderHistoryEntry
// =========================================================
struct OrderHistoryEntry {
    QString status;
    QDateTime timestamp;
    QString note;

    static OrderHistoryEntry fromJson(const QJsonObject &obj) {
        return {
            obj.value(QStringLiteral("status")).toString(),
            QDateTime::fromString(obj.value(QStringLiteral("timestamp")).toString(), Qt::ISODate),
            obj.value(QStringLiteral("note")).toString()
        };
    }
};

// =========================================================
// OrderModel
// =========================================================
struct OrderModel {
    QString id;
    QString orderNumber;
    QString buyerId;
    QString buyerUsername;
    double subtotal{0.0};
    double shippingCost{0.0};
    double total{0.0};
    QString status;
    QVector<OrderItemSnapshot> items;
    ShippingAddressModel shippingAddress;
    QString paymentMethod;
    QString paymentStatus;
    QString trackingId;
    QString shippingStatus;
    QVector<OrderHistoryEntry> history;
    QDateTime createdAt;

    static OrderModel fromJson(const QJsonObject &obj) {
        OrderModel o;
        o.id = obj.value(QStringLiteral("id")).toString();
        o.orderNumber = obj.value(QStringLiteral("orderNumber")).toString();
        o.buyerId = obj.value(QStringLiteral("buyerId")).toString();
        o.buyerUsername = obj.value(QStringLiteral("buyerUsername")).toString();
        o.subtotal = obj.value(QStringLiteral("subtotal")).toDouble(0.0);
        o.shippingCost = obj.value(QStringLiteral("shippingCost")).toDouble(0.0);
        o.total = obj.value(QStringLiteral("total")).toDouble(0.0);
        o.status = obj.value(QStringLiteral("status")).toString();

        const QJsonArray itemArr = obj.value(QStringLiteral("items")).toArray();
        for (const auto &val : itemArr) {
            o.items.append(OrderItemSnapshot::fromJson(val.toObject()));
        }

        if (obj.contains(QStringLiteral("shippingAddress")) && obj[QStringLiteral("shippingAddress")].isObject()) {
            o.shippingAddress = ShippingAddressModel::fromJson(obj[QStringLiteral("shippingAddress")].toObject());
        }

        if (obj.contains(QStringLiteral("payment")) && obj[QStringLiteral("payment")].isObject()) {
            const auto pObj = obj[QStringLiteral("payment")].toObject();
            o.paymentMethod = pObj.value(QStringLiteral("method")).toString();
            o.paymentStatus = pObj.value(QStringLiteral("status")).toString();
        }

        if (obj.contains(QStringLiteral("shipping")) && obj[QStringLiteral("shipping")].isObject()) {
            const auto sObj = obj[QStringLiteral("shipping")].toObject();
            o.trackingId = sObj.value(QStringLiteral("trackingId")).toString();
            o.shippingStatus = sObj.value(QStringLiteral("status")).toString();
        }

        const QJsonArray histArr = obj.value(QStringLiteral("history")).toArray();
        for (const auto &val : histArr) {
            o.history.append(OrderHistoryEntry::fromJson(val.toObject()));
        }

        o.createdAt = QDateTime::fromString(obj.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
        return o;
    }
};

// =========================================================
// ExchangeBookSnapshot & ExchangeModel
// =========================================================
struct ExchangeBookSnapshot {
    QString bookId;
    QString title;
    QString author;
    QString condition;
    QString category;
    double price{0.0};
    QString coverImage;
    QString originalOwnerId;
    QString originalOwnerUsername;

    static ExchangeBookSnapshot fromJson(const QJsonObject &obj) {
        return {
            obj.value(QStringLiteral("bookId")).toString(),
            obj.value(QStringLiteral("title")).toString(),
            obj.value(QStringLiteral("author")).toString(),
            obj.value(QStringLiteral("condition")).toString(),
            obj.value(QStringLiteral("category")).toString(),
            obj.value(QStringLiteral("price")).toDouble(0.0),
            obj.value(QStringLiteral("coverImage")).toString(),
            obj.value(QStringLiteral("originalOwnerId")).toString(),
            obj.value(QStringLiteral("originalOwnerUsername")).toString()
        };
    }
};

struct ExchangeModel {
    QString id;
    QString exchangeNumber;
    QString requesterId;
    QString requesterUsername;
    QString receiverId;
    QString receiverUsername;
    QString status;
    QString message;
    QString statusNote;
    ExchangeBookSnapshot requestedBook;
    ExchangeBookSnapshot offeredBook;
    QDateTime createdAt;
    QDateTime completedAt;

    static ExchangeModel fromJson(const QJsonObject &obj) {
        ExchangeModel ex;
        ex.id = obj.value(QStringLiteral("id")).toString();
        ex.exchangeNumber = obj.value(QStringLiteral("exchangeNumber")).toString();
        ex.requesterId = obj.value(QStringLiteral("requesterId")).toString();
        ex.requesterUsername = obj.value(QStringLiteral("requesterUsername")).toString();
        ex.receiverId = obj.value(QStringLiteral("receiverId")).toString();
        ex.receiverUsername = obj.value(QStringLiteral("receiverUsername")).toString();
        ex.status = obj.value(QStringLiteral("status")).toString();
        ex.message = obj.value(QStringLiteral("message")).toString();
        ex.statusNote = obj.value(QStringLiteral("statusNote")).toString();

        if (obj.contains(QStringLiteral("requestedBook")) && obj[QStringLiteral("requestedBook")].isObject()) {
            ex.requestedBook = ExchangeBookSnapshot::fromJson(obj[QStringLiteral("requestedBook")].toObject());
        }
        if (obj.contains(QStringLiteral("offeredBook")) && obj[QStringLiteral("offeredBook")].isObject()) {
            ex.offeredBook = ExchangeBookSnapshot::fromJson(obj[QStringLiteral("offeredBook")].toObject());
        }

        ex.createdAt = QDateTime::fromString(obj.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
        if (obj.contains(QStringLiteral("completedAt")) && obj[QStringLiteral("completedAt")].isString()) {
            ex.completedAt = QDateTime::fromString(obj.value(QStringLiteral("completedAt")).toString(), Qt::ISODate);
        }
        return ex;
    }
};

// =========================================================
// ReviewModel
// =========================================================
struct ReviewModel {
    QString id;
    QString bookId;
    QString reviewerId;
    QString reviewerUsername;
    int rating{5};
    QString comment;
    QDateTime createdAt;

    static ReviewModel fromJson(const QJsonObject &obj) {
        return {
            obj.value(QStringLiteral("id")).toString(),
            obj.value(QStringLiteral("bookId")).toString(),
            obj.value(QStringLiteral("reviewerId")).toString(),
            obj.value(QStringLiteral("reviewerUsername")).toString(),
            obj.value(QStringLiteral("rating")).toInt(5),
            obj.value(QStringLiteral("comment")).toString(),
            QDateTime::fromString(obj.value(QStringLiteral("createdAt")).toString(), Qt::ISODate)
        };
    }
};

// =========================================================
// PaginationMeta
// =========================================================
struct PaginationMeta {
    int total{0};
    int page{1};
    int limit{10};
    int totalPages{0};
    bool hasNextPage{false};
    bool hasPrevPage{false};

    static PaginationMeta fromJson(const QJsonObject &obj) {
        return {
            obj.value(QStringLiteral("total")).toInt(0),
            obj.value(QStringLiteral("page")).toInt(1),
            obj.value(QStringLiteral("limit")).toInt(10),
            obj.value(QStringLiteral("totalPages")).toInt(0),
            obj.value(QStringLiteral("hasNextPage")).toBool(false),
            obj.value(QStringLiteral("hasPrevPage")).toBool(false)
        };
    }
};

#endif // DATAMODELS_H
