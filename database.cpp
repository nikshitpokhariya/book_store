#include "database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDebug>

Database::Database()
{
}

Database::~Database()
{
    if (db.isOpen())
    {
        db.close();
    }
}

Database& Database::instance()
{
    static Database database;
    return database;
}

bool Database::initialize()
{
    db = QSqlDatabase::addDatabase("QSQLITE");

    db.setDatabaseName("bookmarket.db");

    if (!db.open())
    {
        qDebug() << "Database error:"
                 << db.lastError().text();

        return false;
    }

    QSqlQuery query;

    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS users
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT NOT NULL UNIQUE,
            phone TEXT NOT NULL,
            password TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(createTable))
    {
        qDebug() << "Table creation error:"
                 << query.lastError().text();

        return false;
    }

    return true;
}

QString Database::hashPassword(const QString& password)
{
    QByteArray data = password.toUtf8();

    QByteArray hash =
        QCryptographicHash::hash(
            data,
            QCryptographicHash::Sha256
            );

    return hash.toHex();
}

bool Database::emailExists(const QString& email)
{
    QSqlQuery query;

    query.prepare(
        "SELECT id FROM users WHERE email = :email"
        );

    query.bindValue(":email", email);

    if (!query.exec())
    {
        qDebug() << query.lastError().text();
        return false;
    }

    return query.next();
}

bool Database::registerUser(const QString& name,
                            const QString& email,
                            const QString& phone,
                            const QString& password)
{
    if (emailExists(email))
    {
        return false;
    }

    QSqlQuery query;

    query.prepare(
        "INSERT INTO users "
        "(name, email, phone, password) "
        "VALUES (:name, :email, :phone, :password)"
        );

    query.bindValue(":name", name);
    query.bindValue(":email", email);
    query.bindValue(":phone", phone);
    query.bindValue(":password", hashPassword(password));

    if (!query.exec())
    {
        qDebug() << "Registration error:"
                 << query.lastError().text();

        return false;
    }

    return true;
}

bool Database::loginUser(const QString& email,
                         const QString& password)
{
    QSqlQuery query;

    query.prepare(
        "SELECT id FROM users "
        "WHERE email = :email "
        "AND password = :password"
        );

    query.bindValue(":email", email);
    query.bindValue(":password", hashPassword(password));

    if (!query.exec())
    {
        qDebug() << "Login error:"
                 << query.lastError().text();

        return false;
    }

    return query.next();
}

QString Database::getUserName(const QString& email)
{
    QSqlQuery query;

    query.prepare(
        "SELECT name FROM users "
        "WHERE email = :email"
        );

    query.bindValue(":email", email);

    if (!query.exec())
    {
        return "";
    }

    if (query.next())
    {
        return query.value(0).toString();
    }

    return "";
}