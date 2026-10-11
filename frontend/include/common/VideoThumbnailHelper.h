#pragma once

#include <QPixmap>
#include <QString>
#include <QSize>

class VideoThumbnailHelper {
public:
    // Retrieves a thumbnail for the given video path or URL.
    // Checks memory cache, attempts native frame extraction, or generates a procedural poster with book cover backdrop.
    static QPixmap getThumbnail(const QString &videoPathOrUrl,
                                const QSize &targetSize = QSize(320, 280),
                                const QString &bookTitle = QString(),
                                const QString &coverImagePath = QString());

    // Generates a sleek, high-definition video poster with dark gradient, backdrop & play badge
    static QPixmap generateVideoPoster(const QPixmap &extractedFrame,
                                       const QSize &targetSize = QSize(320, 280),
                                       const QString &title = QString(),
                                       const QString &coverImagePath = QString());

    // Native Windows Media Foundation frame extractor
    static QPixmap extractFrameFromLocalFile(const QString &localFilePath,
                                            const QSize &targetSize = QSize());

    // Converts a Cloudinary video URL to an instant starting timestamp frame (so_0) thumbnail URL
    static QString getCloudinaryThumbnailUrl(const QString &videoUrl,
                                             const QSize &targetSize = QSize());

    // Asynchronously loads the real starting timestamp frame into targetLabel with a modern play badge overlay
    static void loadThumbnailAsync(const QString &videoPathOrUrl,
                                   class QLabel *targetLabel,
                                   const QSize &targetSize = QSize(66, 84),
                                   const QString &bookTitle = QString(),
                                   const QString &coverImagePath = QString(),
                                   std::function<void(const QPixmap &)> onLoaded = nullptr);

    // Decorates a video frame with a sleek play badge, film border, and rounded corners
    static QPixmap styleVideoThumbnail(const QPixmap &rawFrame,
                                       const QSize &targetSize);

    // Clears cached thumbnails
    static void clearCache();
};
