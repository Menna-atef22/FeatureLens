#include "SiftTab.h"
#include "ImageView.h"
#include "ImageUtils.h"
#include "UiKit.h"
#include "Paths.h"

#include <QApplication>
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
#include <QVBoxLayout>

namespace {
struct BusyCursor {
    BusyCursor()  { QApplication::setOverrideCursor(Qt::WaitCursor); }
    ~BusyCursor() { QApplication::restoreOverrideCursor(); }
};
const SiftParams kDefaults;
}

SiftTab::SiftTab(ImageStore* store, QWidget* parent) : QWidget(parent), store_(store) {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(18);

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
    octS_      = new ParamSlider("Octaves", 1, 6, kDefaults.octaves, 1, 0, "Number of image-pyramid octaves.");
    scaleS_    = new ParamSlider("Scales per octave", 2, 5, kDefaults.scalesPerOctave, 1, 0, "DoG levels searched per octave.");
    sigmaS_    = new ParamSlider("Base sigma", 0.8, 3.0, kDefaults.sigma0, 0.1, 1, "Initial Gaussian blur.");
    contrastS_ = new ParamSlider("Contrast threshold", 0.005, 0.15, kDefaults.contrastThresh, 0.005, 3,
                                 "Reject weak (low-contrast) keypoints.");
    edgeS_     = new ParamSlider("Edge ratio r", 3, 20, kDefaults.edgeRatio, 1, 0, "Reject keypoints lying on edges.");
    maxS_      = new ParamSlider("Max keypoints", 50, 5000, kDefaults.maxKeypoints, 50, 0, "Keep only the strongest N.");
    for (auto* s : {octS_, scaleS_, sigmaS_, contrastS_, edgeS_, maxS_}) params->body()->addWidget(s);
    auto* resetBtn = makeButton("Reset to defaults", "ghost");
    params->body()->addWidget(resetBtn, 0, Qt::AlignLeft);

    auto* actions = new Card("Actions");
    auto* runBtn  = makeButton("Run on this image");
    runBtn->setShortcut(QKeySequence("Ctrl+Return"));
    auto* allBtn  = makeButton("Run on all images", "secondary");
    auto* saveBtn = makeButton("Save result image...", "secondary");
    status_ = new QLabel("Add some images to get started.");
    status_->setObjectName("Muted");
    status_->setWordWrap(true);
    for (QWidget* w : {static_cast<QWidget*>(runBtn), static_cast<QWidget*>(allBtn),
                       static_cast<QWidget*>(saveBtn), static_cast<QWidget*>(status_)})
        actions->body()->addWidget(w);
    pl->addWidget(actions);
    pl->addWidget(params);
    pl->addStretch();

    auto* right = new QVBoxLayout;
    right->setSpacing(14);
    auto* stats = new QHBoxLayout;
    stats->setSpacing(14);
    kpTile_   = new StatTile("Keypoints");
    detTile_  = new StatTile("Detection time");
    descTile_ = new StatTile("Descriptor time");
    stats->addWidget(kpTile_);
    stats->addWidget(detTile_);
    stats->addWidget(descTile_);
    right->addLayout(stats);

    origView_ = new ImageView("Original");
    resView_  = new ImageView("SIFT keypoints");
    origView_->setEmptyText("Select an image");
    resView_->setEmptyText("Run SIFT to see the keypoints");
    origView_->linkWith(resView_);
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
    table_ = new QTableWidget(0, 5);
    table_->setHorizontalHeaderLabels({"Image", "Keypoints", "Detect (ms)", "Descriptors (ms)", "Total (ms)"});
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

    // SIFT is heavier than Harris: no live preview, run on demand.
    for (auto* s : {octS_, scaleS_, sigmaS_, contrastS_, edgeS_, maxS_})
        connect(s, &ParamSlider::valueChanged, this, &SiftTab::onParamChanged);
    connect(resetBtn, &QPushButton::clicked, this, &SiftTab::onReset);
    connect(runBtn,   &QPushButton::clicked, this, &SiftTab::onRunSelected);
    connect(allBtn,   &QPushButton::clicked, this, &SiftTab::onRunAll);
    connect(saveBtn,  &QPushButton::clicked, this, &SiftTab::onSaveResult);
    connect(store_, &ImageStore::changed,        this, &SiftTab::onStoreChanged);
    connect(store_, &ImageStore::currentChanged, this, &SiftTab::onCurrentChanged);
    connect(table_, &QTableWidget::cellClicked, this, [this](int r, int) { store_->setCurrent(r); });

    onStoreChanged();
}

SiftParams SiftTab::params() const {
    SiftParams p;
    p.octaves         = int(octS_->value() + 0.5);
    p.scalesPerOctave = int(scaleS_->value() + 0.5);
    p.sigma0          = float(sigmaS_->value());
    p.contrastThresh  = float(contrastS_->value());
    p.edgeRatio       = float(edgeS_->value());
    p.maxKeypoints    = int(maxS_->value() + 0.5);
    return p;
}

void SiftTab::setRow(int row, const QString& kp, const QString& det, const QString& desc, const QString& total) {
    if (row < 0 || row >= table_->rowCount()) return;
    table_->item(row, 1)->setText(kp);
    table_->item(row, 2)->setText(det);
    table_->item(row, 3)->setText(desc);
    table_->item(row, 4)->setText(total);
}

void SiftTab::resetTable() {
    const auto& items = store_->items();
    table_->setRowCount(int(items.size()));
    for (int i = 0; i < int(items.size()); ++i) {
        table_->setItem(i, 0, new QTableWidgetItem(items[i].name));
        for (int c = 1; c < 5; ++c) table_->setItem(i, c, new QTableWidgetItem("-"));
    }
    if (store_->current() >= 0) table_->selectRow(store_->current());
}

void SiftTab::onStoreChanged() {
    const int n = int(store_->items().size());
    cache_.assign(n, QImage());
    resetTable();
    status_->setText(n ? QString("%1 image(s) loaded.").arg(n) : "Add some images to get started.");
    onCurrentChanged();
}

void SiftTab::onCurrentChanged() {
    const int idx = store_->current();
    if (idx >= 0 && idx < table_->rowCount()) {
        table_->blockSignals(true);
        table_->selectRow(idx);
        table_->blockSignals(false);
    }
    showImage(idx);
}

void SiftTab::showImage(int idx) {
    if (idx < 0 || idx >= int(store_->items().size())) {
        origView_->clearImage();
        resView_->clearImage();
        kpTile_->setValue("-"); detTile_->setValue("-"); descTile_->setValue("-");
        return;
    }
    origView_->setImage(toQImage(store_->items()[idx].color));
    if (!cache_[idx].isNull()) {
        resView_->setImage(cache_[idx]);
        resView_->setBadge("SIFT keypoints");
        kpTile_->setValue(table_->item(idx, 1)->text());
        detTile_->setValue(table_->item(idx, 2)->text() + " ms");
        descTile_->setValue(table_->item(idx, 3)->text() + " ms");
    } else {
        resView_->clearImage();
        kpTile_->setValue("-"); detTile_->setValue("-"); descTile_->setValue("-");
    }
}

void SiftTab::onParamChanged() {
    cache_.assign(cache_.size(), QImage());
    resetTable();
    if (resView_->hasImage()) resView_->setBadge("SIFT keypoints (outdated - press Run)");
    status_->setText("Parameters changed - press Run.");
}

void SiftTab::onReset() {
    octS_->setValue(kDefaults.octaves);
    scaleS_->setValue(kDefaults.scalesPerOctave);
    sigmaS_->setValue(kDefaults.sigma0);
    contrastS_->setValue(kDefaults.contrastThresh);
    edgeS_->setValue(kDefaults.edgeRatio);
    maxS_->setValue(kDefaults.maxKeypoints);
    onParamChanged();
}

void SiftTab::runOne(int idx, QImage& outImg, int& count, double& detMs, double& descMs) {
    const auto& item = store_->items()[idx];
    auto kps = SiftDetector::run(item.gray, params(), detMs, descMs);   // timed part
    std::vector<Corner> pts;
    pts.reserve(kps.size());
    for (const auto& k : kps) pts.push_back({int(k.x + 0.5f), int(k.y + 0.5f), k.response});
    outImg = toQImage(item.color);
    drawCornerMarkers(outImg, pts);
    count = int(kps.size());
}

void SiftTab::onRunSelected() {
    const int idx = store_->current();
    if (idx < 0) { status_->setText("Add some images first."); return; }
    BusyCursor busy;
    QImage out; int n; double det, desc;
    runOne(idx, out, n, det, desc);
    cache_[idx] = out;
    setRow(idx, QString::number(n), QString::number(det, 'f', 2), QString::number(desc, 'f', 2),
           QString::number(det + desc, 'f', 2));
    showImage(idx);
    status_->setText(QString("%1: %2 keypoints, detect %3 ms + descriptors %4 ms")
                     .arg(store_->items()[idx].name).arg(n).arg(det, 0, 'f', 1).arg(desc, 0, 'f', 1));
}

void SiftTab::onRunAll() {
    const int N = int(store_->items().size());
    if (N == 0) { status_->setText("Add some images first."); return; }
    BusyCursor busy;
    progress_->setRange(0, N);
    progress_->setValue(0);
    progress_->show();
    double total = 0;
    for (int i = 0; i < N; ++i) {
        QImage out; int n; double det, desc;
        runOne(i, out, n, det, desc);
        cache_[i] = out;
        total += det + desc;
        setRow(i, QString::number(n), QString::number(det, 'f', 2), QString::number(desc, 'f', 2),
               QString::number(det + desc, 'f', 2));
        progress_->setValue(i + 1);
        status_->setText(QString("Running... %1 / %2").arg(i + 1).arg(N));
        QApplication::processEvents();
    }
    progress_->hide();
    status_->setText(QString("Done: %1 images, total %2 ms (avg %3 ms).")
                     .arg(N).arg(total, 0, 'f', 1).arg(total / N, 0, 'f', 1));
    showImage(store_->current());
}

void SiftTab::onSaveResult() {
    const int idx = store_->current();
    if (idx < 0 || cache_[idx].isNull()) {
        QMessageBox::information(this, "SIFT", "Run SIFT first, then save the result.");
        return;
    }
    QDir().mkpath(RESULTS_DIR);
    const QString def = QString("%1/sift_%2.png").arg(RESULTS_DIR, store_->items()[idx].name);
    const QString path = QFileDialog::getSaveFileName(this, "Save result image", def, "PNG image (*.png)");
    if (path.isEmpty()) return;
    status_->setText(cache_[idx].save(path) ? "Saved: " + path : "Could not save: " + path);
}
