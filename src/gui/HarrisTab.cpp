#include "HarrisTab.h"
#include "ImageView.h"
#include "ImageUtils.h"
#include "UiKit.h"
#include "Paths.h"
#include "common/Draw.h"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

namespace {
struct BusyCursor {
    BusyCursor()  { QApplication::setOverrideCursor(Qt::WaitCursor); }
    ~BusyCursor() { QApplication::restoreOverrideCursor(); }
};
const HarrisParams kDefaults;   // default values come straight from HarrisParams
}

HarrisTab::HarrisTab(ImageStore* store, QWidget* parent) : QWidget(parent), store_(store) {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(18);

    // =============== left: parameters (scrolls if the window is small) ===============
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFixedWidth(334);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* panel = new QWidget;
    panel->setObjectName("Transparent");
    auto* pl = new QVBoxLayout(panel);
    pl->setContentsMargins(0, 0, 8, 0);
    pl->setSpacing(14);
    scroll->setWidget(panel);

    auto* params = new Card("Parameters");
    kS_      = new ParamSlider("k (Harris constant)", 0.01, 0.20, kDefaults.k, 0.01, 2,
                               "Typical range 0.04 - 0.06.");
    thrS_    = new ParamSlider("Threshold (x max R)", 0.0001, 1.0, kDefaults.thresholdRatio, 0.0001, 4,
                               "Keep corners whose response R is above threshold x max R.");
    thrS_->setLogScale(true);
    radiusS_ = new ParamSlider("Window radius", 1, 10, kDefaults.gaussRadius, 1, 0,
                               "Size of the Gaussian window around each pixel.");
    sigmaS_  = new ParamSlider("Window sigma", 0.3, 5.0, kDefaults.sigma, 0.1, 1,
                               "Blur strength of that window.");
    nmsS_    = new ParamSlider("NMS radius", 1, 20, kDefaults.nmsRadius, 1, 0,
                               "Minimum distance between two detected corners.");
    for (auto* s : {kS_, thrS_, radiusS_, sigmaS_, nmsS_}) params->body()->addWidget(s);
    live_ = new QCheckBox("Live preview (re-run while you tweak)");
    live_->setChecked(true);
    auto* resetBtn = makeButton("Reset to defaults", "ghost");
    params->body()->addWidget(live_);
    params->body()->addWidget(resetBtn, 0, Qt::AlignLeft);

    auto* actions = new Card("Actions");
    auto* runBtn  = makeButton("Run on this image");
    runBtn->setShortcut(QKeySequence("Ctrl+Return"));
    runBtn->setToolTip("Ctrl+Enter");
    auto* allBtn  = makeButton("Run on all images", "secondary");
    auto* saveBtn = makeButton("Save result image...", "secondary");
    status_ = new QLabel("Add some images to get started.");
    status_->setObjectName("Muted");
    status_->setWordWrap(true);
    for (QWidget* w : {static_cast<QWidget*>(runBtn), static_cast<QWidget*>(allBtn),
                       static_cast<QWidget*>(saveBtn), static_cast<QWidget*>(status_)})
        actions->body()->addWidget(w);

    pl->addWidget(actions);      // Run buttons first so they are always visible
    pl->addWidget(params);
    pl->addStretch();

    // =============== right: stats + views + table ===============
    auto* right = new QVBoxLayout;
    right->setSpacing(14);

    auto* stats = new QHBoxLayout;
    stats->setSpacing(14);
    cornersTile_ = new StatTile("Corners found");
    timeTile_    = new StatTile("Detection time");
    sizeTile_    = new StatTile("Image size");
    stats->addWidget(cornersTile_);
    stats->addWidget(timeTile_);
    stats->addWidget(sizeTile_);
    right->addLayout(stats);

    origView_ = new ImageView("Original");
    resView_  = new ImageView("Harris result");
    origView_->setEmptyText("Select an image");
    resView_->setEmptyText("Run Harris to see the corners");
    origView_->linkWith(resView_);                    // zoom/pan together
    auto* split = new QSplitter(Qt::Horizontal);
    split->setChildrenCollapsible(false);
    split->setHandleWidth(14);
    split->addWidget(origView_);
    split->addWidget(resView_);
    right->addWidget(split, 1);

    auto* batch = new Card("All images");
    progress_ = new QProgressBar;
    progress_->setTextVisible(false);
    progress_->hide();
    table_ = new QTableWidget(0, 3);
    table_->setHorizontalHeaderLabels({"Image", "Corners", "Time (ms)"});
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->verticalHeader()->hide();
    table_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table_->setShowGrid(false);
    table_->setAlternatingRowColors(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setFixedHeight(150);
    batch->body()->addWidget(progress_);
    batch->body()->addWidget(table_);
    right->addWidget(batch);

    root->addWidget(scroll);
    root->addLayout(right, 1);

    // =============== wiring ===============
    debounce_ = new QTimer(this);
    debounce_->setSingleShot(true);
    debounce_->setInterval(250);
    connect(debounce_, &QTimer::timeout, this, &HarrisTab::onRunSelected);

    for (auto* s : {kS_, thrS_, radiusS_, sigmaS_, nmsS_})
        connect(s, &ParamSlider::valueChanged, this, &HarrisTab::onParamChanged);
    connect(resetBtn, &QPushButton::clicked, this, &HarrisTab::onReset);
    connect(runBtn,   &QPushButton::clicked, this, &HarrisTab::onRunSelected);
    connect(allBtn,   &QPushButton::clicked, this, &HarrisTab::onRunAll);
    connect(saveBtn,  &QPushButton::clicked, this, &HarrisTab::onSaveResult);
    connect(store_, &ImageStore::changed,        this, &HarrisTab::onStoreChanged);
    connect(store_, &ImageStore::currentChanged, this, &HarrisTab::onCurrentChanged);
    connect(table_, &QTableWidget::cellClicked, this, [this](int r, int) { store_->setCurrent(r); });

    onStoreChanged();
}

HarrisParams HarrisTab::params() const {
    HarrisParams p;
    p.k = float(kS_->value());
    p.thresholdRatio = float(thrS_->value());
    p.gaussRadius = int(radiusS_->value() + 0.5);
    p.sigma = float(sigmaS_->value());
    p.nmsRadius = int(nmsS_->value() + 0.5);
    return p;
}

// ---------------------------------------------------------------- table helpers
void HarrisTab::setRow(int row, const QString& corners, const QString& ms) {
    if (row < 0 || row >= table_->rowCount()) return;
    table_->item(row, 1)->setText(corners);
    table_->item(row, 2)->setText(ms);
}

void HarrisTab::resetTable() {
    const auto& items = store_->items();
    table_->setRowCount(int(items.size()));
    for (int i = 0; i < int(items.size()); ++i) {
        table_->setItem(i, 0, new QTableWidgetItem(items[i].name));
        table_->setItem(i, 1, new QTableWidgetItem("-"));
        table_->setItem(i, 2, new QTableWidgetItem("-"));
    }
    if (store_->current() >= 0) table_->selectRow(store_->current());
}

// ---------------------------------------------------------------- store / selection
void HarrisTab::onStoreChanged() {
    const int n = int(store_->items().size());
    cache_.assign(n, QImage());
    resetTable();
    status_->setText(n ? QString("%1 image(s) loaded.").arg(n) : "Add some images to get started.");
    onCurrentChanged();
}

void HarrisTab::onCurrentChanged() {
    const int idx = store_->current();
    if (idx >= 0 && idx < table_->rowCount()) {
        table_->blockSignals(true);
        table_->selectRow(idx);
        table_->blockSignals(false);
    }
    showImage(idx);
    if (idx >= 0 && cache_[idx].isNull() && live_->isChecked()) onRunSelected();
}

void HarrisTab::showImage(int idx) {
    if (idx < 0 || idx >= int(store_->items().size())) {
        origView_->clearImage();
        resView_->clearImage();
        cornersTile_->setValue("-");
        timeTile_->setValue("-");
        sizeTile_->setValue("-");
        return;
    }
    const auto& it = store_->items()[idx];
    origView_->setImage(toQImage(it.color));
    sizeTile_->setValue(QString("%1 x %2").arg(it.color.width).arg(it.color.height));
    if (!cache_[idx].isNull()) {
        resView_->setImage(cache_[idx]);
        resView_->setBadge("Harris result");
    } else {
        resView_->clearImage();
        cornersTile_->setValue("-");
        timeTile_->setValue("-");
    }
}

void HarrisTab::onParamChanged() {
    cache_.assign(cache_.size(), QImage());      // every old result is now outdated
    resetTable();
    if (live_->isChecked()) {
        debounce_->start();                       // wait until the slider stops moving
    } else {
        if (resView_->hasImage()) resView_->setBadge("Harris result (outdated - press Run)");
        status_->setText("Parameters changed - press Run.");
    }
}

void HarrisTab::onReset() {
    kS_->setValue(kDefaults.k);
    thrS_->setValue(kDefaults.thresholdRatio);
    radiusS_->setValue(kDefaults.gaussRadius);
    sigmaS_->setValue(kDefaults.sigma);
    nmsS_->setValue(kDefaults.nmsRadius);
    onParamChanged();
}

// ---------------------------------------------------------------- detection
void HarrisTab::runOne(int idx, QImage& outImg, int& count, double& ms) {
    const auto& item = store_->items()[idx];
    auto corners = HarrisDetector::detect(item.gray, params(), ms);   // timed part

    // clear ring + center dot on every corner (size scales with the image)
    outImg = toQImage(item.color);
    drawCornerMarkers(outImg, corners);
    count = int(corners.size());
}

void HarrisTab::onRunSelected() {
    debounce_->stop();
    const int idx = store_->current();
    if (idx < 0) { status_->setText("Add some images first."); return; }

    BusyCursor busy;
    QImage out; int n; double ms;
    runOne(idx, out, n, ms);
    cache_[idx] = out;
    resView_->setImage(out);
    resView_->setBadge("Harris result");
    cornersTile_->setValue(QString::number(n));
    timeTile_->setValue(QString("%1 ms").arg(ms, 0, 'f', 1));
    setRow(idx, QString::number(n), QString::number(ms, 'f', 2));
    status_->setText(QString("%1: %2 corners in %3 ms").arg(store_->items()[idx].name).arg(n).arg(ms, 0, 'f', 2));
}

void HarrisTab::onRunAll() {
    const int N = int(store_->items().size());
    if (N == 0) { status_->setText("Add some images first."); return; }

    BusyCursor busy;
    progress_->setRange(0, N);
    progress_->setValue(0);
    progress_->show();
    double total = 0;
    for (int i = 0; i < N; ++i) {
        QImage out; int n; double ms;
        runOne(i, out, n, ms);
        cache_[i] = out;
        total += ms;
        setRow(i, QString::number(n), QString::number(ms, 'f', 2));
        progress_->setValue(i + 1);
        status_->setText(QString("Running... %1 / %2").arg(i + 1).arg(N));
        QApplication::processEvents();            // keep the UI alive
    }
    progress_->hide();
    status_->setText(QString("Done: %1 images, total %2 ms (avg %3 ms).")
                     .arg(N).arg(total, 0, 'f', 1).arg(total / N, 0, 'f', 1));
    showImage(store_->current());
    const int idx = store_->current();
    if (idx >= 0 && idx < table_->rowCount()) {
        cornersTile_->setValue(table_->item(idx, 1)->text());
        timeTile_->setValue(table_->item(idx, 2)->text() + " ms");
    }
}

void HarrisTab::onSaveResult() {
    const int idx = store_->current();
    if (idx < 0 || cache_[idx].isNull()) {
        QMessageBox::information(this, "Harris", "Run Harris first, then save the result.");
        return;
    }
    QDir().mkpath(RESULTS_DIR);
    const QString def = QString("%1/harris_%2.png").arg(RESULTS_DIR, store_->items()[idx].name);
    const QString path = QFileDialog::getSaveFileName(this, "Save result image", def, "PNG image (*.png)");
    if (path.isEmpty()) return;
    if (cache_[idx].save(path)) status_->setText("Saved: " + path);
    else status_->setText("Could not save: " + path);
}
