#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QSize>
#include <QHash>
#include <QPointer>

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

    static QPixmap proceduralCover(const QString &title,
                                   const QString &author,
                                   const QSize &size);

    static QString resolveLocalPath(const QString &pathOrUrl);

private:
    explicit ImageLoader(QObject *parent = nullptr);
    ~ImageLoader() override = default;

    QNetworkAccessManager *netManager_;
    QHash<QString, QPixmap> cache_;
};

#endif // IMAGELOADER_H
