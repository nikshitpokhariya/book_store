#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkCookieJar>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QDebug>

QString ApiClient::s_customBaseUrl = QString();

ApiClient &ApiClient::instance()
{
    static ApiClient inst;
    return inst;
}

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_nam->setCookieJar(new QNetworkCookieJar(this));
}

void ApiClient::setBaseUrl(const QString &url)
{
    s_customBaseUrl = url.trimmed();
    while (s_customBaseUrl.endsWith(QLatin1Char('/'))) {
        s_customBaseUrl.chop(1);
    }
}

QString ApiClient::baseUrl()
{
    if (!s_customBaseUrl.isEmpty()) {
        return s_customBaseUrl;
    }

    const QString envUrl = qEnvironmentVariable("BOOKBAZZAR_API_URL").trimmed();
    if (!envUrl.isEmpty()) {
        QString cleaned = envUrl;
        while (cleaned.endsWith(QLatin1Char('/'))) {
            cleaned.chop(1);
        }
        return cleaned;
    }

    QString defaultUrl = QString::fromLatin1(kBaseUrl).trimmed();
    while (defaultUrl.endsWith(QLatin1Char('/'))) {
        defaultUrl.chop(1);
    }
    return defaultUrl;
}

QNetworkRequest ApiClient::buildRequest(const QString &path,
                                        const QUrlQuery &query,
                                        bool authenticated) const
{
    QString base = baseUrl();
    QString trimmedPath = path.trimmed();

    QString fullUrl = base;
    if (!trimmedPath.startsWith(QLatin1Char('/')) && !fullUrl.endsWith(QLatin1Char('/'))) {
        fullUrl += QLatin1Char('/');
    } else if (trimmedPath.startsWith(QLatin1Char('/')) && fullUrl.endsWith(QLatin1Char('/'))) {
        fullUrl.chop(1);
    }
    fullUrl += trimmedPath;

    QUrl url(fullUrl);
    if (!query.isEmpty()) {
        if (url.hasQuery()) {
            QUrlQuery existingQuery(url.query());
            const auto items = query.queryItems();
            for (const auto &item : items) {
                existingQuery.addQueryItem(item.first, item.second);
            }
            url.setQuery(existingQuery);
        } else {
            url.setQuery(query);
        }
    }

    QNetworkRequest request(url);
    request.setTransferTimeout(30000); // 30 seconds transfer timeout
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BookBazzar/1.0 (Windows; Qt6)"));
    request.setRawHeader("Accept", "application/json");

    if (url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0) {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
        sslConfig.setProtocol(QSsl::SecureProtocols);
        request.setSslConfiguration(sslConfig);
    }

    if (authenticated) {
        const QString token = SessionManager::instance().accessToken();
        if (!token.isEmpty()) {
            request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
        }
    }

    return request;
}

QNetworkReply *ApiClient::getPublic(const QString &path, const QUrlQuery &query)
{
    QNetworkRequest req = buildRequest(path, query, false);
    return m_nam->get(req);
}

QNetworkReply *ApiClient::get(const QString &path, const QUrlQuery &query)
{
    QNetworkRequest req = buildRequest(path, query, true);
    return m_nam->get(req);
}

QNetworkReply *ApiClient::post(const QString &path,
                               const QJsonObject &body,
                               bool authenticated)
{
    QNetworkRequest req = buildRequest(path, {}, authenticated);
    QByteArray data = QJsonDocument(body).toJson(QJsonDocument::Compact);
    return m_nam->post(req, data);
}

QNetworkReply *ApiClient::put(const QString &path,
                              const QJsonObject &body,
                              bool authenticated)
{
    QNetworkRequest req = buildRequest(path, {}, authenticated);
    QByteArray data = QJsonDocument(body).toJson(QJsonDocument::Compact);
    return m_nam->put(req, data);
}

QNetworkReply *ApiClient::patch(const QString &path, const QJsonObject &body)
{
    QNetworkRequest req = buildRequest(path, {}, true);
    QByteArray data = QJsonDocument(body).toJson(QJsonDocument::Compact);
    return m_nam->sendCustomRequest(req, "PATCH", data);
}

QNetworkReply *ApiClient::deleteResource(const QString &path, const QJsonObject &body)
{
    QNetworkRequest req = buildRequest(path, {}, true);
    if (!body.isEmpty()) {
        QByteArray data = QJsonDocument(body).toJson(QJsonDocument::Compact);
        return m_nam->sendCustomRequest(req, "DELETE", data);
    }
    return m_nam->deleteResource(req);
}

void ApiClient::uploadMedia(const QString &filePath,
                            std::function<void(bool success, const QString &url, const QString &errorMsg)> callback)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (callback) {
            callback(false, QString(), QStringLiteral("Cannot open file for upload: %1").arg(filePath));
        }
        return;
    }
    const QByteArray fileData = file.readAll();
    file.close();

    QFileInfo fileInfo(filePath);
    const QString fileName = fileInfo.fileName();
    const QString suffix = fileInfo.suffix().toLower();

    QString mimeType = QStringLiteral("application/octet-stream");
    if (suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg")) {
        mimeType = QStringLiteral("image/jpeg");
    } else if (suffix == QStringLiteral("png")) {
        mimeType = QStringLiteral("image/png");
    } else if (suffix == QStringLiteral("webp")) {
        mimeType = QStringLiteral("image/webp");
    } else if (suffix == QStringLiteral("gif")) {
        mimeType = QStringLiteral("image/gif");
    } else if (suffix == QStringLiteral("bmp")) {
        mimeType = QStringLiteral("image/bmp");
    } else if (suffix == QStringLiteral("mp4")) {
        mimeType = QStringLiteral("video/mp4");
    } else if (suffix == QStringLiteral("mov")) {
        mimeType = QStringLiteral("video/quicktime");
    } else if (suffix == QStringLiteral("webm")) {
        mimeType = QStringLiteral("video/webm");
    }

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setRawHeader("Content-Type", mimeType.toUtf8());
    filePart.setRawHeader(
        "Content-Disposition",
        QStringLiteral("form-data; name=\"file\"; filename=\"%1\"").arg(fileName).toUtf8());
    filePart.setBody(fileData);

    multiPart->append(filePart);

    QString fullUrl = baseUrl() + QStringLiteral("/api/upload");
    QUrl url(fullUrl);
    QNetworkRequest request(url);
    request.setTransferTimeout(120000); // 120 seconds timeout for large media files
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BookBazzar/1.0 (Windows; Qt6)"));

    if (url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0) {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
        sslConfig.setProtocol(QSsl::SecureProtocols);
        request.setSslConfiguration(sslConfig);
    }

    const QString token = SessionManager::instance().accessToken();
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    }

    QNetworkReply *reply = m_nam->post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, [reply, callback]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!ok) {
            if (callback) {
                callback(false, QString(), errorMsg.isEmpty() ? QStringLiteral("Upload failed") : errorMsg);
            }
            return;
        }

        QString url;
        if (json.contains(QStringLiteral("url")) && json[QStringLiteral("url")].isString()) {
            url = json[QStringLiteral("url")].toString();
        } else if (json.contains(QStringLiteral("secure_url")) && json[QStringLiteral("secure_url")].isString()) {
            url = json[QStringLiteral("secure_url")].toString();
        } else if (json.contains(QStringLiteral("data")) && json[QStringLiteral("data")].isObject()) {
            QJsonObject data = json[QStringLiteral("data")].toObject();
            url = data.value(QStringLiteral("url")).toString();
            if (url.isEmpty()) {
                url = data.value(QStringLiteral("secure_url")).toString();
            }
        }

        if (url.isEmpty()) {
            if (callback) {
                callback(false, QString(), QStringLiteral("Upload service did not return an image URL"));
            }
            return;
        }

        if (callback) {
            callback(true, url, QString());
        }
    });
}

std::tuple<bool, QJsonObject, QString> ApiClient::parseReply(QNetworkReply *reply)
{
    if (!reply) {
        return { false, QJsonObject(), QStringLiteral("Null network reply") };
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray data = reply->readAll();

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    const QJsonObject root = doc.isObject() ? doc.object() : QJsonObject();

    const bool hasNetworkError = (reply->error() != QNetworkReply::NoError);

    QString errorMsg;
    if (root.contains(QStringLiteral("message")) && root[QStringLiteral("message")].isString()) {
        errorMsg = root[QStringLiteral("message")].toString();
    } else if (root.contains(QStringLiteral("error")) && root[QStringLiteral("error")].isString()) {
        errorMsg = root[QStringLiteral("error")].toString();
    } else if (root.contains(QStringLiteral("errors")) && root[QStringLiteral("errors")].isArray()) {
        const QJsonArray arr = root[QStringLiteral("errors")].toArray();
        QStringList errList;
        for (const auto &item : arr) {
            if (item.isString()) errList << item.toString();
        }
        if (!errList.isEmpty()) {
            errorMsg = errList.join(QStringLiteral(", "));
        }
    }

    // Specific error mapping for network or protocol level failures
    if (errorMsg.isEmpty()) {
        switch (reply->error()) {
        case QNetworkReply::TimeoutError:
            errorMsg = QStringLiteral("Request timed out. The backend server might be starting up from idle sleep or unreachable.");
            break;
        case QNetworkReply::ConnectionRefusedError:
            errorMsg = QStringLiteral("Connection refused. Please verify your internet connection and backend server status.");
            break;
        case QNetworkReply::HostNotFoundError:
            errorMsg = QStringLiteral("Cannot resolve backend server host. Please check the network or API URL configuration.");
            break;
        case QNetworkReply::SslHandshakeFailedError:
            errorMsg = QStringLiteral("Secure SSL/TLS handshake failed. Please check your system certificates and date/time.");
            break;
        case QNetworkReply::RemoteHostClosedError:
            errorMsg = QStringLiteral("Remote server closed the connection unexpectedly.");
            break;
        default:
            break;
        }
    }

    // HTTP status code fallback messages
    if (errorMsg.isEmpty()) {
        if (statusCode == 401) {
            errorMsg = QStringLiteral("Session expired or unauthorized. Please sign in again.");
        } else if (statusCode == 403) {
            errorMsg = QStringLiteral("Access denied. You do not have permission for this resource.");
        } else if (statusCode == 404) {
            errorMsg = QStringLiteral("Requested resource was not found on the server.");
        } else if (statusCode == 409) {
            errorMsg = QStringLiteral("Resource conflict occurred. Please refresh and try again.");
        } else if (statusCode == 413) {
            errorMsg = QStringLiteral("Media file exceeds server size limit (max 50MB).");
        } else if (statusCode == 429) {
            errorMsg = QStringLiteral("Too many requests. Please wait a moment and try again.");
        } else if (statusCode >= 500 && statusCode < 600) {
            errorMsg = QStringLiteral("Backend server encountered an error (code %1). Please try again shortly.").arg(statusCode);
        } else if (hasNetworkError) {
            errorMsg = reply->errorString();
        }
    }

    if (statusCode == 401) {
        emit ApiClient::instance().unauthorized();
    }

    if (hasNetworkError || statusCode < 200 || statusCode >= 300) {
        qWarning() << "[ApiClient Error] Status:" << statusCode
                   << "URL:" << reply->url().toString()
                   << "Error:" << reply->errorString()
                   << "Msg:" << errorMsg;
        if (errorMsg.isEmpty()) {
            errorMsg = QStringLiteral("Request failed (code %1)").arg(statusCode);
        }
        return { false, root, errorMsg };
    }

    if (root.contains(QStringLiteral("success")) && !root[QStringLiteral("success")].toBool()) {
        return { false, root, errorMsg.isEmpty() ? QStringLiteral("Request failed") : errorMsg };
    }

    return { true, root, QString() };
}

void ApiClient::refreshToken()
{
    if (m_isRefreshing) {
        return;
    }
    m_isRefreshing = true;

    auto *reply = post(QStringLiteral("/api/auth/refresh"), {}, false);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_isRefreshing = false;
        auto [ok, json, err] = parseReply(reply);
        reply->deleteLater();

        if (ok && json.contains(QStringLiteral("accessToken"))) {
            const QString newAccessToken = json[QStringLiteral("accessToken")].toString();
            const qint64 expiresIn = json.value(QStringLiteral("expiresIn")).toInteger(900);
            SessionManager::instance().setSession(
                newAccessToken,
                expiresIn,
                SessionManager::instance().userId(),
                SessionManager::instance().username()
            );
            emit tokenRefreshed(newAccessToken);
        } else {
            SessionManager::instance().clearSession();
            emit refreshFailed();
        }
    });
}
