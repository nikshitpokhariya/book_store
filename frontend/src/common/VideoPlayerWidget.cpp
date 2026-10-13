#include "VideoPlayerWidget.h"
#include "VideoThumbnailHelper.h"
#include "ImageLoader.h"
#include "network/ApiClient.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDesktopServices>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dshow.h>
#pragma comment(lib, "strmiids.lib")
#endif

namespace {
QString formatTimeMs(qint64 ms)
{
    if (ms < 0) ms = 0;
    qint64 totalSec = ms / 1000;
    qint64 mins = totalSec / 60;
    qint64 secs = totalSec % 60;
    return QStringLiteral("%1:%2")
        .arg(mins, 2, 10, QLatin1Char('0'))
        .arg(secs, 2, 10, QLatin1Char('0'));
}
}

VideoPlayerWidget::VideoPlayerWidget(QWidget *parent)
    : QWidget(parent),
      progressTimer_(new QTimer(this)),
      netManager_(new QNetworkAccessManager(this))
{
    setupUI();
    connect(progressTimer_, &QTimer::timeout, this, &VideoPlayerWidget::updatePlaybackProgress);
}

VideoPlayerWidget::~VideoPlayerWidget()
{
    cleanupDirectShow();
}

void VideoPlayerWidget::setupUI()
{
    setStyleSheet("background-color: #0F172A; border-radius: 12px;");
    setMinimumSize(320, 280);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Video Viewing Container
    videoContainer_ = new QWidget(this);
    videoContainer_->setStyleSheet("background-color: #0F172A; border-top-left-radius: 12px; border-top-right-radius: 12px;");
    videoContainer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *containerLayout = new QVBoxLayout(videoContainer_);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    // Native DirectShow Render Surface
    videoSurface_ = new QWidget(videoContainer_);
    videoSurface_->setStyleSheet("background-color: #000000;");
    videoSurface_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    videoSurface_->setAttribute(Qt::WA_NativeWindow, true);
    videoSurface_->setAttribute(Qt::WA_OpaquePaintEvent, true);
    videoSurface_->setVisible(false);
    containerLayout->addWidget(videoSurface_);

    // Poster Overlay with Glowing Play Badge
    posterOverlay_ = new QLabel(videoContainer_);
    posterOverlay_->setAlignment(Qt::AlignCenter);
    posterOverlay_->setStyleSheet("background: transparent;");
    posterOverlay_->setCursor(Qt::PointingHandCursor);
    posterOverlay_->installEventFilter(this);
    containerLayout->addWidget(posterOverlay_);

    mainLayout->addWidget(videoContainer_, 1);

    // Controls Bar
    controlsBar_ = new QWidget(this);
    controlsBar_->setFixedHeight(44);
    controlsBar_->setStyleSheet(QString(R"(
        QWidget {
            background-color: #1E293B;
            border-bottom-left-radius: 12px;
            border-bottom-right-radius: 12px;
            border-top: 1px solid #334155;
        }
        QPushButton {
            background-color: transparent;
            color: #FFFFFF;
            border: none;
            font-size: 15px;
            font-weight: bold;
            padding: 4px 8px;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 0.12);
            color: %1;
        }
        QSlider::groove:horizontal {
            height: 4px;
            background: #334155;
            border-radius: 2px;
        }
        QSlider::sub-page:horizontal {
            background: %1;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #FFFFFF;
            width: 12px;
            margin: -4px 0;
            border-radius: 6px;
        }
        QSlider::handle:horizontal:hover {
            background: %1;
        }
    )").arg(AppStyle::Primary));

    QHBoxLayout *ctrlLayout = new QHBoxLayout(controlsBar_);
    ctrlLayout->setContentsMargins(12, 4, 12, 4);
    ctrlLayout->setSpacing(10);

    playPauseBtn_ = new QPushButton("▶", controlsBar_);
    playPauseBtn_->setFixedSize(30, 30);
    playPauseBtn_->setCursor(Qt::PointingHandCursor);
    connect(playPauseBtn_, &QPushButton::clicked, this, &VideoPlayerWidget::onPlayPauseClicked);
    ctrlLayout->addWidget(playPauseBtn_);

    replayBtn_ = new QPushButton("↺", controlsBar_);
    replayBtn_->setFixedSize(28, 28);
    replayBtn_->setToolTip("Replay Video");
    replayBtn_->setCursor(Qt::PointingHandCursor);
    connect(replayBtn_, &QPushButton::clicked, this, &VideoPlayerWidget::onReplayClicked);
    ctrlLayout->addWidget(replayBtn_);

    seekSlider_ = new QSlider(Qt::Horizontal, controlsBar_);
    seekSlider_->setRange(0, 100);
    seekSlider_->setValue(0);
    seekSlider_->setCursor(Qt::PointingHandCursor);
    connect(seekSlider_, &QSlider::sliderMoved, this, &VideoPlayerWidget::onSliderMoved);
    connect(seekSlider_, &QSlider::sliderReleased, this, &VideoPlayerWidget::onSliderReleased);
    ctrlLayout->addWidget(seekSlider_, 1);

    timeLabel_ = new QLabel("00:00 / 00:00", controlsBar_);
    timeLabel_->setStyleSheet("color: #94A3B8; font-size: 11px; font-weight: 600; border: none; background: transparent;");
    ctrlLayout->addWidget(timeLabel_);

    muteBtn_ = new QPushButton("🔊", controlsBar_);
    muteBtn_->setFixedSize(28, 28);
    muteBtn_->setCursor(Qt::PointingHandCursor);
    connect(muteBtn_, &QPushButton::clicked, this, &VideoPlayerWidget::onMuteToggled);
    ctrlLayout->addWidget(muteBtn_);

    popoutBtn_ = new QPushButton("⤢", controlsBar_);
    popoutBtn_->setFixedSize(28, 28);
    popoutBtn_->setToolTip("Play in Full Screen Player");
    popoutBtn_->setCursor(Qt::PointingHandCursor);
    connect(popoutBtn_, &QPushButton::clicked, this, &VideoPlayerWidget::openInExternalPlayer);
    ctrlLayout->addWidget(popoutBtn_);

    mainLayout->addWidget(controlsBar_);
}

void VideoPlayerWidget::refreshPoster()
{
    const QSize targetSize = videoContainer_->size().isValid() && videoContainer_->width() > 50
                                 ? videoContainer_->size()
                                 : QSize(352, 280);

    QPixmap thumb = VideoThumbnailHelper::getThumbnail(currentSource_, targetSize, bookTitle_, coverBackdrop_);
    QPixmap poster = VideoThumbnailHelper::generateVideoPoster(thumb, targetSize, bookTitle_, coverBackdrop_);
    posterOverlay_->setPixmap(poster);

    const QString cldThumb = VideoThumbnailHelper::getCloudinaryThumbnailUrl(currentSource_, targetSize);
    if (!cldThumb.isEmpty()) {
        ImageLoader::instance().fetchPixmap(cldThumb, targetSize, [this, targetSize](const QPixmap &pix) {
            if (!pix.isNull() && !isPlaying_ && !isPaused_ && posterOverlay_) {
                QPixmap updatedPoster = VideoThumbnailHelper::generateVideoPoster(pix, targetSize, bookTitle_, coverBackdrop_);
                posterOverlay_->setPixmap(updatedPoster);
            }
        });
    }
}

void VideoPlayerWidget::setVideoSource(const QString &urlOrPath, const QString &title, const QString &coverBackdrop)
{
    stop();
    cleanupDirectShow();

    currentSource_ = urlOrPath.trimmed();
    bookTitle_ = title;
    coverBackdrop_ = coverBackdrop;
    resolvedLocalPath_.clear();
    totalDurationMs_ = 0;
    timeLabel_->setText("00:00 / 00:00");
    seekSlider_->setValue(0);

    if (currentSource_.isEmpty()) {
        posterOverlay_->clear();
        return;
    }

    refreshPoster();

    // Check if local file exists directly or in backend uploads
    const QString local = ImageLoader::resolveLocalPath(currentSource_);
    if (!local.isEmpty() && QFileInfo::exists(local)) {
        resolvedLocalPath_ = local;
        initDirectShow(resolvedLocalPath_);
        refreshPoster();
    } else {
        QString effectiveUrl = currentSource_;
        if (!effectiveUrl.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) &&
            !effectiveUrl.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
            if (effectiveUrl.startsWith(QLatin1Char('/')) || effectiveUrl.startsWith(QStringLiteral("uploads/"))) {
                QString base = ApiClient::baseUrl();
                if (!effectiveUrl.startsWith(QLatin1Char('/'))) base += QLatin1Char('/');
                effectiveUrl = base + effectiveUrl;
            }
        }
        if (effectiveUrl.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
            effectiveUrl.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
            downloadRemoteVideo(effectiveUrl);
        }
    }
}

bool VideoPlayerWidget::initDirectShow(const QString &filePath)
{
#ifdef _WIN32
    cleanupDirectShow();

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    (void)hr;

    IGraphBuilder *graph = nullptr;
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                          IID_IGraphBuilder, (void **)&graph);
    if (FAILED(hr) || !graph) return false;
    pGraph_ = graph;

    IMediaControl *control = nullptr;
    IVideoWindow *vidWin = nullptr;
    IMediaEventEx *evt = nullptr;
    IMediaSeeking *seeking = nullptr;
    IBasicAudio *audio = nullptr;

    graph->QueryInterface(IID_IMediaControl, (void **)&control);
    graph->QueryInterface(IID_IVideoWindow, (void **)&vidWin);
    graph->QueryInterface(IID_IMediaEventEx, (void **)&evt);
    graph->QueryInterface(IID_IMediaSeeking, (void **)&seeking);
    graph->QueryInterface(IID_IBasicAudio, (void **)&audio);

    pControl_ = control;
    pVidWin_ = vidWin;
    pEvent_ = evt;
    pSeeking_ = seeking;
    pAudio_ = audio;

    const std::wstring wpath = QDir::toNativeSeparators(filePath).toStdWString();
    hr = graph->RenderFile(wpath.c_str(), NULL);
    if (FAILED(hr)) {
        cleanupDirectShow();
        return false;
    }

    if (vidWin && videoSurface_) {
        HWND hwnd = (HWND)videoSurface_->winId();
        vidWin->put_Owner((OAHWND)hwnd);
        vidWin->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
        RECT rc;
        GetClientRect(hwnd, &rc);
        vidWin->SetWindowPosition(0, 0, rc.right - rc.left, rc.bottom - rc.top);
    }

    if (seeking) {
        LONGLONG dur = 0;
        if (SUCCEEDED(seeking->GetDuration(&dur)) && dur > 0) {
            totalDurationMs_ = dur / 10000; // 100ns units to milliseconds
            seekSlider_->setRange(0, static_cast<int>(totalDurationMs_));
            timeLabel_->setText(QStringLiteral("00:00 / %1").arg(formatTimeMs(totalDurationMs_)));
        }
    }

    return true;
#else
    Q_UNUSED(filePath);
    return false;
#endif
}

void VideoPlayerWidget::cleanupDirectShow()
{
#ifdef _WIN32
    if (pControl_) {
        static_cast<IMediaControl *>(pControl_)->Stop();
    }
    if (pVidWin_) {
        static_cast<IVideoWindow *>(pVidWin_)->put_Visible(OAFALSE);
        static_cast<IVideoWindow *>(pVidWin_)->put_Owner((OAHWND)NULL);
        static_cast<IVideoWindow *>(pVidWin_)->Release();
        pVidWin_ = nullptr;
    }
    if (pControl_) {
        static_cast<IMediaControl *>(pControl_)->Release();
        pControl_ = nullptr;
    }
    if (pEvent_) {
        static_cast<IMediaEventEx *>(pEvent_)->Release();
        pEvent_ = nullptr;
    }
    if (pSeeking_) {
        static_cast<IMediaSeeking *>(pSeeking_)->Release();
        pSeeking_ = nullptr;
    }
    if (pAudio_) {
        static_cast<IBasicAudio *>(pAudio_)->Release();
        pAudio_ = nullptr;
    }
    if (pGraph_) {
        static_cast<IGraphBuilder *>(pGraph_)->Release();
        pGraph_ = nullptr;
    }
#endif
}

void VideoPlayerWidget::downloadRemoteVideo(const QString &url)
{
    QUrl qurl(url);
    QString fileName = qurl.fileName();
    if (fileName.isEmpty()) fileName = QStringLiteral("temp_video.mp4");

    const QString tempDir = QDir::tempPath() + QStringLiteral("/bookbazzar_media");
    QDir().mkpath(tempDir);
    const QString targetFile = tempDir + QStringLiteral("/") + fileName;

    if (QFileInfo::exists(targetFile) && QFileInfo(targetFile).size() > 0) {
        resolvedLocalPath_ = targetFile;
        initDirectShow(resolvedLocalPath_);
        refreshPoster();
        return;
    }

    QNetworkRequest req(qurl);
    QNetworkReply *reply = netManager_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, targetFile]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QFile f(targetFile);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(reply->readAll());
                f.close();
                resolvedLocalPath_ = targetFile;
                initDirectShow(resolvedLocalPath_);
                refreshPoster();
            }
        }
    });
}

void VideoPlayerWidget::openInExternalPlayer()
{
    QString target = !resolvedLocalPath_.isEmpty() ? resolvedLocalPath_ : currentSource_;
    if (target.isEmpty()) return;

    if (QFileInfo::exists(target)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(target));
    } else if (target.startsWith("http://", Qt::CaseInsensitive) || target.startsWith("https://", Qt::CaseInsensitive)) {
        QDesktopServices::openUrl(QUrl(target));
    } else {
        const QString loc = ImageLoader::resolveLocalPath(target);
        if (!loc.isEmpty() && QFileInfo::exists(loc)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(loc));
        }
    }
}

void VideoPlayerWidget::play()
{
    if (resolvedLocalPath_.isEmpty()) {
        const QString local = ImageLoader::resolveLocalPath(currentSource_);
        if (!local.isEmpty() && QFileInfo::exists(local)) {
            resolvedLocalPath_ = local;
            initDirectShow(resolvedLocalPath_);
        }
    }

    if (!pControl_) {
        if (!resolvedLocalPath_.isEmpty()) {
            initDirectShow(resolvedLocalPath_);
        }
    }

#ifdef _WIN32
    if (pControl_) {
        posterOverlay_->setVisible(false);
        videoSurface_->setVisible(true);

        if (pVidWin_ && videoSurface_) {
            HWND hwnd = (HWND)videoSurface_->winId();
            RECT rc;
            GetClientRect(hwnd, &rc);
            int rw = rc.right - rc.left;
            int rh = rc.bottom - rc.top;
            if (rw <= 0 || rh <= 0) {
                rw = videoContainer_->width() > 0 ? videoContainer_->width() : 352;
                rh = videoContainer_->height() > 0 ? videoContainer_->height() : 280;
            }
            static_cast<IVideoWindow *>(pVidWin_)->SetWindowPosition(0, 0, rw, rh);
            static_cast<IVideoWindow *>(pVidWin_)->put_Visible(OATRUE);
        }

        static_cast<IMediaControl *>(pControl_)->Run();
        isPlaying_ = true;
        isPaused_ = false;
        playPauseBtn_->setText("⏸");
        progressTimer_->start(120);
        emit playbackStarted();
        return;
    }
#endif

    // High-reliability Fallback:
    // If DirectShow could not decode the MP4 stream on this Windows installation,
    // seamlessly open in Windows default media player so the video is 100% playable!
    openInExternalPlayer();
}

void VideoPlayerWidget::pause()
{
#ifdef _WIN32
    if (pControl_ && isPlaying_) {
        static_cast<IMediaControl *>(pControl_)->Pause();
        isPlaying_ = false;
        isPaused_ = true;
        playPauseBtn_->setText("▶");
        progressTimer_->stop();
        emit playbackStopped();
    }
#endif
}

void VideoPlayerWidget::stop()
{
    progressTimer_->stop();
#ifdef _WIN32
    if (pControl_) {
        static_cast<IMediaControl *>(pControl_)->Stop();
    }
    if (pSeeking_) {
        LONGLONG pos = 0;
        static_cast<IMediaSeeking *>(pSeeking_)->SetPositions(
            &pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
    }
    if (pVidWin_) {
        static_cast<IVideoWindow *>(pVidWin_)->put_Visible(OAFALSE);
    }
#endif
    isPlaying_ = false;
    isPaused_ = false;
    playPauseBtn_->setText("▶");
    seekSlider_->setValue(0);
    timeLabel_->setText(QStringLiteral("00:00 / %1").arg(formatTimeMs(totalDurationMs_)));

    if (videoSurface_) videoSurface_->setVisible(false);
    if (posterOverlay_) posterOverlay_->setVisible(true);
    emit playbackStopped();
}

void VideoPlayerWidget::replay()
{
#ifdef _WIN32
    if (pSeeking_) {
        LONGLONG pos = 0;
        static_cast<IMediaSeeking *>(pSeeking_)->SetPositions(
            &pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
    }
#endif
    seekSlider_->setValue(0);
    play();
}

void VideoPlayerWidget::onPlayPauseClicked()
{
    if (isPlaying_) {
        pause();
    } else {
        play();
    }
}

void VideoPlayerWidget::onReplayClicked()
{
    replay();
}

void VideoPlayerWidget::onSliderMoved(int position)
{
    isSeeking_ = true;
    timeLabel_->setText(QStringLiteral("%1 / %2")
        .arg(formatTimeMs(position))
        .arg(formatTimeMs(totalDurationMs_)));
}

void VideoPlayerWidget::onSliderReleased()
{
    isSeeking_ = false;
#ifdef _WIN32
    if (pSeeking_ && totalDurationMs_ > 0) {
        LONGLONG newPos = static_cast<LONGLONG>(seekSlider_->value()) * 10000;
        static_cast<IMediaSeeking *>(pSeeking_)->SetPositions(
            &newPos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
    }
#endif
}

void VideoPlayerWidget::onMuteToggled()
{
    isMuted_ = !isMuted_;
#ifdef _WIN32
    if (pAudio_) {
        static_cast<IBasicAudio *>(pAudio_)->put_Volume(isMuted_ ? -10000 : 0);
    }
#endif
    muteBtn_->setText(isMuted_ ? "🔇" : "🔊");
}

void VideoPlayerWidget::updatePlaybackProgress()
{
#ifdef _WIN32
    if (pEvent_) {
        long evCode = 0;
        LONG_PTR p1 = 0, p2 = 0;
        while (static_cast<IMediaEventEx *>(pEvent_)->GetEvent(&evCode, &p1, &p2, 0) == S_OK) {
            static_cast<IMediaEventEx *>(pEvent_)->FreeEventParams(evCode, p1, p2);
            if (evCode == EC_COMPLETE) {
                stop();
                return;
            }
        }
    }

    if (pSeeking_ && isPlaying_ && !isSeeking_) {
        LONGLONG curPos = 0;
        if (SUCCEEDED(static_cast<IMediaSeeking *>(pSeeking_)->GetCurrentPosition(&curPos))) {
            const qint64 curMs = curPos / 10000;
            seekSlider_->setValue(static_cast<int>(curMs));
            timeLabel_->setText(QStringLiteral("%1 / %2")
                .arg(formatTimeMs(curMs))
                .arg(formatTimeMs(totalDurationMs_)));

            if (totalDurationMs_ > 0 && curMs >= totalDurationMs_) {
                stop();
            }
        }
    }
#endif
}

void VideoPlayerWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
#ifdef _WIN32
    if (pVidWin_ && videoSurface_ && videoSurface_->isVisible()) {
        HWND hwnd = (HWND)videoSurface_->winId();
        RECT rc;
        GetClientRect(hwnd, &rc);
        static_cast<IVideoWindow *>(pVidWin_)->SetWindowPosition(0, 0, rc.right - rc.left, rc.bottom - rc.top);
    }
#endif
    if (!isPlaying_ && !isPaused_) {
        refreshPoster();
    }
}

bool VideoPlayerWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == posterOverlay_ && event->type() == QEvent::MouseButtonPress) {
        play();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}
