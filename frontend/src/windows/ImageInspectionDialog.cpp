#include "windows/ImageInspectionDialog.h"
#include "common/ImageLoader.h"
#include "common/VideoThumbnailHelper.h"
#include "common/VideoPlayerWidget.h"
#include "common/AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QStyle>

// ============================================================================
// ImageInspectionCanvas Implementation
// ============================================================================

ImageInspectionCanvas::ImageInspectionCanvas(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setCursor(Qt::OpenHandCursor);
    setMouseTracking(true);
}

void ImageInspectionCanvas::setImage(const QPixmap &pix, const QString &statusText)
{
    originalPix_ = pix;
    statusText_ = statusText;
    resetZoom();
}

void ImageInspectionCanvas::resetZoom()
{
    panOffset_ = QPoint(0, 0);

    if (originalPix_.isNull() || width() <= 0 || height() <= 0) {
        scale_ = 1.0;
        fitScale_ = 1.0;
    } else {
        const double sx = static_cast<double>(width() - 40) / originalPix_.width();
        const double sy = static_cast<double>(height() - 40) / originalPix_.height();
        fitScale_ = qMin(sx, sy);
        if (fitScale_ <= 0.0) fitScale_ = 1.0;
        scale_ = fitScale_;
    }

    emit scaleChanged(scale_);
    update();
}

void ImageInspectionCanvas::zoomIn()
{
    scale_ = qMin(scale_ * 1.25, 5.0);
    clampPan();
    emit scaleChanged(scale_);
    update();
}

void ImageInspectionCanvas::zoomOut()
{
    scale_ = qMax(scale_ * 0.8, 0.1);
    clampPan();
    emit scaleChanged(scale_);
    update();
}

void ImageInspectionCanvas::clampPan()
{
    if (originalPix_.isNull()) {
        panOffset_ = QPoint(0, 0);
        return;
    }

    const int scaledW = static_cast<int>(originalPix_.width() * scale_);
    const int scaledH = static_cast<int>(originalPix_.height() * scale_);

    const int maxPanX = qMax(0, (scaledW - width()) / 2 + 100);
    const int maxPanY = qMax(0, (scaledH - height()) / 2 + 100);

    panOffset_.setX(qBound(-maxPanX, panOffset_.x(), maxPanX));
    panOffset_.setY(qBound(-maxPanY, panOffset_.y(), maxPanY));
}

void ImageInspectionCanvas::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // Deep modern darkroom backdrop
    p.fillRect(rect(), QColor("#070A11"));

    if (originalPix_.isNull()) {
        p.setPen(QColor("#64748B"));
        QFont font("Segoe UI", 13, QFont::DemiBold);
        p.setFont(font);
        p.drawText(rect(), Qt::AlignCenter, "Loading image...");
        return;
    }

    const int scaledW = static_cast<int>(originalPix_.width() * scale_);
    const int scaledH = static_cast<int>(originalPix_.height() * scale_);

    const int x = (width() - scaledW) / 2 + panOffset_.x();
    const int y = (height() - scaledH) / 2 + panOffset_.y();

    // Subtle drop shadow behind image
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 90));
    p.drawRoundedRect(x - 4, y - 4, scaledW + 8, scaledH + 8, 4, 4);

    // Draw high-resolution pixmap cleanly without distracting overlays
    p.drawPixmap(QRect(x, y, scaledW, scaledH), originalPix_);
}

void ImageInspectionCanvas::wheelEvent(QWheelEvent *event)
{
    const int numDegrees = event->angleDelta().y();
    if (numDegrees > 0) {
        zoomIn();
    } else if (numDegrees < 0) {
        zoomOut();
    }
    event->accept();
}

void ImageInspectionCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        lastMousePos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
}

void ImageInspectionCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging_) {
        const QPoint delta = event->pos() - lastMousePos_;
        panOffset_ += delta;
        lastMousePos_ = event->pos();
        clampPan();
        update();
        event->accept();
    }
}

void ImageInspectionCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
        setCursor(Qt::OpenHandCursor);
        event->accept();
    }
}

void ImageInspectionCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (qAbs(scale_ - fitScale_) < 0.05) {
            scale_ = 1.0; // 100% actual pixel size
        } else {
            scale_ = fitScale_; // Fit to screen
        }
        clampPan();
        emit scaleChanged(scale_);
        update();
        event->accept();
    }
}

// ============================================================================
// ImageInspectionDialog Implementation
// ============================================================================

ImageInspectionDialog::ImageInspectionDialog(const QVector<InspectionMediaItem> &items,
                                             int initialIndex,
                                             const QString &bookTitle,
                                             QWidget *parent)
    : QDialog(parent, Qt::Window | Qt::FramelessWindowHint),
      items_(items),
      currentIndex_(initialIndex),
      bookTitle_(bookTitle)
{
    setupUI();
    showMediaAtIndex(currentIndex_);
}

ImageInspectionDialog::~ImageInspectionDialog()
{
    if (videoPlayer_) {
        videoPlayer_->stop();
    }
}

void ImageInspectionDialog::setupUI()
{
    setStyleSheet("background-color: #0B0F19; color: #FFFFFF; font-family: 'Segoe UI', sans-serif;");

    // Center dialog with responsive sizing (94% of screen/parent)
    QScreen *screen = QApplication::primaryScreen();
    if (parentWidget()) {
        const QRect pGeom = parentWidget()->geometry();
        const int w = qMin(1360, static_cast<int>(pGeom.width() * 0.94));
        const int h = qMin(880, static_cast<int>(pGeom.height() * 0.94));
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(w, h), pGeom));
    } else if (screen) {
        const QRect geom = screen->availableGeometry();
        const int w = qMin(1360, static_cast<int>(geom.width() * 0.94));
        const int h = qMin(880, static_cast<int>(geom.height() * 0.94));
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(w, h), geom));
    } else {
        resize(1200, 800);
    }

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---------------------------------------------------------
    // Top Bar (Amazon-Style Navigation, Drag Handle & Zoom Bar)
    // ---------------------------------------------------------
    QFrame *topBar = new QFrame(this);
    topBar_ = topBar;
    topBar->setFixedHeight(58);
    topBar->setStyleSheet(R"(
        QFrame {
            background-color: #111827;
            border-bottom: 1px solid #1F2937;
        }
    )");

    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(20, 0, 20, 0);
    topLayout->setSpacing(12);

    // Title
    headerTitleLabel_ = new QLabel(bookTitle_.isEmpty() ? "Book Inspection" : bookTitle_, topBar);
    headerTitleLabel_->setStyleSheet("color: #F8FAFC; font-size: 15px; font-weight: 700;");
    headerTitleLabel_->setMaximumWidth(450);
    headerTitleLabel_->setToolTip(bookTitle_);
    topLayout->addWidget(headerTitleLabel_);

    counterBadge_ = new QLabel("1 / 1", topBar);
    counterBadge_->setStyleSheet(R"(
        background-color: #1E293B;
        color: #94A3B8;
        border: 1px solid #334155;
        border-radius: 6px;
        padding: 3px 10px;
        font-size: 12px;
        font-weight: 700;
    )");
    topLayout->addWidget(counterBadge_);

    topLayout->addStretch();

    // Zoom Controls
    zoomBadge_ = new QLabel("Fit", topBar);
    zoomBadge_->setStyleSheet("color: #CBD5E1; font-size: 12px; font-weight: 600; padding-right: 4px;");
    topLayout->addWidget(zoomBadge_);

    auto createControlBtn = [topBar](const QString &text, const QString &tooltip) {
        QPushButton *btn = new QPushButton(text, topBar);
        btn->setFixedSize(34, 34);
        btn->setToolTip(tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: #1F2937;
                color: #F8FAFC;
                border: 1px solid #374151;
                border-radius: 6px;
                font-size: 14px;
                font-weight: 700;
            }
            QPushButton:hover {
                background-color: #374151;
                border-color: #4B5563;
                color: #FFFFFF;
            }
            QPushButton:pressed {
                background-color: #4F46E5;
            }
        )");
        return btn;
    };

    zoomOutBtn_ = createControlBtn("−", "Zoom Out (Shortcut: -)");
    connect(zoomOutBtn_, &QPushButton::clicked, this, [this]() { if (canvas_) canvas_->zoomOut(); });
    topLayout->addWidget(zoomOutBtn_);

    zoomInBtn_ = createControlBtn("+", "Zoom In (Shortcut: +)");
    connect(zoomInBtn_, &QPushButton::clicked, this, [this]() { if (canvas_) canvas_->zoomIn(); });
    topLayout->addWidget(zoomInBtn_);

    resetZoomBtn_ = createControlBtn("⊡", "Fit to Screen / 100% (Shortcut: 0 or Double-click)");
    connect(resetZoomBtn_, &QPushButton::clicked, this, [this]() { if (canvas_) canvas_->resetZoom(); });
    topLayout->addWidget(resetZoomBtn_);

    maxRestoreBtn_ = createControlBtn("⛶", "Toggle Full Screen Preview (Shortcut: F or F11)");
    connect(maxRestoreBtn_, &QPushButton::clicked, this, &ImageInspectionDialog::toggleFullScreen);
    topLayout->addWidget(maxRestoreBtn_);

    QFrame *sep = new QFrame(topBar);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color: #374151;");
    sep->setFixedHeight(24);
    topLayout->addWidget(sep);

    // Close Button
    closeBtn_ = new QPushButton("✕", topBar);
    closeBtn_->setFixedSize(36, 36);
    closeBtn_->setToolTip("Close Preview (Shortcut: ESC)");
    closeBtn_->setCursor(Qt::PointingHandCursor);
    closeBtn_->setStyleSheet(R"(
        QPushButton {
            background-color: #EF4444;
            color: #FFFFFF;
            border: none;
            border-radius: 6px;
            font-size: 16px;
            font-weight: 800;
        }
        QPushButton:hover {
            background-color: #DC2626;
        }
        QPushButton:pressed {
            background-color: #B91C1C;
        }
    )");
    connect(closeBtn_, &QPushButton::clicked, this, [this]() {
        if (videoPlayer_) videoPlayer_->stop();
        accept();
    });
    topLayout->addWidget(closeBtn_);

    rootLayout->addWidget(topBar);

    // ---------------------------------------------------------
    // Main Body: Left Thumbnail Filmstrip + Center Media Stack
    // ---------------------------------------------------------
    QHBoxLayout *bodyLayout = new QHBoxLayout();
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // Left Filmstrip
    thumbScrollArea_ = new QScrollArea(this);
    thumbScrollArea_->setFixedWidth(108);
    thumbScrollArea_->setWidgetResizable(true);
    thumbScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    thumbScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    thumbScrollArea_->setFrameShape(QFrame::NoFrame);
    thumbScrollArea_->setStyleSheet(R"(
        QScrollArea {
            background-color: #0E131F;
            border-right: 1px solid #1F2937;
        }
        QScrollBar:vertical {
            border: none;
            background: #0E131F;
            width: 6px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #334155;
            min-height: 24px;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical:hover {
            background: #6366F1;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");

    QWidget *thumbContainer = new QWidget();
    thumbContainer->setStyleSheet("background: transparent;");
    thumbLayout_ = new QVBoxLayout(thumbContainer);
    thumbLayout_->setContentsMargins(12, 14, 12, 14);
    thumbLayout_->setSpacing(10);

    for (int i = 0; i < items_.size(); ++i) {
        const auto &item = items_[i];
        QPushButton *tBtn = new QPushButton(thumbContainer);
        tBtn->setFixedSize(76, 94);
        tBtn->setCursor(Qt::PointingHandCursor);
        tBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #1E293B;
                border: 2px solid #334155;
                border-radius: 8px;
            }
            QPushButton:hover {
                border-color: #6366F1;
            }
        )");

        QLabel *lbl = new QLabel(tBtn);
        lbl->setGeometry(3, 3, 70, 88);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("background: transparent; border-radius: 6px;");

        if (item.isVideo) {
            VideoThumbnailHelper::loadThumbnailAsync(item.url, lbl, QSize(70, 88), item.title, QString());
        } else {
            ImageLoader::instance().load(item.url, lbl, QSize(70, 88), bookTitle_);
        }

        connect(tBtn, &QPushButton::clicked, this, [this, i]() {
            showMediaAtIndex(i);
        });

        thumbLayout_->addWidget(tBtn);
        thumbButtons_.append(tBtn);
    }

    thumbLayout_->addStretch();
    thumbScrollArea_->setWidget(thumbContainer);
    bodyLayout->addWidget(thumbScrollArea_);

    // Center Container with Stacked Canvas + Native Video Player
    canvasContainer_ = new QFrame(this);
    canvasContainer_->setStyleSheet("background-color: #070A11; border: none;");

    QVBoxLayout *canvasContainerLayout = new QVBoxLayout(canvasContainer_);
    canvasContainerLayout->setContentsMargins(0, 0, 0, 0);
    canvasContainerLayout->setSpacing(0);

    mediaStack_ = new QStackedWidget(canvasContainer_);
    mediaStack_->setStyleSheet("background-color: #070A11; border: none;");

    // Page 0: High-Resolution Pan & Zoom Photo Canvas
    canvas_ = new ImageInspectionCanvas(mediaStack_);
    connect(canvas_, &ImageInspectionCanvas::scaleChanged, this, &ImageInspectionDialog::updateZoomBadge);
    mediaStack_->addWidget(canvas_);

    // Page 1: Native Embedded Video Player
    videoPlayer_ = new VideoPlayerWidget(mediaStack_);
    mediaStack_->addWidget(videoPlayer_);

    canvasContainerLayout->addWidget(mediaStack_, 1);

    // Floating Overlay Prev / Next Buttons
    prevBtn_ = new QPushButton("❮", canvasContainer_);
    prevBtn_->setFixedSize(46, 46);
    prevBtn_->setCursor(Qt::PointingHandCursor);
    prevBtn_->setToolTip("Previous Media (Shortcut: Left Arrow or A)");
    prevBtn_->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(30, 41, 59, 0.88);
            color: #FFFFFF;
            border: 1.5px solid #475569;
            border-radius: 23px;
            font-size: 18px;
            font-weight: 800;
        }
        QPushButton:hover {
            background-color: #4F46E5;
            border-color: #818CF8;
        }
        QPushButton:pressed {
            background-color: #3730A3;
        }
    )");
    connect(prevBtn_, &QPushButton::clicked, this, &ImageInspectionDialog::showPrevious);

    nextBtn_ = new QPushButton("❯", canvasContainer_);
    nextBtn_->setFixedSize(46, 46);
    nextBtn_->setCursor(Qt::PointingHandCursor);
    nextBtn_->setToolTip("Next Media (Shortcut: Right Arrow or D)");
    nextBtn_->setStyleSheet(prevBtn_->styleSheet());
    connect(nextBtn_, &QPushButton::clicked, this, &ImageInspectionDialog::showNext);

    bodyLayout->addWidget(canvasContainer_, 1);
    rootLayout->addLayout(bodyLayout, 1);
}

void ImageInspectionDialog::updateZoomBadge(double scale)
{
    if (!zoomBadge_) return;
    const int percent = static_cast<int>(scale * 100 + 0.5);
    zoomBadge_->setText(QString("%1%").arg(percent));
}

void ImageInspectionDialog::updateOverlayPositions()
{
    if (!canvasContainer_ || !prevBtn_ || !nextBtn_) return;

    const int h = canvasContainer_->height();
    const int w = canvasContainer_->width();

    prevBtn_->move(20, (h - prevBtn_->height()) / 2);
    nextBtn_->move(w - nextBtn_->width() - 20, (h - nextBtn_->height()) / 2);

    const bool hasMultiple = items_.size() > 1;
    prevBtn_->setVisible(hasMultiple);
    nextBtn_->setVisible(hasMultiple);

    prevBtn_->raise();
    nextBtn_->raise();
}

void ImageInspectionDialog::updateThumbnailSelection()
{
    for (int i = 0; i < thumbButtons_.size(); ++i) {
        if (i == currentIndex_) {
            thumbButtons_[i]->setStyleSheet(R"(
                QPushButton {
                    background-color: #1E293B;
                    border: 3px solid #6366F1;
                    border-radius: 8px;
                }
            )");
            if (thumbScrollArea_) thumbScrollArea_->ensureWidgetVisible(thumbButtons_[i]);
        } else {
            thumbButtons_[i]->setStyleSheet(R"(
                QPushButton {
                    background-color: #1E293B;
                    border: 2px solid #334155;
                    border-radius: 8px;
                }
                QPushButton:hover {
                    border-color: #4F46E5;
                }
            )");
        }
    }

    if (counterBadge_) {
        counterBadge_->setText(QString("%1 / %2").arg(currentIndex_ + 1).arg(items_.size()));
    }

    updateOverlayPositions();
}

void ImageInspectionDialog::showMediaAtIndex(int index)
{
    if (items_.isEmpty()) return;
    if (index < 0) index = 0;
    if (index >= items_.size()) index = items_.size() - 1;

    currentIndex_ = index;
    updateThumbnailSelection();

    const auto &item = items_[index];
    if (item.isVideo) {
        // Switch to playable Native Video Player
        if (zoomBadge_) zoomBadge_->setText("🎬 HD Video");
        if (zoomOutBtn_) zoomOutBtn_->setVisible(false);
        if (zoomInBtn_) zoomInBtn_->setVisible(false);
        if (resetZoomBtn_) resetZoomBtn_->setVisible(false);

        if (mediaStack_ && videoPlayer_) {
            mediaStack_->setCurrentWidget(videoPlayer_);
            videoPlayer_->setVisible(true);
            videoPlayer_->setVideoSource(item.url, bookTitle_);
        }
    } else {
        // Switch to Pan & Zoom High-Resolution Canvas
        if (videoPlayer_) {
            videoPlayer_->stop();
            videoPlayer_->setVisible(false);
        }

        if (zoomOutBtn_) zoomOutBtn_->setVisible(true);
        if (zoomInBtn_) zoomInBtn_->setVisible(true);
        if (resetZoomBtn_) resetZoomBtn_->setVisible(true);

        if (mediaStack_ && canvas_) {
            mediaStack_->setCurrentWidget(canvas_);
            canvas_->setImage(QPixmap());

            // Fetch high-resolution photo without downsampling
            ImageLoader::instance().fetchPixmap(item.url, QSize(2560, 2560), [this, index](const QPixmap &pix) {
                if (currentIndex_ == index && canvas_) {
                    canvas_->setImage(pix);
                }
            });
        }
    }

    updateOverlayPositions();
}

void ImageInspectionDialog::showPrevious()
{
    if (items_.isEmpty()) return;
    const int prev = (currentIndex_ - 1 + items_.size()) % items_.size();
    showMediaAtIndex(prev);
}

void ImageInspectionDialog::showNext()
{
    if (items_.isEmpty()) return;
    const int next = (currentIndex_ + 1) % items_.size();
    showMediaAtIndex(next);
}

void ImageInspectionDialog::toggleFullScreen()
{
    if (!isCustomMaximized_) {
        normalGeometry_ = geometry();
        isCustomMaximized_ = true;
        if (maxRestoreBtn_) maxRestoreBtn_->setText("❐");
        showMaximized();
    } else {
        isCustomMaximized_ = false;
        if (maxRestoreBtn_) maxRestoreBtn_->setText("⛶");
        showNormal();
        if (normalGeometry_.isValid()) {
            setGeometry(normalGeometry_);
        }
    }
    if (canvas_ && mediaStack_ && mediaStack_->currentWidget() == canvas_) {
        canvas_->resetZoom();
    }
    updateOverlayPositions();
}

void ImageInspectionDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    updateOverlayPositions();
}

void ImageInspectionDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && topBar_ && topBar_->geometry().contains(event->pos())) {
        isDraggingWindow_ = true;
        dragStartPos_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

void ImageInspectionDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (isDraggingWindow_ && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragStartPos_);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void ImageInspectionDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDraggingWindow_ = false;
    }
    QDialog::mouseReleaseEvent(event);
}

void ImageInspectionDialog::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();
    if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        if (videoPlayer_) videoPlayer_->stop();
        accept();
        event->accept();
        return;
    }
    if (key == Qt::Key_Left || key == Qt::Key_A) {
        showPrevious();
        event->accept();
        return;
    }
    if (key == Qt::Key_Right || key == Qt::Key_D) {
        showNext();
        event->accept();
        return;
    }
    if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
        if (canvas_ && mediaStack_ && mediaStack_->currentWidget() == canvas_) {
            canvas_->zoomIn();
        }
        event->accept();
        return;
    }
    if (key == Qt::Key_Minus) {
        if (canvas_ && mediaStack_ && mediaStack_->currentWidget() == canvas_) {
            canvas_->zoomOut();
        }
        event->accept();
        return;
    }
    if (key == Qt::Key_0) {
        if (canvas_ && mediaStack_ && mediaStack_->currentWidget() == canvas_) {
            canvas_->resetZoom();
        }
        event->accept();
        return;
    }
    if (key == Qt::Key_Space) {
        if (currentIndex_ >= 0 && currentIndex_ < items_.size() && items_[currentIndex_].isVideo) {
            if (videoPlayer_) {
                if (videoPlayer_->isPlaying()) videoPlayer_->pause();
                else videoPlayer_->play();
            }
            event->accept();
            return;
        } else {
            if (canvas_) canvas_->resetZoom();
            event->accept();
            return;
        }
    }
    if (key == Qt::Key_F11 || key == Qt::Key_F) {
        toggleFullScreen();
        event->accept();
        return;
    }

    QDialog::keyPressEvent(event);
}
