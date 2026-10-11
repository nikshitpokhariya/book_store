#include "ImageLoader.h"
#include <QLabel>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDebug>

ImageLoader& ImageLoader::instance()
{
    static ImageLoader loader;
    return loader;
}

ImageLoader::ImageLoader(QObject *parent)
    : QObject(parent),
      netManager_(new QNetworkAccessManager(this))
{
    diskCacheDir_ = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/BookBazzar/images");
    QDir().mkpath(diskCacheDir_);
}

QString ImageLoader::getDiskCacheFilePath(const QString &url)
{
    const QByteArray hash = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha256).toHex();
    return diskCacheDir_ + QStringLiteral("/") + QString::fromLatin1(hash) + QStringLiteral(".cache");
}

bool ImageLoader::loadFromDiskCache(const QString &url, QPixmap &outPix)
{
    const QString cachePath = getDiskCacheFilePath(url);
    if (QFileInfo::exists(cachePath)) {
        if (outPix.load(cachePath) && !outPix.isNull()) {
            return true;
        } else {
            QFile::remove(cachePath);
        }
    }
    return false;
}

void ImageLoader::saveToDiskCache(const QString &url, const QByteArray &data)
{
    if (data.isEmpty()) return;
    const QString cachePath = getDiskCacheFilePath(url);
    QFile file(cachePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
    }
}

QString ImageLoader::getOptimizedCloudinaryUrl(const QString &url, const QSize &size, bool cropFill)
{
    const QString trimmed = url.trimmed();
    if (!trimmed.contains(QStringLiteral("res.cloudinary.com")) ||
        !trimmed.contains(QStringLiteral("/image/upload/"))) {
        return trimmed;
    }

    // If it already contains explicit transformations, leave it
    if (trimmed.contains(QStringLiteral("/image/upload/c_")) ||
        trimmed.contains(QStringLiteral("/image/upload/w_")) ||
        trimmed.contains(QStringLiteral("/image/upload/h_"))) {
        return trimmed;
    }

    const int targetW = qMax(80, size.width() * 2);
    const int targetH = qMax(80, size.height() * 2);

    if (targetW >= 2400 || targetH >= 2400) {
        QString transform = QStringLiteral("c_limit,w_2560,h_2560,q_auto:best,f_auto/");
        QString result = trimmed;
        result.replace(QStringLiteral("/image/upload/"), QStringLiteral("/image/upload/") + transform);
        return result;
    }

    QString transform;
    if (cropFill) {
        transform = QStringLiteral("c_fill,w_%1,h_%2,g_auto,q_auto,f_jpg/").arg(targetW).arg(targetH);
    } else {
        transform = QStringLiteral("c_limit,w_%1,h_%2,q_auto,f_jpg/").arg(targetW).arg(targetH);
    }

    QString result = trimmed;
    result.replace(QStringLiteral("/image/upload/"), QStringLiteral("/image/upload/") + transform);
    return result;
}

QString ImageLoader::resolveLocalPath(const QString &pathOrUrl)
{
    const QString trimmed = pathOrUrl.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    static QHash<QString, QString> s_resolvedPathCache;
    auto cacheIt = s_resolvedPathCache.constFind(trimmed);
    if (cacheIt != s_resolvedPathCache.constEnd()) {
        return cacheIt.value();
    }

    // Direct local file check
    QFileInfo fi(trimmed);
    if (fi.exists() && fi.isFile()) {
        QString canon = fi.canonicalFilePath();
        s_resolvedPathCache.insert(trimmed, canon);
        return canon;
    }

    // Check if it's an /uploads/ URL
    QString filename;
    if (trimmed.contains(QStringLiteral("/uploads/"))) {
        filename = trimmed.section(QStringLiteral("/uploads/"), 1, 1);
    } else if (trimmed.startsWith(QStringLiteral("uploads/"))) {
        filename = trimmed.mid(8);
    } else {
        QUrl u(trimmed);
        if (u.isValid() && !u.fileName().isEmpty()) {
            filename = u.fileName();
        }
    }

    if (!filename.isEmpty()) {
        filename = filename.section('?', 0, 0).section('#', 0, 0);

        const QList<QDir> searchRoots = {
            QDir(QDir::currentPath()),
            QDir(QCoreApplication::applicationDirPath())
        };

        for (const auto &root : searchRoots) {
            QDir cur = root;
            for (int depth = 0; depth < 6; ++depth) {
                const QStringList candidates = {
                    cur.filePath(QStringLiteral("backend/public/uploads/") + filename),
                    cur.filePath(QStringLiteral("public/uploads/") + filename),
                    cur.filePath(QStringLiteral("uploads/") + filename),
                    cur.filePath(filename)
                };

                for (const auto &candidate : candidates) {
                    QFileInfo cfi(candidate);
                    if (cfi.exists() && cfi.isFile()) {
                        QString canon = cfi.canonicalFilePath();
                        s_resolvedPathCache.insert(trimmed, canon);
                        return canon;
                    }
                }
                if (!cur.cdUp()) break;
            }
        }
    }

    s_resolvedPathCache.insert(trimmed, QString());
    return QString();
}

QPixmap ImageLoader::proceduralCover(const QString &title,
                                     const QString &author,
                                     const QSize &size)
{
    const int w = qMax(40, size.width());
    const int h = qMax(40, size.height());

    QPixmap pix(w, h);
    pix.fill(QColor("#0F172A"));

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);

    static const QVector<QPair<QColor, QColor>> palette = {
        { QColor("#4F46E5"), QColor("#312E81") },
        { QColor("#0D9488"), QColor("#115E59") },
        { QColor("#2563EB"), QColor("#1E3A8A") },
        { QColor("#7C3AED"), QColor("#4C1D95") },
        { QColor("#D97706"), QColor("#78350F") }
    };

    const uint hash = qHash(title + author);
    const auto colors = palette[hash % static_cast<uint>(palette.size())];

    QLinearGradient grad(0, 0, w, h);
    grad.setColorAt(0, colors.first);
    grad.setColorAt(1, colors.second);
    painter.setBrush(grad);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, w, h, 8, 8);

    // Subtle spine
    painter.setBrush(QColor(255, 255, 255, 20));
    painter.drawRect(0, 0, qMax(6, w / 16), h);

    // Decorative inner border
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
    painter.drawRoundedRect(8, 8, w - 16, h - 16, 4, 4);

    // Title / author text
    if (w >= 90 && h >= 120 && !title.isEmpty()) {
        painter.setPen(QColor("#FFFFFF"));
        QFont font("Segoe UI", qMax(8, w / 12), QFont::Bold);
        painter.setFont(font);
        painter.drawText(QRect(12, 16, w - 24, h / 2),
                         Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap,
                         title);

        if (!author.isEmpty()) {
            painter.setPen(QColor(255, 255, 255, 180));
            QFont aFont("Segoe UI", qMax(7, w / 15));
            painter.setFont(aFont);
            painter.drawText(QRect(12, h - 30, w - 24, 20),
                             Qt::AlignBottom | Qt::AlignLeft | Qt::TextSingleLine,
                             author);
        }
    }

    return pix;
}

QPixmap ImageLoader::coverCrop(const QPixmap &pix, const QSize &size, int borderRadius, const QColor &bgColor)
{
    if (pix.isNull() || size.width() <= 0 || size.height() <= 0) {
        return pix;
    }

    // High-DPI 2x supersampling for razor-sharp rendering on all modern displays
    const int scaleFactor = 2;
    const QSize renderSize(size.width() * scaleFactor, size.height() * scaleFactor);

    // Scale with smooth transformation
    QPixmap scaled = pix.scaled(renderSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    // Center crop to exact target render size
    int x = qMax(0, (scaled.width() - renderSize.width()) / 2);
    int y = qMax(0, (scaled.height() - renderSize.height()) / 2);
    QPixmap cropped = scaled.copy(x, y, renderSize.width(), renderSize.height());

    QPixmap result(renderSize);
    result.fill(bgColor);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (borderRadius > 0) {
        QPainterPath path;
        path.addRoundedRect(0, 0, renderSize.width(), renderSize.height(),
                            borderRadius * scaleFactor, borderRadius * scaleFactor);
        painter.setClipPath(path);
    }

    painter.drawPixmap(0, 0, cropped);
    painter.end();

    result.setDevicePixelRatio(scaleFactor);
    return result;
}

void ImageLoader::fetchPixmap(const QString &pathOrUrl,
                              const QSize &preferredSize,
                              std::function<void(const QPixmap &pix)> callback)
{
    const QString trimmed = pathOrUrl.trimmed();
    if (trimmed.isEmpty()) {
        if (callback) callback(QPixmap());
        return;
    }

    // 1. Check local file (always high-res original)
    const QString local = resolveLocalPath(trimmed);
    if (!local.isEmpty()) {
        QPixmap pix(local);
        if (!pix.isNull()) {
            if (callback) callback(pix);
            return;
        }
    }

    // 2. Optimize URL for Cloudinary if applicable
    const bool isCloudinary = trimmed.contains(QStringLiteral("res.cloudinary.com"));
    const QString reqUrl = (isCloudinary && preferredSize.isValid() && preferredSize.width() > 0)
                               ? getOptimizedCloudinaryUrl(trimmed, preferredSize, preferredSize.width() <= 120)
                               : trimmed;

    // 3. Check memory cache for exact requested URL
    if (cache_.contains(reqUrl) && !cache_[reqUrl].isNull()) {
        if (callback) callback(cache_[reqUrl]);
        return;
    }

    // If master URL is cached and it's large enough (not a downscaled thumbnail)
    if (cache_.contains(trimmed) && !cache_[trimmed].isNull()) {
        const QPixmap &master = cache_[trimmed];
        const int minReqW = preferredSize.isValid() ? preferredSize.width() : 0;
        const int minReqH = preferredSize.isValid() ? preferredSize.height() : 0;
        if (master.width() >= minReqW && master.height() >= minReqH) {
            if (callback) callback(master);
            return;
        }
    }

    // 4. Check disk cache
    QPixmap diskPix;
    if (loadFromDiskCache(reqUrl, diskPix)) {
        cache_.insert(reqUrl, diskPix);
        if (callback) callback(diskPix);
        return;
    }

    // Only fallback to disk cache for trimmed if request was for the full original
    if (reqUrl == trimmed && loadFromDiskCache(trimmed, diskPix)) {
        cache_.insert(trimmed, diskPix);
        if (callback) callback(diskPix);
        return;
    }

    if (!reqUrl.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) &&
        !reqUrl.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        if (callback) callback(QPixmap());
        return;
    }

    // 5. In-flight deduplication
    if (inFlightCallbacks_.contains(reqUrl)) {
        if (callback) inFlightCallbacks_[reqUrl].append(callback);
        return;
    }

    if (callback) {
        inFlightCallbacks_[reqUrl].append(callback);
    }

    QNetworkRequest request{QUrl(reqUrl)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BookBazzar/1.0 (Windows; Qt6)"));
    request.setRawHeader("Accept", "image/jpeg,image/png,*/*;q=0.8");

    QNetworkReply *reply = netManager_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, reqUrl, trimmed]() {
        reply->deleteLater();

        const auto callbacks = inFlightCallbacks_.take(reqUrl);

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[ImageLoader] HTTP error fetching image:" << reqUrl
                       << "Error:" << reply->errorString();
            for (const auto &cb : callbacks) {
                if (cb) cb(QPixmap());
            }
            return;
        }

        const QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            qWarning() << "[ImageLoader] Received empty body for image:" << reqUrl;
            for (const auto &cb : callbacks) {
                if (cb) cb(QPixmap());
            }
            return;
        }

        QPixmap pix;
        if (!pix.loadFromData(data)) {
            QImage img;
            if (img.loadFromData(data)) {
                pix = QPixmap::fromImage(img);
            } else if (img.loadFromData(data, "JPG") || img.loadFromData(data, "JPEG") || img.loadFromData(data, "PNG")) {
                pix = QPixmap::fromImage(img);
            }
        }

        if (!pix.isNull()) {
            // Save to disk and memory caches under the specific requested URL
            saveToDiskCache(reqUrl, data);
            cache_.insert(reqUrl, pix);

            // CRITICAL: Only cache under master trimmed URL if this was an uncompressed/full-resolution fetch!
            // Never poison the master key with downsized thumbnails!
            if (reqUrl == trimmed || pix.width() >= 1200) {
                saveToDiskCache(trimmed, data);
                cache_.insert(trimmed, pix);
            }

            for (const auto &cb : callbacks) {
                if (cb) cb(pix);
            }
        } else {
            qWarning() << "[ImageLoader] Failed to decode image data (" << data.size() << "bytes) from:" << reqUrl;
            for (const auto &cb : callbacks) {
                if (cb) cb(QPixmap());
            }
        }
    });
}

void ImageLoader::load(const QString &pathOrUrl,
                       QLabel *targetLabel,
                       const QSize &size,
                       const QString &title,
                       const QString &author)
{
    if (!targetLabel) return;

    const QString trimmed = pathOrUrl.trimmed();
    targetLabel->setProperty("expectedUrl", trimmed);

    // Try resolving as local file first
    const QString local = resolveLocalPath(trimmed);
    if (!local.isEmpty()) {
        QPixmap pix(local);
        if (!pix.isNull()) {
            targetLabel->setPixmap(coverCrop(pix, size, 8));
            return;
        }
    }

    // Check memory cache
    if (cache_.contains(trimmed) && !cache_[trimmed].isNull()) {
        targetLabel->setPixmap(coverCrop(cache_[trimmed], size, 8));
        return;
    }

    // Check disk cache
    QPixmap diskPix;
    if (loadFromDiskCache(trimmed, diskPix)) {
        cache_.insert(trimmed, diskPix);
        targetLabel->setPixmap(coverCrop(diskPix, size, 8));
        return;
    }

    // Display high-polish procedural fallback placeholder while loading
    const QPixmap fallback = proceduralCover(title, author, size);
    targetLabel->setPixmap(fallback);

    if (trimmed.isEmpty() || (!trimmed.startsWith("http://") && !trimmed.startsWith("https://"))) {
        return;
    }

    QPointer<QLabel> safeLabel(targetLabel);
    fetchPixmap(trimmed, size, [this, safeLabel, trimmed, size](const QPixmap &pix) {
        if (!safeLabel) return;
        if (safeLabel->property("expectedUrl").toString() != trimmed) {
            return; // Discard stale out-of-order response
        }
        if (!pix.isNull()) {
            safeLabel->setPixmap(coverCrop(pix, size, 8));
        }
    });
}
