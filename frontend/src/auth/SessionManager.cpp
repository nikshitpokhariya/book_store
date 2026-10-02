#include "auth/SessionManager.h"

SessionManager &SessionManager::instance()
{
    static SessionManager inst;
    return inst;
}

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
{
}

bool SessionManager::isLoggedIn() const
{
    return !m_accessToken.isEmpty() && !isTokenExpired();
}

void SessionManager::setSession(const QString &accessToken,
                                qint64 expiresInSeconds,
                                const QString &userId,
                                const QString &username)
{
    m_accessToken    = accessToken;
    m_userId         = userId;
    m_username       = username;
    // Give a 60-second safety buffer before the server-side expiry
    m_tokenExpiresAt = QDateTime::currentDateTimeUtc()
                           .addSecs(expiresInSeconds - 60);
}

void SessionManager::clearSession()
{
    m_accessToken.clear();
    m_userId.clear();
    m_username.clear();
    m_tokenExpiresAt = QDateTime();
    emit sessionCleared();
}

QString SessionManager::accessToken() const   { return m_accessToken; }
QString SessionManager::userId() const        { return m_userId; }
QString SessionManager::username() const      { return m_username; }
QDateTime SessionManager::tokenExpiresAt() const { return m_tokenExpiresAt; }

bool SessionManager::isTokenExpired() const
{
    if (m_tokenExpiresAt.isNull())
        return true;
    return QDateTime::currentDateTimeUtc() >= m_tokenExpiresAt;
}
