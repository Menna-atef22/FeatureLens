#include "MainWindow.h"
#include "HarrisTab.h"
#include "MinEigenTab.h"
#include "SiftTab.h"
#include "MatchingTab.h"
#include "ImageUtils.h"
#include "UiKit.h"
#include "Paths.h"
#include "Theme.h"

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QUrl>
#include <QVBoxLayout>

namespace {
const QStringList kImageFilters = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.tif", "*.tiff"};

struct PageInfo { const char* nav; const char* title; const char* subtitle; };
const PageInfo kPages[] = {
    {"Harris corners",   "Harris Corner Detector",
     "Finds corners with the response R = det(M) - k * trace(M)^2. Tweak the sliders - the result updates live."},
    {"Min eigenvalue",   "Min Eigenvalue (\u03BB-) Detector",
     "Shi-Tomasi style corners: a point is a corner when the smaller eigenvalue of the structure tensor is large."},
    {"SIFT",             "SIFT Keypoints",
     "Scale-invariant keypoints and descriptors."},
    {"Matching",         "Feature Matching (SSD / NCC)",
     "Match features between two images using SSD or normalized cross-correlation."},
};
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("FeatureLens - Harris, SIFT & Matching");
    setMinimumSize(1100, 700);
    setAcceptDrops(true);
    store_ = new ImageStore(this);

    auto* root = new QWidget;
    root->setObjectName("Root");
    auto* rootL = new QHBoxLayout(root);
    rootL->setContentsMargins(0, 0, 0, 0);
    rootL->setSpacing(0);
    setCentralWidget(root);

    // ======================= sidebar =======================
    auto* side = new QFrame;
    side->setObjectName("Sidebar");
    side->setFixedWidth(230);
    auto* sl = new QVBoxLayout(side);
    sl->setContentsMargins(16, 22, 16, 16);
    sl->setSpacing(4);
    auto* logo = new QLabel("FeatureLens");
    logo->setObjectName("Logo");
    auto* logoSub = new QLabel("Feature detection lab");
    logoSub->setObjectName("LogoSub");
    sl->addWidget(logo);
    sl->addWidget(logoSub);
    sl->addSpacing(26);
    auto* section = new QLabel("ALGORITHMS");
    section->setObjectName("NavSection");
    sl->addWidget(section);
    sl->addSpacing(4);

    nav_ = new QButtonGroup(this);
    nav_->setExclusive(true);
    for (int i = 0; i < 4; ++i) {
        auto* b = new QPushButton(kPages[i].nav);
        b->setObjectName("NavBtn");
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)));
        b->setToolTip(QString("Ctrl+%1").arg(i + 1));
        nav_->addButton(b, i);
        sl->addWidget(b);
    }
    sl->addStretch();
    auto* tip = new QLabel("Tips\nScroll = zoom\nDrag = pan\nDouble-click = fit\nDrop files anywhere\nDel = remove image");
    tip->setObjectName("SideTip");
    sl->addWidget(tip);

    // ======================= right side =======================
    auto* right = new QWidget;
    auto* rl = new QVBoxLayout(right);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(0);

    // ---- header ----
    auto* header = new QFrame;
    header->setObjectName("Header");
    auto* hl = new QHBoxLayout(header);
    hl->setContentsMargins(26, 14, 26, 14);
    auto* titles = new QVBoxLayout;
    titles->setSpacing(2);
    pageTitle_ = new QLabel;
    pageTitle_->setObjectName("PageTitle");
    pageSub_ = new QLabel;
    pageSub_->setObjectName("PageSub");
    titles->addWidget(pageTitle_);
    titles->addWidget(pageSub_);
    hl->addLayout(titles, 1);
    auto* uploadBtn = makeButton("Upload images");
    uploadBtn->setToolTip("Ctrl+O");
    clearBtn_ = makeButton("Clear all", "secondary");
    // dark / light mode switch (remembered between runs)
    auto* themeBtn = makeButton("", "secondary");
    themeBtn->setShortcut(QKeySequence("Ctrl+T"));
    themeBtn->setToolTip("Switch dark / light mode (Ctrl+T)");
    auto refreshThemeBtn = [themeBtn] {
        themeBtn->setText(currentThemeMode() == ThemeMode::Dark ? "Light mode" : "Dark mode");
    };
    refreshThemeBtn();
    connect(themeBtn, &QPushButton::clicked, this, [refreshThemeBtn] {
        applyTheme(*qApp, currentThemeMode() == ThemeMode::Dark ? ThemeMode::Light : ThemeMode::Dark);
        saveThemeMode(currentThemeMode());
        refreshThemeBtn();
    });
    hl->addWidget(clearBtn_);
    hl->addWidget(themeBtn);
    hl->addWidget(uploadBtn);
    rl->addWidget(header);

    // ---- body: empty state / workspace ----
    outer_ = new QStackedWidget;
    auto* emptyPage = new QWidget;
    auto* el = new QVBoxLayout(emptyPage);
    el->setContentsMargins(40, 40, 40, 40);
    auto* drop = new DropZone;
    el->addWidget(drop);
    outer_->addWidget(emptyPage);

    auto* work = new QWidget;
    auto* wl = new QVBoxLayout(work);
    wl->setContentsMargins(0, 0, 0, 0);
    wl->setSpacing(0);
    pages_ = new QStackedWidget;
    auto* pageHolder = new QWidget;
    auto* ph = new QVBoxLayout(pageHolder);
    ph->setContentsMargins(26, 20, 26, 16);
    ph->addWidget(pages_);
    pages_->addWidget(new HarrisTab(store_));
    pages_->addWidget(new MinEigenTab(store_));
    pages_->addWidget(new SiftTab(store_));
    pages_->addWidget(new MatchingTab(store_));
    wl->addWidget(pageHolder, 1);

    // ---- gallery strip ----
    auto* gbar = new QFrame;
    gbar->setObjectName("GalleryBar");
    gbar->setFixedHeight(146);
    auto* gl = new QVBoxLayout(gbar);
    gl->setContentsMargins(20, 8, 20, 8);
    gl->setSpacing(2);
    auto* grow = new QHBoxLayout;
    galleryTitle_ = new QLabel;
    galleryTitle_->setObjectName("CardTitle");
    auto* addMore = makeButton("+ Add more", "ghost");
    grow->addWidget(galleryTitle_);
    grow->addStretch();
    grow->addWidget(addMore);
    gl->addLayout(grow);
    gallery_ = new QListWidget;
    gallery_->setObjectName("Gallery");
    gallery_->setViewMode(QListView::IconMode);
    gallery_->setFlow(QListView::LeftToRight);
    gallery_->setWrapping(false);
    gallery_->setMovement(QListView::Static);
    gallery_->setResizeMode(QListView::Adjust);
    gallery_->setIconSize(QSize(96, 64));
    gallery_->setGridSize(QSize(120, 96));
    gallery_->setSpacing(4);
    gallery_->setUniformItemSizes(true);
    gallery_->setTextElideMode(Qt::ElideRight);
    gallery_->setSelectionMode(QAbstractItemView::SingleSelection);
    gallery_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    gl->addWidget(gallery_);
    wl->addWidget(gbar);
    outer_->addWidget(work);

    rl->addWidget(outer_, 1);
    rootL->addWidget(side);
    rootL->addWidget(right, 1);

    // remove selected image: Del key or right-click
    auto* removeAct = new QAction("Remove image", this);
    removeAct->setShortcut(Qt::Key_Delete);
    removeAct->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    gallery_->addAction(removeAct);
    gallery_->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(removeAct, &QAction::triggered, this, [this] { store_->remove(store_->current()); });

    auto* openAct = new QAction(this);
    openAct->setShortcut(QKeySequence::Open);
    addAction(openAct);

    // ======================= wiring =======================
    connect(nav_, &QButtonGroup::idClicked, this, &MainWindow::onNavChanged);
    connect(uploadBtn, &QPushButton::clicked, this, &MainWindow::onLoadImages);
    connect(addMore,   &QPushButton::clicked, this, &MainWindow::onLoadImages);
    connect(openAct,   &QAction::triggered,   this, &MainWindow::onLoadImages);
    connect(drop,      &DropZone::browseRequested, this, &MainWindow::onLoadImages);
    connect(clearBtn_, &QPushButton::clicked, store_, &ImageStore::clear);
    connect(store_, &ImageStore::changed, this, &MainWindow::onStoreChanged);
    connect(store_, &ImageStore::currentChanged, this, [this](int i) {
        gallery_->blockSignals(true);
        gallery_->setCurrentRow(i);
        gallery_->blockSignals(false);
    });
    connect(gallery_, &QListWidget::currentRowChanged, this, [this](int r) { store_->setCurrent(r); });

    nav_->button(0)->setChecked(true);
    onNavChanged(0);

    // auto-load everything in the project's images/ folder
    QDir dir(IMAGES_DIR);
    QStringList paths;
    for (const QString& f : dir.entryList(kImageFilters, QDir::Files, QDir::Name))
        paths << dir.absoluteFilePath(f);
    if (!paths.isEmpty()) store_->load(paths);
    else onStoreChanged();
}

void MainWindow::onNavChanged(int i) {
    pages_->setCurrentIndex(i);
    pageTitle_->setText(kPages[i].title);
    pageSub_->setText(kPages[i].subtitle);
}

void MainWindow::onLoadImages() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this, "Select images", IMAGES_DIR, "Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)");
    loadPaths(files);
}

void MainWindow::loadPaths(const QStringList& in) {
    QStringList paths;
    for (const QString& p : in) {
        QFileInfo fi(p);
        if (fi.isDir()) {
            QDir d(p);
            for (const QString& f : d.entryList(kImageFilters, QDir::Files, QDir::Name))
                paths << d.absoluteFilePath(f);
        } else if (fi.isFile()) {
            paths << p;
        }
    }
    if (paths.isEmpty()) return;
    const int n = store_->load(paths);
    if (n == 0) statusBar()->showMessage("No supported images found.", 4000);
    else if (n < paths.size())
        statusBar()->showMessage(QString("Loaded %1 image(s), skipped %2 unsupported file(s).")
                                 .arg(n).arg(paths.size() - n), 5000);
}

void MainWindow::rebuildGallery() {
    gallery_->blockSignals(true);
    gallery_->clear();
    for (const auto& it : store_->items()) {
        QImage thumb = toQImage(it.color).scaled(96, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        auto* li = new QListWidgetItem(QIcon(QPixmap::fromImage(thumb)), it.name);
        li->setToolTip(QString("%1\n%2 x %3").arg(it.path).arg(it.color.width).arg(it.color.height));
        li->setSizeHint(QSize(120, 96));
        gallery_->addItem(li);
    }
    gallery_->setCurrentRow(store_->current());
    gallery_->blockSignals(false);
}

void MainWindow::onStoreChanged() {
    const int n = int(store_->items().size());
    outer_->setCurrentIndex(n == 0 ? 0 : 1);
    clearBtn_->setEnabled(n > 0);
    galleryTitle_->setText(QString("Images (%1)").arg(n));
    rebuildGallery();
    statusBar()->showMessage(n ? QString("%1 image(s) loaded").arg(n)
                               : "No images yet - drop some files into the window");
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* e) {
    QStringList paths;
    for (const QUrl& u : e->mimeData()->urls())
        if (u.isLocalFile()) paths << u.toLocalFile();
    loadPaths(paths);
    e->acceptProposedAction();
}
