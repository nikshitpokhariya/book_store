#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QList>
#include <QVariantMap>
#include <QSqlDatabase>

// =========================================================
// BOOK STRUCT
// =========================================================

struct Book
{
    int id = -1;
    QString sellerName;
    QString title;
    QString author;
    QString isbn;
    QString category;
    QString condition;
    double price = 0.0;
    QString description;
    QString location;
    QString imagePath;
    QString status;
    QString createdAt;
};

// =========================================================
// DATABASE CLASS
// =========================================================

class Database
{
public:
    // Singleton
    static Database& instance();

    // Lifecycle
    bool initialize();
    bool isOpen() const;

    // Users
    bool registerUser(
        const QString &name,
        const QString &email,
        const QString &phone,
        const QString &password
        );

    bool loginUser(
        const QString &email,
        const QString &password
        );

    QString getUserName(const QString &email);
    bool userExists(const QString &email);

    // Books
    bool addBook(
        const QString &sellerName,
        const QString &title,
        const QString &author,
        const QString &isbn,
        const QString &category,
        const QString &condition,
        double price,
        const QString &description,
        const QString &location,
        const QString &imagePath
        );

    bool deleteBook(int bookId, const QString &sellerName);

    QList<Book> getBooksByCategory(const QString &category);
    QList<Book> getUserListings(const QString &sellerName);
    QList<Book> getRecentBooks(int limit = 20);
    QList<Book> searchBooks(const QString &searchText);
    Book getBookById(int bookId);

    // Orders
    bool createOrder(int bookId, const QString &buyerName);
    QList<QVariantMap> getUserOrders(const QString &buyerName);

private:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase db;
    bool initialized;
};

#endif // DATABASE_H