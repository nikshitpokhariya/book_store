#include "database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

// =========================================================
// CONSTRUCTOR & DESTRUCTOR
// =========================================================

Database::Database()
    : initialized(false)
{
}

Database::~Database()
{
    if (db.isOpen())
    {
        db.close();
    }
}

// =========================================================
// SINGLETON
// =========================================================

Database& Database::instance()
{
    static Database instance;
    return instance;
}

// =========================================================
// INITIALIZE DATABASE
// =========================================================

bool Database::initialize()
{
    if (initialized && db.isOpen())
    {
        return true;
    }

    QString databasePath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir directory;
    if (!directory.exists(databasePath))
    {
        if (!directory.mkpath(databasePath))
        {
            qDebug() << "Could not create database directory:" << databasePath;
            return false;
        }
    }

    QString databaseFile = databasePath + "/bookbazzar.db";

    if (QSqlDatabase::contains("BookBazzarConnection"))
    {
        db = QSqlDatabase::database("BookBazzarConnection");
    }
    else
    {
        db = QSqlDatabase::addDatabase("QSQLITE", "BookBazzarConnection");
        db.setDatabaseName(databaseFile);
    }

    if (!db.open())
    {
        qDebug() << "Database open error:" << db.lastError().text();
        return false;
    }

    QSqlQuery pragma(db);
    if (!pragma.exec("PRAGMA foreign_keys = ON"))
    {
        qDebug() << "Foreign key error:" << pragma.lastError().text();
    }

    // 1. Users Table
    QSqlQuery users(db);
    if (!users.exec(R"(
        CREATE TABLE IF NOT EXISTS users
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT NOT NULL UNIQUE,
            phone TEXT,
            password TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )"))
    {
        qDebug() << "Users table error:" << users.lastError().text();
        return false;
    }

    // 2. Books Table
    QSqlQuery books(db);
    if (!books.exec(R"(
        CREATE TABLE IF NOT EXISTS books
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            seller_name TEXT NOT NULL,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            isbn TEXT,
            category TEXT NOT NULL,
            condition TEXT NOT NULL,
            price REAL NOT NULL,
            description TEXT,
            location TEXT,
            image_path TEXT,
            status TEXT DEFAULT 'Available',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )"))
    {
        qDebug() << "Books table error:" << books.lastError().text();
        return false;
    }

    // 3. Orders Table
    QSqlQuery orders(db);
    if (!orders.exec(R"(
        CREATE TABLE IF NOT EXISTS orders
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            book_id INTEGER NOT NULL,
            buyer_name TEXT NOT NULL,
            order_date DATETIME DEFAULT CURRENT_TIMESTAMP,
            status TEXT DEFAULT 'Pending',
            FOREIGN KEY(book_id) REFERENCES books(id)
        )
    )"))
    {
        qDebug() << "Orders table error:" << orders.lastError().text();
        return false;
    }

    initialized = true;
    qDebug() << "BookBazzar database initialized:" << databaseFile;
    return true;
}

// =========================================================
// IS OPEN
// =========================================================

bool Database::isOpen() const
{
    return db.isOpen();
}

// =========================================================
// USER OPERATIONS
// =========================================================

bool Database::registerUser(
    const QString &name,
    const QString &email,
    const QString &phone,
    const QString &password)
{
    if (!initialize())
    {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO users (name, email, phone, password)
        VALUES (:name, :email, :phone, :password)
    )");

    query.bindValue(":name", name);
    query.bindValue(":email", email);
    query.bindValue(":phone", phone);
    query.bindValue(":password", password);

    if (!query.exec())
    {
        qDebug() << "Register error:" << query.lastError().text();
        return false;
    }

    return true;
}

bool Database::loginUser(const QString &email, const QString &password)
{
    if (!initialize())
    {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id FROM users
        WHERE email = :email AND password = :password
        LIMIT 1
    )");

    query.bindValue(":email", email);
    query.bindValue(":password", password);

    if (!query.exec())
    {
        qDebug() << "Login error:" << query.lastError().text();
        return false;
    }

    return query.next();
}

QString Database::getUserName(const QString &email)
{
    if (!initialize())
    {
        return QString();
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT name FROM users
        WHERE email = :email
        LIMIT 1
    )");

    query.bindValue(":email", email);

    if (!query.exec())
    {
        qDebug() << "Get user name error:" << query.lastError().text();
        return QString();
    }

    if (query.next())
    {
        return query.value(0).toString();
    }

    return QString();
}

bool Database::userExists(const QString &email)
{
    if (!initialize())
    {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id FROM users
        WHERE email = :email
        LIMIT 1
    )");

    query.bindValue(":email", email);

    if (!query.exec())
    {
        return false;
    }

    return query.next();
}

// =========================================================
// BOOK OPERATIONS
// =========================================================

bool Database::addBook(
    const QString &sellerName,
    const QString &title,
    const QString &author,
    const QString &isbn,
    const QString &category,
    const QString &condition,
    double price,
    const QString &description,
    const QString &location,
    const QString &imagePath)
{
    if (!initialize())
    {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO books
        (
            seller_name, title, author, isbn,
            category, condition, price, description,
            location, image_path, status
        )
        VALUES
        (
            :seller_name, :title, :author, :isbn,
            :category, :condition, :price, :description,
            :location, :image_path, 'Available'
        )
    )");

    query.bindValue(":seller_name", sellerName);
    query.bindValue(":title", title);
    query.bindValue(":author", author);
    query.bindValue(":isbn", isbn);
    query.bindValue(":category", category);
    query.bindValue(":condition", condition);
    query.bindValue(":price", price);
    query.bindValue(":description", description);
    query.bindValue(":location", location);
    query.bindValue(":image_path", imagePath);

    if (!query.exec())
    {
        qDebug() << "Add book error:" << query.lastError().text();
        return false;
    }

    return true;
}

bool Database::deleteBook(int bookId, const QString &sellerName)
{
    if (!initialize())
    {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        DELETE FROM books
        WHERE id = :id AND seller_name = :seller_name
    )");

    query.bindValue(":id", bookId);
    query.bindValue(":seller_name", sellerName);

    if (!query.exec())
    {
        qDebug() << "Delete book error:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// Helper to deserialize a book row
static Book readBook(QSqlQuery &query)
{
    Book book;
    book.id = query.value("id").toInt();
    book.sellerName = query.value("seller_name").toString();
    book.title = query.value("title").toString();
    book.author = query.value("author").toString();
    book.isbn = query.value("isbn").toString();
    book.category = query.value("category").toString();
    book.condition = query.value("condition").toString();
    book.price = query.value("price").toDouble();
    book.description = query.value("description").toString();
    book.location = query.value("location").toString();
    book.imagePath = query.value("image_path").toString();
    book.status = query.value("status").toString();
    book.createdAt = query.value("created_at").toString();
    return book;
}

QList<Book> Database::getRecentBooks(int limit)
{
    QList<Book> result;
    if (!initialize())
    {
        return result;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            id, seller_name, title, author, isbn,
            category, condition, price, description,
            location, image_path, status, created_at
        FROM books
        WHERE status = 'Available'
        ORDER BY datetime(created_at) DESC
        LIMIT :limit
    )");

    query.bindValue(":limit", limit);

    if (!query.exec())
    {
        qDebug() << "Get recent books error:" << query.lastError().text();
        return result;
    }

    while (query.next())
    {
        result.append(readBook(query));
    }

    return result;
}

QList<Book> Database::searchBooks(const QString &searchText)
{
    QList<Book> result;
    if (!initialize())
    {
        return result;
    }

    QString text = searchText.trimmed();
    if (text.isEmpty())
    {
        return getRecentBooks();
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            id, seller_name, title, author, isbn,
            category, condition, price, description,
            location, image_path, status, created_at
        FROM books
        WHERE status = 'Available'
        AND
        (
            title LIKE :search
            OR author LIKE :search
            OR isbn LIKE :search
            OR category LIKE :search
            OR location LIKE :search
        )
        ORDER BY datetime(created_at) DESC
    )");

    query.bindValue(":search", "%" + text + "%");

    if (!query.exec())
    {
        qDebug() << "Search books error:" << query.lastError().text();
        return result;
    }

    while (query.next())
    {
        result.append(readBook(query));
    }

    return result;
}

QList<Book> Database::getBooksByCategory(const QString &category)
{
    QList<Book> result;
    if (!initialize())
    {
        return result;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            id, seller_name, title, author, isbn,
            category, condition, price, description,
            location, image_path, status, created_at
        FROM books
        WHERE status = 'Available'
        AND category = :category
        ORDER BY datetime(created_at) DESC
    )");

    query.bindValue(":category", category);

    if (!query.exec())
    {
        qDebug() << "Category search error:" << query.lastError().text();
        return result;
    }

    while (query.next())
    {
        result.append(readBook(query));
    }

    return result;
}

QList<Book> Database::getUserListings(const QString &sellerName)
{
    QList<Book> result;
    if (!initialize())
    {
        return result;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            id, seller_name, title, author, isbn,
            category, condition, price, description,
            location, image_path, status, created_at
        FROM books
        WHERE seller_name = :seller_name
        ORDER BY datetime(created_at) DESC
    )");

    query.bindValue(":seller_name", sellerName);

    if (!query.exec())
    {
        qDebug() << "User listings error:" << query.lastError().text();
        return result;
    }

    while (query.next())
    {
        result.append(readBook(query));
    }

    return result;
}

Book Database::getBookById(int bookId)
{
    Book book;
    if (!initialize())
    {
        return book;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            id, seller_name, title, author, isbn,
            category, condition, price, description,
            location, image_path, status, created_at
        FROM books
        WHERE id = :id
        LIMIT 1
    )");

    query.bindValue(":id", bookId);

    if (!query.exec())
    {
        qDebug() << "Get book error:" << query.lastError().text();
        return book;
    }

    if (query.next())
    {
        book = readBook(query);
    }

    return book;
}

// =========================================================
// ORDER OPERATIONS
// =========================================================

bool Database::createOrder(int bookId, const QString &buyerName)
{
    if (!initialize())
    {
        return false;
    }

    // 1. Verify book availability
    QSqlQuery check(db);
    check.prepare(R"(
        SELECT status FROM books
        WHERE id = :id LIMIT 1
    )");
    check.bindValue(":id", bookId);

    if (!check.exec() || !check.next())
    {
        return false;
    }

    if (check.value(0).toString() != "Available")
    {
        return false;
    }

    // 2. Insert into orders table
    QSqlQuery order(db);
    order.prepare(R"(
        INSERT INTO orders (book_id, buyer_name, status)
        VALUES (:book_id, :buyer_name, 'Pending')
    )");
    order.bindValue(":book_id", bookId);
    order.bindValue(":buyer_name", buyerName);

    if (!order.exec())
    {
        qDebug() << "Create order error:" << order.lastError().text();
        return false;
    }

    // 3. Mark book as Sold
    QSqlQuery update(db);
    update.prepare(R"(
        UPDATE books
        SET status = 'Sold'
        WHERE id = :id AND status = 'Available'
    )");
    update.bindValue(":id", bookId);

    if (!update.exec())
    {
        qDebug() << "Update book status error:" << update.lastError().text();
        return false;
    }

    return update.numRowsAffected() > 0;
}

QList<QVariantMap> Database::getUserOrders(const QString &buyerName)
{
    QList<QVariantMap> result;
    if (!initialize())
    {
        return result;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            orders.id AS order_id,
            orders.book_id,
            orders.buyer_name,
            orders.order_date,
            orders.status,
            books.title,
            books.author,
            books.price,
            books.image_path
        FROM orders
        INNER JOIN books ON orders.book_id = books.id
        WHERE orders.buyer_name = :buyer_name
        ORDER BY datetime(orders.order_date) DESC
    )");

    query.bindValue(":buyer_name", buyerName);

    if (!query.exec())
    {
        qDebug() << "Get orders error:" << query.lastError().text();
        return result;
    }

    while (query.next())
    {
        QVariantMap order;
        order["order_id"] = query.value("order_id");
        order["book_id"] = query.value("book_id");
        order["buyer_name"] = query.value("buyer_name");
        order["order_date"] = query.value("order_date");
        order["status"] = query.value("status");
        order["title"] = query.value("title");
        order["author"] = query.value("author");
        order["price"] = query.value("price");
        order["image_path"] = query.value("image_path");
        result.append(order);
    }

    return result;
}