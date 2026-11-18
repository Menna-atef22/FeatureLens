#include "UiKit.h"
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMouseEvent>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------- Card
Card::Card(const QString& title, QWidget* parent) : QFrame(parent) {
    setObjectName("Card");
    body_ = new QVBoxLayout(this);
    body_->setContentsMargins(18, 16, 18, 16);
    body_->setSpacing(12);
    if (!title.isEmpty()) {
        auto* t = new QLabel(title);
        t->setObjectName("CardTitle");
        body_->addWidget(t);
    }
}

// ---------------------------------------------------------------- ParamSlider
ParamSlider::ParamSlider(const QString& label, double min, double max, double value,
                         double step, int decimals, const QString& hint, QWidget* parent)
    : QWidget(parent), min_(min), max_(max), step_(step), decimals_(decimals) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(5);

    auto* top = new QHBoxLayout;
    auto* name = new QLabel(label);
    name->setObjectName("ParamName");
    edit_ = new QLineEdit;
    edit_->setObjectName("ParamValue");
    edit_->setAlignment(Qt::AlignCenter);
    edit_->setFixedWidth(74);
    edit_->setToolTip("Click, type a value and press Enter");
    auto* val = new QDoubleValidator(-1e9, 1e9, std::max(0, decimals_), edit_);
    val->setNotation(QDoubleValidator::StandardNotation);
    val->setLocale(QLocale::c());
    edit_->setValidator(val);                 // numbers only; range is applied on commit
    top->addWidget(name);
    top->addStretch();
    top->addWidget(edit_);
    v->addLayout(top);

    slider_ = new QSlider(Qt::Horizontal);
    slider_->setCursor(Qt::PointingHandCursor);
    v->addWidget(slider_);

    if (!hint.isEmpty()) {
        auto* h = new QLabel(hint);
        h->setObjectName("Hint");
        h->setWordWrap(true);
        v->addWidget(h);
        setToolTip(hint);
    }

    slider_->setRange(0, std::max(1, int(std::lround((max_ - min_) / step_))));
    setValue(value);
    connect(slider_, &QSlider::valueChanged, this, [this] {
        value_ = fromSlider();
        updateText();
        emit valueChanged(value_);
    });
    connect(edit_, &QLineEdit::editingFinished, this, &ParamSlider::commitText);
}

void ParamSlider::setLogScale(bool on) {
    const double keep = value();
    log_ = on;
    slider_->blockSignals(true);
    slider_->setRange(0, on ? 1000 : std::max(1, int(std::lround((max_ - min_) / step_))));
    slider_->blockSignals(false);
    setValue(keep);
}

double ParamSlider::fromSlider() const {
    const int pos = slider_->value();
    if (log_) return min_ * std::pow(max_ / min_, pos / 1000.0);
    return std::min(max_, min_ + pos * step_);
}

double ParamSlider::value() const { return value_; }

void ParamSlider::setValue(double v) {
    v = std::clamp(v, min_, max_);
    // keep exactly what was typed, rounded to the digits that are displayed
    const double k = std::pow(10.0, std::max(0, decimals_));
    v = std::clamp(std::round(v * k) / k, min_, max_);
    int pos;
    if (log_) pos = int(std::lround(1000.0 * std::log(v / min_) / std::log(max_ / min_)));
    else      pos = int(std::lround((v - min_) / step_));
    value_ = v;
    slider_->blockSignals(true);
    slider_->setValue(pos);
    slider_->blockSignals(false);
    updateText();
}

void ParamSlider::commitText() {
    bool ok = false;
    const double v = edit_->text().trimmed().replace(',', '.').toDouble(&ok);
    if (!ok) { updateText(); return; }                // empty / invalid -> put the old value back
    const double before = value_;
    setValue(v);                                      // clamps to [min, max], moves the slider
    if (std::abs(value_ - before) > 1e-12) emit valueChanged(value_);
}

void ParamSlider::updateText() { edit_->setText(QString::number(value_, 'f', decimals_)); }

// ---------------------------------------------------------------- StatTile
StatTile::StatTile(const QString& title, QWidget* parent) : QFrame(parent) {
    setObjectName("StatTile");
    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(18, 12, 18, 12);
    l->setSpacing(2);
    auto* t = new QLabel(title.toUpper());
    t->setObjectName("StatTitle");
    value_ = new QLabel("-");
    value_->setObjectName("StatValue");
    l->addWidget(t);
    l->addWidget(value_);
}
void StatTile::setValue(const QString& v) { value_->setText(v); }

// ---------------------------------------------------------------- DropZone
DropZone::DropZone(QWidget* parent) : QFrame(parent) {
    setObjectName("DropZone");
    setCursor(Qt::PointingHandCursor);
    auto* l = new QVBoxLayout(this);
    l->setAlignment(Qt::AlignCenter);
    l->setSpacing(10);

    auto* icon = new QLabel("+");
    icon->setObjectName("DropIcon");
    icon->setAlignment(Qt::AlignCenter);
    icon->setFixedSize(76, 76);
    auto* title = new QLabel("Drop images here");
    title->setObjectName("DropTitle");
    title->setAlignment(Qt::AlignCenter);
    auto* sub = new QLabel("PNG, JPG, BMP or TIFF - you can also drop a whole folder");
    sub->setObjectName("Muted");
    sub->setAlignment(Qt::AlignCenter);
    auto* btn = makeButton("Browse files...");
    connect(btn, &QPushButton::clicked, this, &DropZone::browseRequested);

    l->addWidget(icon, 0, Qt::AlignCenter);
    l->addWidget(title);
    l->addWidget(sub);
    l->addSpacing(8);
    l->addWidget(btn, 0, Qt::AlignCenter);
}
void DropZone::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) emit browseRequested();
}

// ---------------------------------------------------------------- helpers
QPushButton* makeButton(const QString& text, const QString& kind, QWidget* parent) {
    auto* b = new QPushButton(text, parent);
    b->setProperty("kind", kind);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QWidget* makeComingSoonPage(const QString& title, const QString& owner, const QString& hint) {
    auto* page = new QWidget;
    auto* l = new QVBoxLayout(page);
    l->setAlignment(Qt::AlignCenter);
    auto* card = new Card;
    card->setMinimumWidth(460);
    card->body()->setSpacing(8);
    auto* badge = new QLabel("IN PROGRESS");
    badge->setObjectName("SoonBadge");
    badge->setAlignment(Qt::AlignCenter);
    auto* t = new QLabel(title);
    t->setObjectName("SoonTitle");
    t->setAlignment(Qt::AlignCenter);
    auto* o = new QLabel(owner + " - not implemented yet");
    o->setObjectName("Muted");
    o->setAlignment(Qt::AlignCenter);
    auto* h = new QLabel(hint);
    h->setObjectName("Hint");
    h->setAlignment(Qt::AlignCenter);
    h->setWordWrap(true);
    card->body()->addWidget(badge, 0, Qt::AlignCenter);
    card->body()->addWidget(t);
    card->body()->addWidget(o);
    card->body()->addWidget(h);
    l->addWidget(card, 0, Qt::AlignCenter);
    return page;
}
