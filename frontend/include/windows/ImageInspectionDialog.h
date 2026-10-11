#ifndef IMAGEINSPECTIONDIALOG_H
#define IMAGEINSPECTIONDIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>
#include <QPixmap>
#include <QPoint>

class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class QHBoxLayout;
class QFrame;
class QWheelEvent;
class QMouseEvent;
class QKeyEvent;
class QPaintEvent;

struct InspectionMediaItem {
    bool isVideo{false};
    QString url;
    QString title;
};

class ImageInspectionCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit ImageInspectionCanvas(QWidget *parent = nullptr);

    void setImage(const QPixmap &pix, const QString &statusText = QString());
    void zoomIn();
    void zoomOut();
    void resetZoom();
    double currentScale() const { return scale_; }

signals:
    void scaleChanged(double scale);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void clampPan();

    QPixmap originalPix_;
    QString statusText_;
    double scale_{1.0};
    double fitScale_{1.0};
    QPoint panOffset_{0, 0};
    bool dragging_{false};
    QPoint lastMousePos_;
};

class VideoPlayerWidget;
class QStackedWidget;

class ImageInspectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageInspectionDialog(const QVector<InspectionMediaItem> &items,
                                  int initialIndex,
                                  const QString &bookTitle,
                                  QWidget *parent = nullptr);
    ~ImageInspectionDialog() override;

public slots:
    void showMediaAtIndex(int index);
    void showPrevious();
    void showNext();
    void toggleFullScreen();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setupUI();
    void updateThumbnailSelection();
    void updateZoomBadge(double scale);
    void updateOverlayPositions();

    QVector<InspectionMediaItem> items_;
    int currentIndex_{0};
    QString bookTitle_;

    // UI elements
    QWidget *topBar_{nullptr};
    QLabel *headerTitleLabel_{nullptr};
    QLabel *counterBadge_{nullptr};
    QLabel *zoomBadge_{nullptr};
    QPushButton *zoomInBtn_{nullptr};
    QPushButton *zoomOutBtn_{nullptr};
    QPushButton *resetZoomBtn_{nullptr};
    QPushButton *maxRestoreBtn_{nullptr};
    QPushButton *closeBtn_{nullptr};

    QPushButton *prevBtn_{nullptr};
    QPushButton *nextBtn_{nullptr};

    QScrollArea *thumbScrollArea_{nullptr};
    QVBoxLayout *thumbLayout_{nullptr};
    QVector<QPushButton*> thumbButtons_;

    QFrame *canvasContainer_{nullptr};
    QStackedWidget *mediaStack_{nullptr};
    ImageInspectionCanvas *canvas_{nullptr};
    VideoPlayerWidget *videoPlayer_{nullptr};

    bool isDraggingWindow_{false};
    QPoint dragStartPos_;
    bool isCustomMaximized_{false};
    QRect normalGeometry_;
};

#endif // IMAGEINSPECTIONDIALOG_H
