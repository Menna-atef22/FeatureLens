#pragma once
#include <QApplication>
#include <QColor>
#include <QFont>
#include <QPalette>
#include <QSettings>
#include <QString>
#include <QStyle>
#include <algorithm>
#include <vector>

// Two looks: Dark (default, space/magenta like Bee-yond Sights) and Light.
// Every color lives in the table below as { name, dark value, light value }.
// To re-skin: edit the values here. The sidebar stays dark in both modes.
enum class ThemeMode { Dark, Light };

inline ThemeMode& currentThemeMode() {
    static ThemeMode m = ThemeMode::Dark;
    return m;
}

struct ThemeToken { const char* name; const char* dark; const char* light; };

inline const std::vector<ThemeToken>& themeTokens() {
    static const std::vector<ThemeToken> t = {
        // page + text
        {"bg",          "#14172c", "#f7f4fa"},
        {"text",        "#f3f3f4", "#2a1a3d"},
        {"title",       "#ffffff", "#2a1a3d"},
        {"muted",       "#a095b5", "#7a6a90"},
        {"hint",        "#8a7fa0", "#9a8cae"},
        {"label",       "#a79bb8", "#8a7a9e"},
        {"placeholder", "#b5a3c8", "#a58bb5"},
        // surfaces + borders
        {"surface",     "#1c1b36", "#ffffff"},
        {"header",      "#1a1a33", "#ffffff"},
        {"status",      "#181a31", "#ffffff"},
        {"border",      "#352a4d", "#eadff0"},
        {"border2",     "#4a3a63", "#d9c6e0"},
        {"hover",       "#272244", "#f9eef5"},
        {"tint",        "#3b1b44", "#fbe3f0"},
        {"tintText",    "#f08bc3", "#a41c6c"},
        // accent (magenta from the nav underline)
        {"accent",        "#a41c6c", "#a41c6c"},
        {"accentDark",    "#8a155a", "#8a155a"},
        {"accentLight",   "#ec6cb0", "#a41c6c"},
        {"accentPressed", "#6e1048", "#6e1048"},
        // drop zone / disabled
        {"dropBorder",  "#5a3a63", "#e3b5d0"},
        {"dropHover",   "#231f3d", "#fdf5f9"},
        {"disBg",       "#3a3050", "#e7dcec"},
        {"disText",     "#7d7192", "#a898b4"},
        {"disText2",    "#5e5476", "#cbb9d3"},
        {"disBorder",   "#2b2543", "#eadff0"},
        // slider / progress / table
        {"groove",      "#2f2a4a", "#ecdff0"},
        {"handleHover", "#f3d3e6", "#fbe3f0"},
        {"grid",        "#2a2545", "#f1e8f4"},
        {"alt",         "#1f1e3b", "#fbf7fc"},
        {"selBg",       "#5a1b4e", "#fbe3f0"},
        {"selText",     "#ffffff", "#2a1a3d"},
        {"thBg",        "#24213f", "#f8eef5"},
        // image views
        {"imgBg",       "#10122a", "#f1eaf4"},
        {"badgeBg",     "rgba(21,24,47,0.85)",   "rgba(42,26,61,0.80)"},
        {"zoomBg",      "rgba(28,27,54,0.92)",   "rgba(255,255,255,0.92)"},
        {"zoomText",    "#f3f3f4", "#4a1d4d"},
        {"scrollHover", "#6b4a7c", "#c4a3d0"},
    };
    return t;
}

inline QString themeColor(const char* name, ThemeMode m) {
    for (const auto& t : themeTokens())
        if (QString(t.name) == name) return QString(m == ThemeMode::Dark ? t.dark : t.light);
    return QString();
}

inline QString themeStyleSheet(ThemeMode m) {
    QString qss = R"(
        #Root, QMainWindow { background: @bg; }
        QWidget { color: @text; font-size: 13px; }
        QLabel, QCheckBox { background: transparent; }
        QToolTip { background: @accent; color: white; border: none; padding: 6px 8px; }
        QStatusBar { background: @status; color: @label; border-top: 1px solid @border; }
        QStatusBar::item { border: none; }
        QStatusBar QLabel { padding-left: 12px; }

        /* ---------- sidebar ---------- */
        #Sidebar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #2a1d40, stop:1 #15182f); }
        #Sidebar QLabel { color: #cdbfdc; }
        #Sidebar #Logo { color: white; font-size: 22px; font-weight: 800; }
        #Sidebar #LogoSub { color: #b5a3c8; font-size: 12px; }
        #Sidebar #NavSection { color: #8d7ea3; font-size: 11px; font-weight: 700; letter-spacing: 1px; }
        #Sidebar #SideTip { color: #8d7ea3; font-size: 11px; }
        QPushButton#NavBtn { background: transparent; color: #e6dcee; text-align: left;
                             padding: 11px 14px; border-radius: 10px; font-weight: 600; }
        QPushButton#NavBtn:hover { background: rgba(255,255,255,0.08); }
        QPushButton#NavBtn:checked { background: @accent; color: white; }

        /* ---------- header ---------- */
        #Header { background: @header; border-bottom: 1px solid @border; }
        #PageTitle { font-size: 20px; font-weight: 800; color: @title; }
        #PageSub { color: @muted; font-size: 12px; }

        /* ---------- cards ---------- */
        #Card { background: @surface; border: 1px solid @border; border-radius: 14px; }
        #CardTitle { font-size: 14px; font-weight: 700; color: @title; }
        #StatTile { background: @surface; border: 1px solid @border; border-radius: 14px; }
        #StatTitle { font-size: 10px; font-weight: 700; color: @label; letter-spacing: 1px; }
        #StatValue { font-size: 24px; font-weight: 800; color: @accentLight; }
        #Muted { color: @muted; }
        #Hint { color: @hint; font-size: 11px; }
        #ParamName { font-weight: 600; }
        #ParamValue { background: @tint; color: @tintText; border: 1px solid transparent; border-radius: 8px; padding: 3px 8px; font-weight: 700; }
        #ParamValue:hover { border: 1px solid @border2; }
        #ParamValue:focus { background: @surface; color: @text; border: 1px solid @accent; }
        #SoonBadge { background: @tint; color: @tintText; border-radius: 10px; padding: 4px 12px;
                     font-size: 11px; font-weight: 700; letter-spacing: 1px; }
        #SoonTitle { font-size: 22px; font-weight: 800; }

        /* ---------- drop zone ---------- */
        #DropZone { background: @surface; border: 2px dashed @dropBorder; border-radius: 22px; }
        #DropZone:hover { border: 2px dashed @accent; background: @dropHover; }
        #DropIcon { background: @tint; color: @accentLight; border-radius: 38px; font-size: 40px; font-weight: 300; }
        #DropTitle { font-size: 22px; font-weight: 800; }

        /* ---------- buttons ---------- */
        QPushButton { background: @accent; color: white; border: none; border-radius: 9px;
                      padding: 9px 16px; font-weight: 600; }
        QPushButton:hover { background: @accentDark; }
        QPushButton:pressed { background: @accentPressed; }
        QPushButton:disabled { background: @disBg; color: @disText; }
        QPushButton[kind="secondary"] { background: @surface; color: @accentLight; border: 1px solid @border2; }
        QPushButton[kind="secondary"]:hover { background: @hover; }
        QPushButton[kind="secondary"]:disabled { background: @status; color: @disText2; border: 1px solid @disBorder; }
        QPushButton[kind="ghost"] { background: transparent; color: @accentLight; padding: 5px 10px; }
        QPushButton[kind="ghost"]:hover { background: @tint; }
        QPushButton[kind="ghost"]:disabled { background: transparent; color: @disText2; }

        /* ---------- sliders ---------- */
        QSlider::groove:horizontal { height: 6px; background: @groove; border-radius: 3px; }
        QSlider::sub-page:horizontal { background: @accent; border-radius: 3px; }
        QSlider::handle:horizontal { background: white; border: 2px solid @accent; width: 14px; height: 14px;
                                     margin: -6px 0; border-radius: 9px; }
        QSlider::handle:horizontal:hover { background: @handleHover; }

        /* ---------- table ---------- */
        QTableWidget { background: @header; border: none; gridline-color: @grid; alternate-background-color: @alt;
                       selection-background-color: @selBg; selection-color: @selText; }
        QTableWidget::item { padding: 4px 8px; border: none; }
        QHeaderView::section { background: @thBg; color: @tintText; border: none; padding: 8px; font-weight: 700; }
        QHeaderView { background: transparent; }
        QTableCornerButton::section { background: @thBg; border: none; }

        /* ---------- progress ---------- */
        QProgressBar { background: @groove; border: none; border-radius: 3px; max-height: 6px; }
        QProgressBar::chunk { background: @accent; border-radius: 3px; }

        /* ---------- image views ---------- */
        QGraphicsView#ImageView { background: @imgBg; border: 1px solid @border; border-radius: 14px; }
        #ViewBadge { background: @badgeBg; color: white; border-radius: 10px;
                     padding: 3px 10px; font-weight: 600; font-size: 12px; }
        #ZoomBar, #ZoomBar QLabel { background: transparent; }
        #ZoomBar #ZoomLabel { color: @zoomText; font-weight: 700; background: @zoomBg;
                     border: 1px solid @border2; border-radius: 8px; padding: 4px 6px; }
        QPushButton#ZoomBtn { background: @zoomBg; color: @zoomText; border: 1px solid @border2;
                              border-radius: 8px; padding: 4px 9px; min-width: 14px; }
        QPushButton#ZoomBtn:hover { background: @tint; }

        /* ---------- gallery strip ---------- */
        #GalleryBar { background: @surface; border-top: 1px solid @border; }
        QListWidget#Gallery { background: transparent; border: none; outline: none; }
        QListWidget#Gallery::item { border-radius: 10px; padding: 4px; color: @label; border: 2px solid transparent; }
        QListWidget#Gallery::item:hover { background: @hover; }
        QListWidget#Gallery::item:selected { background: @tint; border: 2px solid @accent; color: @title; }

        /* ---------- inputs / scroll ---------- */
        QSpinBox, QDoubleSpinBox, QComboBox { background: @surface; border: 1px solid @border2; border-radius: 7px; padding: 5px 8px; }
        QScrollArea { background: transparent; border: none; }
        #Transparent { background: transparent; }
        QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
        QScrollBar::handle:vertical { background: @border2; border-radius: 4px; min-height: 30px; }
        QScrollBar::handle:vertical:hover { background: @scrollHover; }
        QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
        QScrollBar::handle:horizontal { background: @border2; border-radius: 4px; min-width: 30px; }
        QScrollBar::handle:horizontal:hover { background: @scrollHover; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
        QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
        QSplitter::handle { background: transparent; }
        QMenu { background: @surface; border: 1px solid @border; padding: 6px; }
        QMenu::item { padding: 7px 18px; border-radius: 6px; }
        QMenu::item:selected { background: @tint; }
    )";
    // replace longest token names first (so @accentDark wins over @accent)
    std::vector<ThemeToken> toks = themeTokens();
    std::sort(toks.begin(), toks.end(), [](const ThemeToken& a, const ThemeToken& b) {
        return QString(a.name).size() > QString(b.name).size();
    });
    for (const auto& t : toks)
        qss.replace(QString("@") + t.name, QString(m == ThemeMode::Dark ? t.dark : t.light));
    return qss;
}

// Kept for older code: the dark theme stylesheet.
inline QString purpleTheme(ThemeMode m = ThemeMode::Dark) { return themeStyleSheet(m); }

// Remember the user's choice between runs.
inline ThemeMode loadThemeMode() {
    QSettings s("FeatureLens", "FeatureLens");
    return s.value("theme", "dark").toString() == "light" ? ThemeMode::Light : ThemeMode::Dark;
}
inline void saveThemeMode(ThemeMode m) {
    QSettings s("FeatureLens", "FeatureLens");
    s.setValue("theme", m == ThemeMode::Light ? "light" : "dark");
}

// Fusion style + palette (so native widgets match) + the stylesheet. Safe to call again
// at any time to switch modes.
inline void applyTheme(QApplication& app, ThemeMode m) {
    currentThemeMode() = m;
    if (app.style()->objectName().compare("fusion", Qt::CaseInsensitive) != 0)
        app.setStyle("Fusion");
    auto c = [m](const char* n) { return QColor(themeColor(n, m)); };
    QPalette p;
    p.setColor(QPalette::Window,          c("bg"));
    p.setColor(QPalette::WindowText,      c("text"));
    p.setColor(QPalette::Base,            c("surface"));
    p.setColor(QPalette::AlternateBase,   c("alt"));
    p.setColor(QPalette::Text,            c("text"));
    p.setColor(QPalette::Button,          c("surface"));
    p.setColor(QPalette::ButtonText,      c("text"));
    p.setColor(QPalette::Highlight,       c("accent"));
    p.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    p.setColor(QPalette::ToolTipBase,     c("accent"));
    p.setColor(QPalette::ToolTipText,     QColor("#ffffff"));
    p.setColor(QPalette::PlaceholderText, c("placeholder"));
    app.setPalette(p);
    QFont f = app.font();
    f.setPointSizeF(10);
    app.setFont(f);
    app.setStyleSheet(themeStyleSheet(m));
}

// Call once from main(): applies the saved mode (dark by default).
inline void applyPurpleTheme(QApplication& app) { applyTheme(app, loadThemeMode()); }
