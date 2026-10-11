#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QSize>
#include <QHash>
#include <QVector>
#include <QPointer>
#include <functional>

class QLabel;
class QNetworkAccessManager;

class ImageLoader : public QObject
{
    Q_OBJECT

public:
    static ImageLoader& instance();

    void load(const QString &pathOrUrl,
              QLabel *targetLabel,
              const QSize &size,
              const QString &title = QString(),
              const QString &author = QString());

    void fetchPixmap(const QString &pathOrUrl,
                     const QSize &preferredSize,
                     std::function<void(const QPixmap &pix)> callback);

    static QString getOptimizedCloudinaryUrl(const QString &url,
                                             const QSize &size,
                                             bool cropFill = true);

    static QPixmap coverCrop(const QPixmap &pix,
                             const QSize &size,
                             int borderRadius = 0,
                             const QColor &bgColor = QColor("#0F172A"));

    static QPixmap proceduralCover(const QString &title,
                                   const QString &author,
                                   const QSize &size);

    static QString resolveLocalPath(const QString &pathOrUrl);

private:
    explicit ImageLoader(QObject *parent = nullptr);
    ~ImageLoader() override = default;

    QString getDiskCacheFilePath(const QString &url);
    bool loadFromDiskCache(const QString &url, QPixmap &outPix);
    void saveToDiskCache(const QString &url, const QByteArray &data);

    QNetworkAccessManager *netManager_;
    QHash<QString, QPixmap> cache_;
    QHash<QString, QVector<QPointer<QLabel>>> inFlightLabels_;
    QHash<QString, QVector<std::function<void(const QPixmap&)>>> inFlightCallbacks_;
    QString diskCacheDir_;
};

#endif // IMAGELOADER_H
