#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>

// =========================================================
// SessionManager
// Singleton — holds JWT access token, userId, username
// and token expiry for the currently logged-in user.
// =========================================================

class SessionManager : public QObject
{
    Q_OBJECT

public:
    static SessionManager &instance();

    // =====================================================
    // Auth state
    // =====================================================

    bool isLoggedIn() const;

    void setSession(const QString &accessToken,
                    qint64 expiresInSeconds,
                    const QString &userId,
                    const QString &username);

    void clearSession();

    // =====================================================
    // Accessors
    // =====================================================

    QString accessToken() const;
    QString userId() const;
    QString username() const;
    QDateTime tokenExpiresAt() const;
    bool isTokenExpired() const;

signals:
    void sessionCleared();

private:
    explicit SessionManager(QObject *parent = nullptr);

    SessionManager(const SessionManager &) = delete;
    SessionManager &operator=(const SessionManager &) = delete;

    QString m_accessToken;
    QString m_userId;
    QString m_username;
    QDateTime m_tokenExpiresAt;
};

#endif // SESSIONMANAGER_H
