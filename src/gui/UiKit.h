#pragma once
// Small reusable "web-style" building blocks. Teammates: use these in your tab
// so every tab looks the same (see HarrisTab.cpp for an example).
#include <QFrame>
#include <QString>

class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QVBoxLayout;
class QMouseEvent;

// White rounded panel with an optional title. Add your widgets to body().
class Card : public QFrame {
    Q_OBJECT
public:
    explicit Card(const QString& title = "", QWidget* parent = nullptr);
    QVBoxLayout* body() const { return body_; }
private:
    QVBoxLayout* body_;
};

// Label + slider + live value, e.g. "k   0.04". The value box is editable:
// type a number and press Enter (or click elsewhere) and the slider follows.
class ParamSlider : public QWidget {
    Q_OBJECT
public:
    ParamSlider(const QString& label, double min, double max, double value,
                double step, int decimals, const QString& hint = "", QWidget* parent = nullptr);
    void   setLogScale(bool on);      // good for thresholds like 0.0001 .. 1
    double value() const;
    void   setValue(double v);        // does not emit valueChanged
signals:
    void valueChanged(double v);      // emitted continuously while dragging
private:
    void   updateText();
    void   commitText();               // called when the user finishes typing
    double fromSlider() const;
    QSlider*   slider_;
    QLineEdit* edit_;
    double     value_ = 0;
    double     min_, max_, step_;
    int        decimals_;
    bool       log_ = false;
};

// Big number with a small caption (e.g. "Corners  1,284").
class StatTile : public QFrame {
    Q_OBJECT
public:
    explicit StatTile(const QString& title, QWidget* parent = nullptr);
    void setValue(const QString& v);
private:
    QLabel* value_;
};

// Big "drop images here" area shown when nothing is loaded.
class DropZone : public QFrame {
    Q_OBJECT
public:
    explicit DropZone(QWidget* parent = nullptr);
signals:
    void browseRequested();
protected:
    void mousePressEvent(QMouseEvent* e) override;
};

// kind: "primary" (filled), "secondary" (outlined), "ghost" (text only)
QPushButton* makeButton(const QString& text, const QString& kind = "primary", QWidget* parent = nullptr);

// Nice placeholder page for tabs that are not implemented yet.
QWidget* makeComingSoonPage(const QString& title, const QString& owner, const QString& hint);
