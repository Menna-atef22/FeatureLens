#include "ImageView.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QWheelEvent>
#include <algorithm>

ImageView::ImageView(const QString& title, QWidget* parent) : QGraphicsView(parent) {
    setObjectName("ImageView");
    setScene(new QGraphicsScene(this));
    item_ = scene()->addPixmap(QPixmap());
    item_->setTransformationMode(Qt::SmoothTransformation);

    setFrameShape(QFrame::NoFrame);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMinimumSize(180, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ---- overlays (children of the viewport) ----
    badge_ = new QLabel(title, this);
    badge_->setObjectName("ViewBadge");
    badge_->setAttribute(Qt::WA_TransparentForMouseEvents);
    badge_->setVisible(!title.isEmpty());

    bar_ = new QWidget(this);
    bar_->setObjectName("ZoomBar");
    auto* l = new QHBoxLayout(bar_);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(4);
    auto mk = [&](const QString& t, const QString& tip) {
        auto* b = new QPushButton(t, bar_);
        b->setObjectName("ZoomBtn");
        b->setToolTip(tip);
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::NoFocus);
        l->addWidget(b);
        return b;
    };
    auto* minus = mk("-", "Zoom out");
    zoomLabel_ = new QLabel("100%", bar_);
    zoomLabel_->setObjectName("ZoomLabel");
    zoomLabel_->setAlignment(Qt::AlignCenter);
    zoomLabel_->setMinimumWidth(48);
    l->addWidget(zoomLabel_);
    auto* plus = mk("+", "Zoom in");
    auto* fit  = mk("Fit", "Fit to window (double-click)");
    connect(minus, &QPushButton::clicked, this, [this] { zoomBy(1 / 1.25); });
    connect(plus,  &QPushButton::clicked, this, [this] { zoomBy(1.25); });
    connect(fit,   &QPushButton::clicked, this, [this] { fitToView(); });
    bar_->hide();

    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this] { syncPeer(); });
    connect(verticalScrollBar(),   &QScrollBar::valueChanged, this, [this] { syncPeer(); });
}

void ImageView::setImage(const QImage& img) {
    const bool sameSize = has_ && img.size() == imgSize_;
    item_->setPixmap(QPixmap::fromImage(img));
    scene()->setSceneRect(item_->boundingRect());
    imgSize_ = img.size();
    has_ = true;
    bar_->show();
    layoutOverlays();
    if (!sameSize || fit_) fitToView();    // keep the zoom when only the pixels changed
    else updateZoomUi();
    viewport()->update();
}

void ImageView::clearImage() {
    item_->setPixmap(QPixmap());
    has_ = false;
    bar_->hide();
    viewport()->update();
}

void ImageView::setBadge(const QString& text) {
    badge_->setText(text);
    badge_->setVisible(!text.isEmpty());
    badge_->adjustSize();
}

void ImageView::setEmptyText(const QString& text) { emptyText_ = text; viewport()->update(); }

void ImageView::linkWith(ImageView* other) {
    peer_ = other;
    other->peer_ = this;
}

void ImageView::fitToView() {
    if (!has_) return;
    resetTransform();
    fitInView(item_, Qt::KeepAspectRatio);
    fit_ = true;
    updateZoomUi();
    syncPeer();
}

void ImageView::zoomBy(double factor) {
    if (!has_) return;
    const double s  = transform().m11();
    const double ns = std::clamp(s * factor, 0.02, 40.0);
    factor = ns / s;
    scale(factor, factor);
    fit_ = false;
    updateZoomUi();
    syncPeer();
}

void ImageView::syncPeer() {
    if (!peer_ || syncing_ || !has_ || !peer_->has_) return;
    peer_->syncing_ = true;
    peer_->setTransform(transform());
    peer_->fit_ = fit_;
    peer_->horizontalScrollBar()->setValue(horizontalScrollBar()->value());
    peer_->verticalScrollBar()->setValue(verticalScrollBar()->value());
    peer_->updateZoomUi();
    peer_->syncing_ = false;
}

void ImageView::updateZoomUi() {
    const double s = transform().m11();
    zoomLabel_->setText(QString::number(int(s * 100 + 0.5)) + "%");
    // pixel-accurate view when zoomed far in (easier to inspect single corners)
    item_->setTransformationMode(s > 2.0 ? Qt::FastTransformation : Qt::SmoothTransformation);
}

void ImageView::layoutOverlays() {
    badge_->adjustSize();
    badge_->move(12, 12);
    bar_->adjustSize();
    bar_->move(width() - bar_->width() - 12, height() - bar_->height() - 12);
    badge_->raise();
    bar_->raise();
}

void ImageView::wheelEvent(QWheelEvent* e) {
    if (!has_) { e->ignore(); return; }
    zoomBy(e->angleDelta().y() > 0 ? 1.15 : 1 / 1.15);
    e->accept();
}

void ImageView::resizeEvent(QResizeEvent* e) {
    QGraphicsView::resizeEvent(e);
    if (has_ && fit_) fitToView();
    layoutOverlays();
}

void ImageView::mouseDoubleClickEvent(QMouseEvent* e) {
    Q_UNUSED(e);
    fitToView();
}

void ImageView::paintEvent(QPaintEvent* e) {
    QGraphicsView::paintEvent(e);
    if (!has_) {
        QPainter p(viewport());
        p.setPen(palette().color(QPalette::PlaceholderText));
        QFont f = font();
        f.setPointSizeF(f.pointSizeF() * 1.15);
        p.setFont(f);
        p.drawText(viewport()->rect(), Qt::AlignCenter, emptyText_);
    }
}
