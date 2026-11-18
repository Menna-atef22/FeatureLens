#pragma once
#include <QMainWindow>
#include "ImageStore.h"

class QButtonGroup; class QLabel; class QListWidget; class QPushButton; class QStackedWidget;

// Layout (like a web app):
//   sidebar (navigation)  |  header (title + Upload/Clear)
//                         |  page  (the selected tab)
//                         |  gallery strip with thumbnails of all loaded images
// Drag & drop images (or folders) anywhere on the window to add them.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private slots:
    void onLoadImages();
    void onStoreChanged();
    void onNavChanged(int index);

private:
    void loadPaths(const QStringList& paths);
    void rebuildGallery();

    ImageStore*     store_;
    QStackedWidget* outer_;       // 0 = empty state (drop zone), 1 = workspace
    QStackedWidget* pages_;       // one page per algorithm
    QListWidget*    gallery_;
    QLabel*         galleryTitle_;
    QLabel*         pageTitle_;
    QLabel*         pageSub_;
    QButtonGroup*   nav_;
    QPushButton*    clearBtn_;
};
