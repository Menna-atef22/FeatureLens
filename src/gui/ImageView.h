#pragma once
#include <QGraphicsView>
#include <QImage>

class QLabel;
class QGraphicsPixmapItem;

// Image viewer like a web image viewer:
//   mouse wheel = zoom (around the cursor), drag = pan, double-click = fit,
//   overlay buttons  - / + / Fit.
// Two views can be linked so zoom and pan stay in sync (original vs result).
class ImageView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ImageView(const QString& title = "", QWidget* parent = nullptr);

    void setImage(const QImage& img);
    void clearImage();
    bool hasImage() const { return has_; }

    void setBadge(const QString& text);        // small pill at the top-left
    void setEmptyText(const QString& text);    // text shown when there is no image
    void linkWith(ImageView* other);           // keep zoom / pan in sync
    void fitToView();

protected:
    void wheelEvent(QWheelEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void paintEvent(QPaintEvent* e) override;

private:
    void zoomBy(double factor);
    void syncPeer();
    void layoutOverlays();
    void updateZoomUi();

    QGraphicsPixmapItem* item_ = nullptr;
    QLabel*  badge_ = nullptr;
    QLabel*  zoomLabel_ = nullptr;
    QWidget* bar_ = nullptr;
    ImageView* peer_ = nullptr;
    QString emptyText_ = "Nothing to show yet";
    QSize   imgSize_;
    bool    has_ = false;
    bool    fit_ = true;
    bool    syncing_ = false;
};
