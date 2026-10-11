#include "VideoThumbnailHelper.h"
#include "ImageLoader.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QHash>
#include <QFileInfo>
#include <QDir>
#include <QFont>
#include <QDateTime>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#endif

#include <QLabel>
#include <QPointer>

namespace {
static QHash<QString, QPixmap> s_thumbnailCache;
}

void VideoThumbnailHelper::clearCache()
{
    s_thumbnailCache.clear();
}

QPixmap VideoThumbnailHelper::extractFrameFromLocalFile(const QString &localFilePath,
                                                        const QSize &targetSize)
{
    if (localFilePath.isEmpty()) return QPixmap();

    QFileInfo fi(localFilePath);
    if (!fi.exists() || !fi.isFile()) return QPixmap();

#ifdef _WIN32
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    const bool coInited = (hr == S_OK || hr == S_FALSE);

    hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) {
        if (coInited) CoUninitialize();
        return QPixmap();
    }

    IMFSourceReader *pReader = nullptr;
    const std::wstring wpath = QDir::toNativeSeparators(localFilePath).toStdWString();

    IMFAttributes *pAttributes = nullptr;
    MFCreateAttributes(&pAttributes, 1);
    if (pAttributes) {
        pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    }

    hr = MFCreateSourceReaderFromURL(wpath.c_str(), pAttributes, &pReader);
    if (pAttributes) pAttributes->Release();

    if (FAILED(hr) || !pReader) {
        MFShutdown();
        if (coInited) CoUninitialize();
        return QPixmap();
    }

    IMFMediaType *pType = nullptr;
    MFCreateMediaType(&pType);
    if (pType) {
        pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
        pType->Release();
    }

    if (FAILED(hr)) {
        pReader->Release();
        MFShutdown();
        if (coInited) CoUninitialize();
        return QPixmap();
    }

    // Explicitly seek to timestamp 0 (starting timestamp)
    PROPVARIANT var;
    PropVariantInit(&var);
    var.vt = VT_I8;
    var.hVal.QuadPart = 0;
    pReader->SetCurrentPosition(GUID_NULL, var);
    PropVariantClear(&var);

    IMFMediaType *pCurrentType = nullptr;
    UINT32 width = 0, height = 0;
    LONG defaultStride = 0;
    if (SUCCEEDED(pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType)) && pCurrentType) {
        MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &width, &height);
        UINT32 strideVal = 0;
        if (SUCCEEDED(pCurrentType->GetUINT32(MF_MT_DEFAULT_STRIDE, &strideVal))) {
            defaultStride = static_cast<LONG>(strideVal);
        }
        pCurrentType->Release();
    }

    if (width == 0 || height == 0) {
        width = 640;
        height = 480;
    }
    if (defaultStride == 0) {
        defaultStride = static_cast<LONG>(width * 4);
    }

    QImage extractedImg;
    for (int attempt = 0; attempt < 20; ++attempt) {
        DWORD streamIndex = 0, flags = 0;
        LONGLONG timestamp = 0;
        IMFSample *pSample = nullptr;
        hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &flags, &timestamp, &pSample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
            if (pSample) pSample->Release();
            break;
        }

        if (pSample) {
            IMFMediaBuffer *pBuffer = nullptr;
            if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pBuffer)) && pBuffer) {
                BYTE *pData = nullptr;
                DWORD maxLen = 0, curLen = 0;
                if (SUCCEEDED(pBuffer->Lock(&pData, &maxLen, &curLen)) && pData && curLen >= width * height * 4) {
                    const int absStride = qAbs(defaultStride) >= static_cast<LONG>(width * 4) ? qAbs(defaultStride) : static_cast<LONG>(width * 4);
                    QImage img(pData, width, height, absStride, QImage::Format_ARGB32);
                    if (!img.isNull()) {
                        if (defaultStride < 0) {
                            extractedImg = img.mirrored(false, true);
                        } else {
                            extractedImg = img.copy();
                        }
                    }
                    pBuffer->Unlock();
                }
                pBuffer->Release();
            }
            pSample->Release();
            if (!extractedImg.isNull()) break;
        }
    }

    pReader->Release();
    MFShutdown();
    if (coInited) CoUninitialize();

    if (!extractedImg.isNull()) {
        QPixmap pix = QPixmap::fromImage(extractedImg);
        if (targetSize.isValid()) {
            return pix.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        }
        return pix;
    }
#endif

    return QPixmap();
}

QPixmap VideoThumbnailHelper::generateVideoPoster(const QPixmap &extractedFrame,
                                                  const QSize &targetSize,
                                                  const QString &title,
                                                  const QString &coverImagePath)
{
    const int w = targetSize.isValid() ? targetSize.width() : 352;
    const int h = targetSize.isValid() ? targetSize.height() : 280;

    QPixmap poster(w, h);
    poster.fill(QColor("#0F172A"));

    QPainter p(&poster);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QPixmap backdrop = extractedFrame;
    if (backdrop.isNull() && !coverImagePath.isEmpty()) {
        const QString localCover = ImageLoader::resolveLocalPath(coverImagePath);
        if (!localCover.isEmpty()) {
            QPixmap pCov(localCover);
            if (!pCov.isNull()) {
                backdrop = pCov;
            }
        }
    }

    if (!backdrop.isNull()) {
        QPixmap scaled = backdrop.scaled(w, h, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int sx = qMax(0, (scaled.width() - w) / 2);
        int sy = qMax(0, (scaled.height() - h) / 2);
        p.drawPixmap(0, 0, scaled, sx, sy, w, h);

        // Rich cinematic gradient overlay
        QLinearGradient darkGrad(0, 0, 0, h);
        darkGrad.setColorAt(0, QColor(15, 23, 42, 170));
        darkGrad.setColorAt(0.35, QColor(15, 23, 42, 120));
        darkGrad.setColorAt(0.75, QColor(15, 23, 42, 180));
        darkGrad.setColorAt(1, QColor(15, 23, 42, 235));
        p.fillRect(0, 0, w, h, darkGrad);

        // Radial vignette
        QRadialGradient vignette(w / 2.0, h / 2.0, qMax(w, h) * 0.7);
        vignette.setColorAt(0, QColor(15, 23, 42, 40));
        vignette.setColorAt(1, QColor(15, 23, 42, 180));
        p.fillRect(0, 0, w, h, vignette);
    } else {
        // Modern tech gradient background
        QLinearGradient grad(0, 0, w, h);
        grad.setColorAt(0, QColor("#0B0F19"));
        grad.setColorAt(0.5, QColor("#1E1B4B"));
        grad.setColorAt(1, QColor("#0F172A"));
        p.fillRect(0, 0, w, h, grad);

        // Radial glow in center
        QRadialGradient glow(w / 2.0, h * 0.42, w * 0.45);
        glow.setColorAt(0, QColor(99, 102, 241, 100));
        glow.setColorAt(1, QColor(15, 23, 42, 0));
        p.fillRect(0, 0, w, h, glow);
    }

    // Top Film Badge
    p.setBrush(QColor(15, 23, 42, 200));
    p.setPen(QPen(QColor(255, 255, 255, 60), 1));
    p.drawRoundedRect(14, 14, 115, 26, 6, 6);
    p.setPen(QColor("#A5B4FC"));
    QFont badgeFont("Segoe UI", 9, QFont::Bold);
    p.setFont(badgeFont);
    p.drawText(QRect(14, 14, 115, 26), Qt::AlignCenter, "🎬 VIDEO CLIP");

    // Center play button
    const int centerX = w / 2;
    const int centerY = static_cast<int>(h * 0.43);
    const int radius = 30;

    // Glowing outer ring
    p.setBrush(QColor(79, 70, 229, 70));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPoint(centerX, centerY), radius + 9, radius + 9);

    // Primary button circle
    QLinearGradient playGrad(centerX - radius, centerY - radius, centerX + radius, centerY + radius);
    playGrad.setColorAt(0, QColor("#6366F1"));
    playGrad.setColorAt(1, QColor("#4338CA"));
    p.setBrush(playGrad);
    p.setPen(QPen(QColor(255, 255, 255, 120), 1.5));
    p.drawEllipse(QPoint(centerX, centerY), radius, radius);

    // Play triangle
    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    QPolygon triangle;
    triangle << QPoint(centerX - 7, centerY - 12)
             << QPoint(centerX + 12, centerY)
             << QPoint(centerX - 7, centerY + 12);
    p.drawPolygon(triangle);

    // Title text
    p.setPen(Qt::white);
    QFont titleFont("Segoe UI", 12, QFont::Bold);
    p.setFont(titleFont);
    QString displayTitle = title.isEmpty() ? QStringLiteral("Inspection Video") : title;
    QFontMetrics fm(titleFont);
    displayTitle = fm.elidedText(displayTitle, Qt::ElideRight, w - 32);
    p.drawText(QRect(16, centerY + radius + 12, w - 32, 24), Qt::AlignCenter, displayTitle);

    // Subtitle helper
    p.setPen(QColor("#CBD5E1"));
    QFont subFont("Segoe UI", 9.5, QFont::Medium);
    p.setFont(subFont);
    p.drawText(QRect(16, centerY + radius + 36, w - 32, 20), Qt::AlignCenter,
               QStringLiteral("Click anywhere to play inspection video"));

    return poster;
}

QString VideoThumbnailHelper::getCloudinaryThumbnailUrl(const QString &videoUrl, const QSize &targetSize)
{
    QString trimmed = videoUrl.trimmed();
    if (!trimmed.contains(QStringLiteral("res.cloudinary.com"), Qt::CaseInsensitive) ||
        !trimmed.contains(QStringLiteral("/video/upload/"), Qt::CaseInsensitive)) {
        return QString();
    }

    // Cloudinary video thumbnail URL:
    // Insert so_0 (start offset 0 timestamp) and convert extension to .jpg
    QString transform = QStringLiteral("so_0,f_jpg,q_auto");
    if (targetSize.isValid() && targetSize.width() > 0 && targetSize.height() > 0) {
        transform += QStringLiteral(",c_fill,w_%1,h_%2,g_auto")
                         .arg(targetSize.width() * 2)
                         .arg(targetSize.height() * 2);
    }

    QString result = trimmed;
    if (!result.contains(QStringLiteral("/video/upload/so_0"))) {
        result.replace(QStringLiteral("/video/upload/"),
                       QStringLiteral("/video/upload/") + transform + QStringLiteral("/"));
    }

    static const QStringList videoExts = {
        QStringLiteral(".mp4"), QStringLiteral(".mov"),
        QStringLiteral(".webm"), QStringLiteral(".mkv"),
        QStringLiteral(".avi"), QStringLiteral(".m4v")
    };
    for (const auto &ext : videoExts) {
        if (result.endsWith(ext, Qt::CaseInsensitive)) {
            result.chop(ext.length());
            result.append(QStringLiteral(".jpg"));
            break;
        }
    }

    return result;
}

QPixmap VideoThumbnailHelper::styleVideoThumbnail(const QPixmap &rawFrame, const QSize &targetSize)
{
    const int w = targetSize.isValid() ? targetSize.width() : 66;
    const int h = targetSize.isValid() ? targetSize.height() : 84;

    const int scaleFactor = 2;
    const int renderW = w * scaleFactor;
    const int renderH = h * scaleFactor;

    QPixmap result(renderW, renderH);
    result.fill(QColor("#0F172A"));

    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // Rounded clipping
    QPainterPath clip;
    clip.addRoundedRect(0, 0, renderW, renderH, 8 * scaleFactor, 8 * scaleFactor);
    p.setClipPath(clip);

    if (!rawFrame.isNull()) {
        QPixmap scaled = rawFrame.scaled(renderW, renderH, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int sx = qMax(0, (scaled.width() - renderW) / 2);
        int sy = qMax(0, (scaled.height() - renderH) / 2);
        p.drawPixmap(0, 0, scaled, sx, sy, renderW, renderH);

        // Dark gradient overlay towards bottom for badge contrast
        QLinearGradient grad(0, renderH * 0.35, 0, renderH);
        grad.setColorAt(0, QColor(15, 23, 42, 0));
        grad.setColorAt(0.65, QColor(15, 23, 42, 130));
        grad.setColorAt(1, QColor(15, 23, 42, 225));
        p.fillRect(0, 0, renderW, renderH, grad);
    } else {
        QLinearGradient grad(0, 0, renderW, renderH);
        grad.setColorAt(0, QColor("#1E1B4B"));
        grad.setColorAt(1, QColor("#0F172A"));
        p.fillRect(0, 0, renderW, renderH, grad);
    }

    // Play badge in center
    const int cx = renderW / 2;
    const int cy = static_cast<int>(renderH * 0.42);
    const int r = 13 * scaleFactor;

    QLinearGradient playGrad(cx - r, cy - r, cx + r, cy + r);
    playGrad.setColorAt(0, QColor("#6366F1"));
    playGrad.setColorAt(1, QColor("#4338CA"));
    p.setBrush(playGrad);
    p.setPen(QPen(QColor(255, 255, 255, 210), 1.5 * scaleFactor));
    p.drawEllipse(QPoint(cx, cy), r, r);

    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    const int triSize = 6 * scaleFactor;
    QPolygon tr;
    tr << QPoint(cx - triSize / 2, cy - triSize)
       << QPoint(cx + triSize, cy)
       << QPoint(cx - triSize / 2, cy + triSize);
    p.drawPolygon(tr);

    // Pill badge at bottom: ▶ VIDEO
    const int pillH = 16 * scaleFactor;
    const int pillW = renderW - 12 * scaleFactor;
    const int pillY = renderH - pillH - 5 * scaleFactor;
    p.setBrush(QColor(15, 23, 42, 220));
    p.setPen(QPen(QColor(255, 255, 255, 60), 1));
    p.drawRoundedRect(6 * scaleFactor, pillY, pillW, pillH, 4 * scaleFactor, 4 * scaleFactor);

    p.setPen(QColor("#A5B4FC"));
    QFont f("Segoe UI", 7.5 * scaleFactor, QFont::Bold);
    p.setFont(f);
    p.drawText(QRect(6 * scaleFactor, pillY, pillW, pillH), Qt::AlignCenter, "▶ VIDEO");

    p.end();

    result.setDevicePixelRatio(scaleFactor);
    return result;
}

void VideoThumbnailHelper::loadThumbnailAsync(const QString &videoPathOrUrl,
                                              QLabel *targetLabel,
                                              const QSize &targetSize,
                                              const QString &bookTitle,
                                              const QString &coverImagePath,
                                              std::function<void(const QPixmap &)> onLoaded)
{
    if (!targetLabel) return;

    const QString cacheKey = QStringLiteral("styled_%1_%2x%3")
        .arg(videoPathOrUrl)
        .arg(targetSize.width())
        .arg(targetSize.height());

    if (s_thumbnailCache.contains(cacheKey) && !s_thumbnailCache.value(cacheKey).isNull()) {
        targetLabel->setPixmap(s_thumbnailCache.value(cacheKey));
        if (onLoaded) onLoaded(s_thumbnailCache.value(cacheKey));
        return;
    }

    // 1. Check local file or resolve local path
    const QString localFile = ImageLoader::resolveLocalPath(videoPathOrUrl);
    if (!localFile.isEmpty()) {
        QPixmap raw = extractFrameFromLocalFile(localFile, targetSize);
        if (!raw.isNull()) {
            QPixmap styled = styleVideoThumbnail(raw, targetSize);
            s_thumbnailCache.insert(cacheKey, styled);
            targetLabel->setPixmap(styled);
            if (onLoaded) onLoaded(styled);
            return;
        }
    }

    // 2. Check if Cloudinary video URL
    const QString cldThumbUrl = getCloudinaryThumbnailUrl(videoPathOrUrl, targetSize);
    if (!cldThumbUrl.isEmpty()) {
        // Set styled placeholder while loading
        QPixmap placeholder = styleVideoThumbnail(QPixmap(), targetSize);
        targetLabel->setPixmap(placeholder);

        QPointer<QLabel> safeLabel(targetLabel);
        ImageLoader::instance().fetchPixmap(cldThumbUrl, targetSize, [cacheKey, targetSize, safeLabel, onLoaded](const QPixmap &pix) {
            if (!pix.isNull()) {
                QPixmap styled = styleVideoThumbnail(pix, targetSize);
                s_thumbnailCache.insert(cacheKey, styled);
                if (safeLabel) {
                    safeLabel->setPixmap(styled);
                }
                if (onLoaded) onLoaded(styled);
            }
        });
        return;
    }

    // 3. Fallback to procedural thumbnail
    QPixmap placeholder = getThumbnail(videoPathOrUrl, targetSize, bookTitle, coverImagePath);
    QPixmap styled = styleVideoThumbnail(placeholder, targetSize);
    s_thumbnailCache.insert(cacheKey, styled);
    targetLabel->setPixmap(styled);
    if (onLoaded) onLoaded(styled);
}

QPixmap VideoThumbnailHelper::getThumbnail(const QString &videoPathOrUrl,
                                           const QSize &targetSize,
                                           const QString &bookTitle,
                                           const QString &coverImagePath)
{
    const QString cacheKey = QStringLiteral("%1_%2_%3x%4").arg(
        videoPathOrUrl,
        coverImagePath,
        QString::number(targetSize.width()),
        QString::number(targetSize.height()));

    if (s_thumbnailCache.contains(cacheKey)) {
        return s_thumbnailCache.value(cacheKey);
    }

    const QString localFile = ImageLoader::resolveLocalPath(videoPathOrUrl);
    QPixmap frame;
    if (!localFile.isEmpty()) {
        frame = extractFrameFromLocalFile(localFile, targetSize);
    }

    if (frame.isNull() && !coverImagePath.isEmpty()) {
        const QString localCover = ImageLoader::resolveLocalPath(coverImagePath);
        if (!localCover.isEmpty()) {
            QPixmap pCov(localCover);
            if (!pCov.isNull()) {
                frame = pCov;
            }
        }
    }

    QPixmap result;
    if (!frame.isNull()) {
        result = frame;
    } else {
        result = styleVideoThumbnail(QPixmap(), targetSize);
    }

    s_thumbnailCache.insert(cacheKey, result);
    return result;
}
