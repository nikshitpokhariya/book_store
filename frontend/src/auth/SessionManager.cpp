#include "auth/SessionManager.h"
#include "network/ApiClient.h"
#include <QTimer>

SessionManager &SessionManager::instance()
{
    static SessionManager inst;
    return inst;
}

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
    , m_refreshTimer(new QTimer(this))
{
    m_refreshTimer->setSingleShot(true);
    connect(m_refreshTimer, &QTimer::timeout, this, []() {
        if (!SessionManager::instance().accessToken().isEmpty()) {
            ApiClient::instance().refreshToken();
        }
    });
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

    // Schedule proactive token refresh ~2 minutes before expiration (or half duration if short)
    if (m_refreshTimer) {
        m_refreshTimer->stop();
        if (expiresInSeconds > 0) {
            qint64 refreshSecs = (expiresInSeconds > 240) ? (expiresInSeconds - 120) : (expiresInSeconds / 2);
            if (refreshSecs < 10) refreshSecs = 10;
            m_refreshTimer->start(static_cast<int>(refreshSecs * 1000));
        }
    }
}

void SessionManager::clearSession()
{
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
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
