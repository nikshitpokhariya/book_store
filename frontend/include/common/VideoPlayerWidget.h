#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QString>
#include <QNetworkAccessManager>

class VideoPlayerWidget : public QWidget {
    Q_OBJECT

public:
    explicit VideoPlayerWidget(QWidget *parent = nullptr);
    ~VideoPlayerWidget() override;

    void setVideoSource(const QString &urlOrPath, const QString &title = QString(), const QString &coverBackdrop = QString());
    void play();
    void pause();
    void stop();
    void replay();
    void openInExternalPlayer();
    bool isPlaying() const { return isPlaying_; }

    void refreshPoster();

signals:
    void playbackStarted();
    void playbackStopped();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
    void onPlayPauseClicked();
    void onReplayClicked();
    void onMuteToggled();

private slots:
    void onSliderMoved(int position);
    void onSliderReleased();
    void updatePlaybackProgress();

private:
    void setupUI();
    bool initDirectShow(const QString &filePath);
    void cleanupDirectShow();
    void downloadRemoteVideo(const QString &url);

    QString currentSource_;
    QString resolvedLocalPath_;
    QString bookTitle_;
    QString coverBackdrop_;

    QWidget *videoContainer_{nullptr};
    QWidget *videoSurface_{nullptr};
    QLabel  *posterOverlay_{nullptr};
    QLabel  *statusBadge_{nullptr};

    QWidget     *controlsBar_{nullptr};
    QPushButton *playPauseBtn_{nullptr};
    QPushButton *replayBtn_{nullptr};
    QSlider     *seekSlider_{nullptr};
    QLabel      *timeLabel_{nullptr};
    QPushButton *muteBtn_{nullptr};
    QPushButton *popoutBtn_{nullptr};

    QTimer *progressTimer_{nullptr};
    QNetworkAccessManager *netManager_{nullptr};

    bool isPlaying_{false};
    bool isPaused_{false};
    bool isMuted_{false};
    bool isSeeking_{false};
    qint64 totalDurationMs_{0};

    // DirectShow interface pointers (void* to avoid Windows header pollution in Qt header)
    void *pGraph_{nullptr};
    void *pControl_{nullptr};
    void *pVidWin_{nullptr};
    void *pEvent_{nullptr};
    void *pSeeking_{nullptr};
    void *pAudio_{nullptr};
};
