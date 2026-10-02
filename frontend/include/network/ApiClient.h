#ifndef APICLIENT_H
#define APICLIENT_H

// =========================================================
// ApiClient
// =========================================================
// Thin, thread-safe(ish) wrapper around QNetworkAccessManager.
// Handles:
//   - Bearer token injection for every authenticated request
//   - Transparent token refresh via the HttpOnly refresh cookie
//   - JSON parse helpers
//   - Base URL resolution
//
// Usage — authenticated GET:
//   auto *reply = ApiClient::instance().get("/api/books", authenticated);
//   connect(reply, &QNetworkReply::finished, this, [reply]() {
//       auto [ok, json, err] = ApiClient::parseReply(reply);
//       reply->deleteLater();
//       if (ok) { /* use json */ } else { /* show err */ }
//   });
// =========================================================

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUrlQuery>
#include <QString>
#include <tuple>

class ApiClient : public QObject
{
    Q_OBJECT

public:
    static ApiClient &instance();

    static QString baseUrl() {
        return QString::fromUtf8(kBaseUrl);
    }

    // =====================================================
    // Convenience HTTP verbs
    // =====================================================

    // Unauthenticated GET (no token header)
    QNetworkReply *getPublic(const QString &path,
                             const QUrlQuery &query = {});

    // Authenticated GET (adds Authorization: Bearer <token>)
    QNetworkReply *get(const QString &path,
                       const QUrlQuery &query = {});

    // Authenticated POST with JSON body
    QNetworkReply *post(const QString &path,
                        const QJsonObject &body = {},
                        bool authenticated = true);

    // Authenticated PUT with JSON body
    QNetworkReply *put(const QString &path,
                       const QJsonObject &body = {},
                       bool authenticated = true);

    // Authenticated PATCH with JSON body
    QNetworkReply *patch(const QString &path,
                         const QJsonObject &body = {});

    // Authenticated DELETE (optionally with JSON body)
    QNetworkReply *deleteResource(const QString &path,
                                  const QJsonObject &body = {});

    // Convenience alias for deleteResource
    QNetworkReply *del(const QString &path,
                       const QJsonObject &body = {}) {
        return deleteResource(path, body);
    }

    // =====================================================
    // Reply parsing helper
    // =====================================================

    // Returns { success, json-root-object, error-message }
    static std::tuple<bool, QJsonObject, QString>
    parseReply(QNetworkReply *reply);

    // =====================================================
    // Token refresh
    // =====================================================

    // Call this whenever a 401 is received.
    // emits tokenRefreshed(newToken) on success,
    //       refreshFailed() on failure.
    void refreshToken();

signals:
    void tokenRefreshed(const QString &newAccessToken);
    void refreshFailed();

private:
    explicit ApiClient(QObject *parent = nullptr);

    ApiClient(const ApiClient &) = delete;
    ApiClient &operator=(const ApiClient &) = delete;

    QNetworkRequest buildRequest(const QString &path,
                                 const QUrlQuery &query,
                                 bool authenticated) const;

    QNetworkAccessManager *m_nam;

    static constexpr const char *kBaseUrl = "http://localhost:8080";
};

#endif // APICLIENT_H
