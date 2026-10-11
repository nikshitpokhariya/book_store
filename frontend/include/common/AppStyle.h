#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QString>
#include <QColor>
#include <QWidget>

class AppStyle {
public:
    // Color Palette Tokens
    static constexpr const char* Primary        = "#4F46E5"; // Indigo 600
    static constexpr const char* PrimaryHover   = "#4338CA"; // Indigo 700
    static constexpr const char* PrimaryActive  = "#3730A3"; // Indigo 800
    static constexpr const char* PrimaryLight   = "#EEF2FF"; // Indigo 50
    static constexpr const char* PrimaryBorder  = "#C7D2FE"; // Indigo 200

    static constexpr const char* Danger         = "#DC2626"; // Red 600
    static constexpr const char* DangerHover    = "#B91C1C"; // Red 700
    static constexpr const char* DangerLight    = "#FEE2E2"; // Red 50
    static constexpr const char* DangerText     = "#991B1B"; // Red 800
    static constexpr const char* DangerBorder   = "#FECACA"; // Red 200

    static constexpr const char* Success        = "#16A34A"; // Green 600
    static constexpr const char* SuccessLight   = "#D1FAE5"; // Green 50
    static constexpr const char* SuccessText    = "#065F46"; // Green 800
    static constexpr const char* SuccessBorder  = "#A7F3D0"; // Green 200

    static constexpr const char* Warning        = "#D97706"; // Amber 600
    static constexpr const char* WarningLight   = "#FEF3C7"; // Amber 50
    static constexpr const char* WarningText    = "#92400E"; // Amber 800
    static constexpr const char* WarningBorder  = "#FDE68A"; // Amber 200

    static constexpr const char* Info           = "#2563EB"; // Blue 600
    static constexpr const char* InfoLight      = "#DBEAFE"; // Blue 50
    static constexpr const char* InfoText       = "#1E40AF"; // Blue 800
    static constexpr const char* InfoBorder     = "#BFDBFE"; // Blue 200

    static constexpr const char* Background     = "#F8FAFC"; // Slate 50
    static constexpr const char* Surface        = "#FFFFFF";
    static constexpr const char* SurfaceSubtle  = "#F1F5F9"; // Slate 100
    static constexpr const char* BorderSubtle   = "#E2E8F0"; // Slate 200
    static constexpr const char* BorderStrong   = "#CBD5E1"; // Slate 300

    static constexpr const char* TextPrimary    = "#0F172A"; // Slate 900
    static constexpr const char* TextSecondary  = "#475569"; // Slate 600
    static constexpr const char* TextMuted      = "#94A3B8"; // Slate 400

    // Common QSS Stylesheets
    static QString appFont();
    static QString baseWindowStyle();
    static QString primaryButtonStyle();
    static QString secondaryButtonStyle();
    static QString ghostButtonStyle();
    static QString dangerButtonStyle();
    static QString inputStyle();
    static QString comboBoxStyle();
    static QString cardStyle();
    static QString statCardStyle();
    static QString navBarStyle();
    static QString navButtonStyle();
    static QString badgeStyle(const QString &bg, const QString &textColor, const QString &border = QString());
    static QString statusBadgeStyle(const QString &status);
    static QString tableStyle();
    static QString scrollBarStyle();
    static QString activeTabStyle();
    static QString inactiveTabStyle();

    // Elevation / shadow helper
    static void applyElevation(QWidget *widget, int blurRadius = 20, int yOffset = 4, int alpha = 25);
};

#endif // APPSTYLE_H
