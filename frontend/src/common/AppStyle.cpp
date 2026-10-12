#include "AppStyle.h"
#include <QGraphicsDropShadowEffect>

QString AppStyle::appFont()
{
    return R"("Segoe UI", -apple-system, BlinkMacSystemFont, "Roboto", "Helvetica Neue", sans-serif)";
}

QString AppStyle::baseWindowStyle()
{
    return QString(R"(
        QWidget {
            font-family: %1;
            color: %2;
        }
        QLabel {
            border: none;
            background: transparent;
        }
        QScrollArea {
            border: none;
            background-color: %3;
        }
        QPushButton, QToolButton {
            cursor: pointer;
        }
    )").arg(appFont(), TextPrimary, Background);
}

QString AppStyle::primaryButtonStyle()
{
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: #FFFFFF;
            border: none;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 16px;
            cursor: pointer;
        }
        QPushButton:hover {
            background-color: %2;
        }
        QPushButton:pressed {
            background-color: %3;
        }
        QPushButton:focus {
            border: 2px solid %4;
        }
        QPushButton:disabled {
            background-color: %5;
            color: %6;
            cursor: default;
        }
    )").arg(Primary, PrimaryHover, PrimaryActive, PrimaryBorder, BorderSubtle, TextMuted);
}

QString AppStyle::secondaryButtonStyle()
{
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 16px;
            cursor: pointer;
        }
        QPushButton:hover {
            background-color: %4;
            border-color: %5;
        }
        QPushButton:pressed {
            background-color: %3;
        }
        QPushButton:focus {
            border: 1.5px solid %6;
        }
        QPushButton:disabled {
            background-color: %7;
            color: %8;
            border-color: %7;
            cursor: default;
        }
    )").arg(Surface, TextPrimary, BorderSubtle, SurfaceSubtle, BorderStrong, Primary, SurfaceSubtle, TextMuted);
}

QString AppStyle::ghostButtonStyle()
{
    return QString(R"(
        QPushButton {
            background-color: transparent;
            color: %1;
            border: none;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 14px;
            cursor: pointer;
        }
        QPushButton:hover {
            color: %2;
            background-color: %3;
        }
        QPushButton:pressed {
            background-color: #E0E7FF;
        }
        QPushButton:focus {
            border: 1.5px solid %4;
        }
    )").arg(TextSecondary, Primary, PrimaryLight, PrimaryBorder);
}

QString AppStyle::dangerButtonStyle()
{
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: #FFFFFF;
            border: none;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 16px;
            cursor: pointer;
        }
        QPushButton:hover {
            background-color: %2;
        }
        QPushButton:pressed {
            background-color: #991B1B;
        }
        QPushButton:focus {
            border: 2px solid %3;
        }
    )").arg(Danger, DangerHover, DangerBorder);
}

QString AppStyle::inputStyle()
{
    return QString(R"(
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox {
            background-color: #FFFFFF;
            color: %1;
            border: 1px solid %2;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13px;
            selection-background-color: %3;
            selection-color: #FFFFFF;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 1.5px solid %3;
            background-color: #FFFFFF;
        }
        QLineEdit:disabled, QTextEdit:disabled {
            background-color: %4;
            color: %5;
        }
    )").arg(TextPrimary, BorderSubtle, Primary, SurfaceSubtle, TextMuted);
}

QString AppStyle::comboBoxStyle()
{
    return QString(R"(
        QComboBox {
            background-color: #FFFFFF;
            color: %1;
            border: 1px solid %2;
            border-radius: 8px;
            padding: 6px 12px;
            font-size: 13px;
            min-height: 24px;
        }
        QComboBox:hover {
            border-color: %3;
        }
        QComboBox:focus {
            border: 1.5px solid %4;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 28px;
            border-left: 1px solid %2;
            border-top-right-radius: 8px;
            border-bottom-right-radius: 8px;
            background-color: %5;
        }
        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            color: %1;
            border: 1px solid %2;
            selection-background-color: %6;
            selection-color: %4;
            outline: none;
            padding: 4px;
        }
    )").arg(TextPrimary, BorderSubtle, BorderStrong, Primary, SurfaceSubtle, PrimaryLight);
}

QString AppStyle::cardStyle()
{
    return QString(R"(
        .QFrame, QFrame#card {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 12px;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )").arg(BorderSubtle);
}

QString AppStyle::statCardStyle()
{
    return QString(R"(
        .QFrame {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 12px;
            padding: 12px;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )").arg(BorderSubtle);
}

QString AppStyle::navBarStyle()
{
    return QString(R"(
        QFrame#navigationBar {
            background-color: #FFFFFF;
            border-bottom: 1px solid %1;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )").arg(BorderSubtle);
}

QString AppStyle::navButtonStyle()
{
    return QString(R"(
        QPushButton {
            background: transparent;
            border: none;
            color: %1;
            padding: 0 12px;
            font-size: 14px;
            font-weight: 600;
            border-radius: 8px;
        }
        QPushButton:hover {
            color: %2;
            background-color: %3;
        }
        QPushButton:pressed {
            background-color: #E0E7FF;
        }
    )").arg(TextSecondary, Primary, PrimaryLight);
}

QString AppStyle::badgeStyle(const QString &bg, const QString &textColor, const QString &border)
{
    QString borderRule = border.isEmpty() ? "border: none;" : QString("border: 1px solid %1;").arg(border);
    return QString(R"(
        QLabel {
            background-color: %1;
            color: %2;
            %3
            border-radius: 6px;
            padding: 3px 8px;
            font-size: 11px;
            font-weight: 700;
        }
    )").arg(bg, textColor, borderRule);
}

QString AppStyle::statusBadgeStyle(const QString &status)
{
    QString s = status.toUpper().trimmed();
    if (s == "DELIVERED" || s == "ACCEPTED" || s == "AVAILABLE") {
        return badgeStyle(SuccessLight, SuccessText, SuccessBorder);
    } else if (s == "CANCELLED" || s == "REJECTED" || s == "RETURN_REJECTED") {
        return badgeStyle(DangerLight, DangerText, DangerBorder);
    } else if (s == "RETURN_REQUESTED" || s == "RETURNED" || s == "PENDING" || s == "EXCHANGED") {
        return badgeStyle(WarningLight, WarningText, WarningBorder);
    } else if (s == "SOLD") {
        return badgeStyle(SurfaceSubtle, TextSecondary, BorderSubtle);
    } else {
        // PLACED, CONFIRMED, PACKED, SHIPPED, OUT_FOR_DELIVERY, or any in-flight status
        return badgeStyle(InfoLight, InfoText, InfoBorder);
    }
}

QString AppStyle::tableStyle()
{
    return QString(R"(
        QTableWidget {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 8px;
            gridline-color: %1;
            font-size: 13px;
            selection-background-color: %2;
            selection-color: %3;
        }
        QHeaderView::section {
            background-color: %4;
            color: %5;
            padding: 8px 12px;
            border: none;
            border-bottom: 1px solid %1;
            font-weight: 700;
            font-size: 12px;
        }
    )").arg(BorderSubtle, PrimaryLight, TextPrimary, SurfaceSubtle, TextSecondary);
}

QString AppStyle::scrollBarStyle()
{
    return QString(R"(
        QScrollBar:vertical {
            border: none;
            background: #F8FAFC;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #CBD5E1;
            min-height: 24px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background: #94A3B8;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            border: none;
            background: #F8FAFC;
            height: 8px;
            margin: 0px;
        }
        QScrollBar::handle:horizontal {
            background: #CBD5E1;
            min-width: 24px;
            border-radius: 4px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #94A3B8;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
    )");
}

QString AppStyle::activeTabStyle()
{
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: #FFFFFF;
            border: none;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 700;
            padding: 8px 18px;
            cursor: pointer;
        }
    )").arg(Primary);
}

QString AppStyle::inactiveTabStyle()
{
    return QString(R"(
        QPushButton {
            background-color: transparent;
            color: %1;
            border: 1px solid %2;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 18px;
            cursor: pointer;
        }
        QPushButton:hover {
            background-color: %3;
            color: %4;
        }
    )").arg(TextSecondary, BorderSubtle, SurfaceSubtle, TextPrimary);
}

void AppStyle::applyElevation(QWidget *widget, int blurRadius, int yOffset, int alpha)
{
    if (!widget) return;
    // CRITICAL PERFORMANCE FIX:
    // QGraphicsDropShadowEffect triggers multi-pass software CPU Gaussian rasterization
    // on every repaint and every scroll tick. When applied to repeating elements in a QScrollArea
    // (such as cards, chips, list rows), this bottlenecks the GUI thread and causes heavy scroll lag.
    // We restrict QGraphicsDropShadowEffect strictly to modal dialogs and top-level cards (blurRadius > 16).
    if (blurRadius <= 16) {
        return;
    }
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blurRadius);
    shadow->setOffset(0, yOffset);
    shadow->setColor(QColor(15, 23, 42, alpha));
    widget->setGraphicsEffect(shadow);
}

QString AppStyle::dialogCardStyle()
{
    return QString(R"(
        #dialogCard, #cancelCard {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 16px;
        }
    )").arg(BorderSubtle);
}

QString AppStyle::dialogHeaderStyle()
{
    return QString("color: %1; font-size: 18px; font-weight: 800;").arg(TextPrimary);
}

QString AppStyle::dialogSubheaderStyle()
{
    return QString("color: %1; font-size: 13px; line-height: 1.4;").arg(TextSecondary);
}

