#include "ImageLoader.h"
#include <QLabel>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QLinearGradient>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

ImageLoader& ImageLoader::instance()
{
    static ImageLoader loader;
    return loader;
}

ImageLoader::ImageLoader(QObject *parent)
    : QObject(parent),
      netManager_(new QNetworkAccessManager(this))
{
}

QString ImageLoader::resolveLocalPath(const QString &pathOrUrl)
{
    const QString trimmed = pathOrUrl.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    // Direct local file check
    QFileInfo fi(trimmed);
    if (fi.exists() && fi.isFile()) {
        return fi.canonicalFilePath();
    }

    // Check if it's an /uploads/ URL
    QString filename;
    if (trimmed.contains(QStringLiteral("/uploads/"))) {
        filename = trimmed.section(QStringLiteral("/uploads/"), 1, 1);
    } else if (trimmed.startsWith(QStringLiteral("uploads/"))) {
        filename = trimmed.mid(8);
    }

    if (!filename.isEmpty()) {
        const QStringList candidates = {
            QStringLiteral("d:/1Hello-World/1pbl-oops/backend/public/uploads/") + filename,
            QDir::currentPath() + QStringLiteral("/../backend/public/uploads/") + filename,
            QDir::currentPath() + QStringLiteral("/../../backend/public/uploads/") + filename,
            QDir::currentPath() + QStringLiteral("/backend/public/uploads/") + filename
        };

        for (const auto &candidate : candidates) {
            QFileInfo cfi(candidate);
            if (cfi.exists() && cfi.isFile()) {
                return cfi.canonicalFilePath();
            }
        }
    }

    return QString();
}

QPixmap ImageLoader::proceduralCover(const QString &title,
                                     const QString &author,
                                     const QSize &size)
{
    const int w = qMax(40, size.width());
    const int h = qMax(40, size.height());

    QPixmap pix(w, h);
    pix.fill(Qt::transparent);

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

void ImageLoader::load(const QString &pathOrUrl,
                       QLabel *targetLabel,
                       const QSize &size,
                       const QString &title,
                       const QString &author)
{
    if (!targetLabel) return;

    const QString trimmed = pathOrUrl.trimmed();

    // Try resolving as local file first
    const QString local = resolveLocalPath(trimmed);
    if (!local.isEmpty()) {
        QPixmap pix(local);
        if (!pix.isNull()) {
            targetLabel->setPixmap(pix.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }
    }

    // Check memory cache
    if (cache_.contains(trimmed)) {
        const QPixmap &cached = cache_.value(trimmed);
        if (!cached.isNull()) {
            targetLabel->setPixmap(cached.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }
    }

    // Set fallback while downloading or if path is empty
    const QPixmap fallback = proceduralCover(title, author, size);
    targetLabel->setPixmap(fallback);

    if (trimmed.isEmpty() || (!trimmed.startsWith("http://") && !trimmed.startsWith("https://"))) {
        return;
    }

    // Fetch via HTTP GET
    QPointer<QLabel> safeLabel(targetLabel);
    QNetworkRequest request{QUrl(trimmed)};
    QNetworkReply *reply = netManager_->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, safeLabel, trimmed, size, fallback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            return;
        }

        const QByteArray data = reply->readAll();
        QPixmap pix;
        if (pix.loadFromData(data)) {
            cache_.insert(trimmed, pix);
            if (safeLabel) {
                safeLabel->setPixmap(pix.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
    });
}
