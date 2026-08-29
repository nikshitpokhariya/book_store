#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QSqlDatabase>

class Database
{
public:
    static Database& instance();

    bool initialize();

    bool registerUser(const QString& name,
                      const QString& email,
                      const QString& phone,
                      const QString& password);

    bool loginUser(const QString& email,
                   const QString& password);

    bool emailExists(const QString& email);

    QString getUserName(const QString& email);

private:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase db;

    QString hashPassword(const QString& password);
};

#endif