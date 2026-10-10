#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include <QUrl>
#include <QJsonDocument>
#include <QNetworkCookieJar>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>

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

QNetworkRequest ApiClient::buildRequest(const QString &path,
                                        const QUrlQuery &query,
                                        bool authenticated) const
{
    QString fullUrl = QString::fromLatin1(kBaseUrl);
    if (!path.startsWith('/')) {
        fullUrl += '/';
    }
    fullUrl += path;

    QUrl url(fullUrl);
    if (!query.isEmpty()) {
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

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
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        if (callback) {
            callback(false, QString(), QStringLiteral("Cannot open file for upload: %1").arg(filePath));
        }
        return;
    }

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
    } else if (suffix == QStringLiteral("mp4")) {
        mimeType = QStringLiteral("video/mp4");
    } else if (suffix == QStringLiteral("mov")) {
        mimeType = QStringLiteral("video/quicktime");
    } else if (suffix == QStringLiteral("webm")) {
        mimeType = QStringLiteral("video/webm");
    }

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(mimeType));
    filePart.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QVariant(QStringLiteral("form-data; name=\"file\"; filename=\"%1\"").arg(fileName)));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);

    multiPart->append(filePart);

    QString fullUrl = QString::fromLatin1(kBaseUrl) + QStringLiteral("/api/upload");
    QNetworkRequest request(fullUrl);

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
    } else if (hasNetworkError) {
        errorMsg = reply->errorString();
    }

    if (hasNetworkError || statusCode < 200 || statusCode >= 300) {
        if (errorMsg.isEmpty()) {
            errorMsg = QStringLiteral("HTTP Error %1").arg(statusCode);
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
    auto *reply = post(QStringLiteral("/api/auth/refresh"), {}, false);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
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
