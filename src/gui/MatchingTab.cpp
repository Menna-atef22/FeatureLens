#include "MatchingTab.h"
#include "ImageView.h"
#include "ImageUtils.h"
#include "UiKit.h"
#include "Paths.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace {
struct BusyCursor {
    BusyCursor()  { QApplication::setOverrideCursor(Qt::WaitCursor); }
    ~BusyCursor() { QApplication::restoreOverrideCursor(); }
};
const MatchParams kDefaults;
}

MatchingTab::MatchingTab(ImageStore* store, QWidget* parent) : QWidget(parent), store_(store) {
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

    auto* pick = new Card("Image pair");
    aBox_ = new QComboBox;
    bBox_ = new QComboBox;
    metricBox_ = new QComboBox;
    metricBox_->addItems({"SSD (sum of squared differences)", "NCC (normalized cross-correlation)"});
    pick->body()->addWidget(new QLabel("Image A"));
    pick->body()->addWidget(aBox_);
    pick->body()->addWidget(new QLabel("Image B"));
    pick->body()->addWidget(bBox_);
    pick->body()->addWidget(new QLabel("Metric"));
    pick->body()->addWidget(metricBox_);

    auto* params = new Card("Matching parameters");
    ratioS_ = new ParamSlider("SSD ratio test", 0.3, 1.0, kDefaults.ratio, 0.05, 2,
                              "Accept if best distance / second-best distance < ratio.");
    nccS_   = new ParamSlider("NCC threshold", 0.3, 1.0, kDefaults.nccThreshold, 0.01, 2,
                              "Accept if the best correlation is at least this value.");
    mutual_ = new QCheckBox("Mutual best match only");
    params->body()->addWidget(ratioS_);
    params->body()->addWidget(nccS_);
    params->body()->addWidget(mutual_);

    auto* actions = new Card("Actions");
    auto* pairBtn = makeButton("Match this pair");
    pairBtn->setShortcut(QKeySequence("Ctrl+Return"));
    auto* allBtn  = makeButton("Match all pairs (SSD + NCC)", "secondary");
    auto* saveBtn = makeButton("Save result image...", "secondary");
    status_ = new QLabel("Add at least two images to match.");
    status_->setObjectName("Muted");
    status_->setWordWrap(true);
    for (QWidget* w : {static_cast<QWidget*>(pairBtn), static_cast<QWidget*>(allBtn),
                       static_cast<QWidget*>(saveBtn), static_cast<QWidget*>(status_)})
        actions->body()->addWidget(w);

    pl->addWidget(actions);
    pl->addWidget(pick);
    pl->addWidget(params);
    pl->addStretch();

    auto* right = new QVBoxLayout;
    right->setSpacing(14);
    auto* stats = new QHBoxLayout;
    stats->setSpacing(14);
    matchTile_ = new StatTile("Matches");
    siftTile_  = new StatTile("SIFT time (A + B)");
    timeTile_  = new StatTile("Matching time");
    stats->addWidget(matchTile_);
    stats->addWidget(siftTile_);
    stats->addWidget(timeTile_);
    right->addLayout(stats);

    view_ = new ImageView("Matches");
    view_->setEmptyText("Pick two images and press Match");
    right->addWidget(view_, 1);

    auto* batch = new Card("All pairs");
    progress_ = new QProgressBar;
    progress_->setTextVisible(false);
    progress_->hide();
    table_ = new QTableWidget(0, 5);
    table_->setHorizontalHeaderLabels({"Pair", "SSD matches", "SSD time (ms)", "NCC matches", "NCC time (ms)"});
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->verticalHeader()->hide();
    table_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table_->setShowGrid(false);
    table_->setAlternatingRowColors(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setFixedHeight(150);
    batch->body()->addWidget(progress_);
    batch->body()->addWidget(table_);
    right->addWidget(batch);

    root->addWidget(scroll);
    root->addLayout(right, 1);

    connect(pairBtn, &QPushButton::clicked, this, &MatchingTab::onMatchPair);
    connect(allBtn,  &QPushButton::clicked, this, &MatchingTab::onMatchAll);
    connect(saveBtn, &QPushButton::clicked, this, &MatchingTab::onSaveResult);
    connect(store_, &ImageStore::changed, this, &MatchingTab::onStoreChanged);
    connect(store_, &ImageStore::currentChanged, this, [this](int i) {
        if (i >= 0 && i < aBox_->count()) aBox_->setCurrentIndex(i);
    });

    onStoreChanged();
}

void MatchingTab::onStoreChanged() {
    const auto& items = store_->items();
    feat_.assign(items.size(), Features());
    table_->setRowCount(0);
    lastResult_ = QImage();
    view_->clearImage();
    matchTile_->setValue("-"); siftTile_->setValue("-"); timeTile_->setValue("-");

    aBox_->clear(); bBox_->clear();
    for (const auto& it : items) { aBox_->addItem(it.name); bBox_->addItem(it.name); }
    const int n = int(items.size());
    if (n > 0) aBox_->setCurrentIndex(std::max(0, store_->current()));
    if (n > 1) bBox_->setCurrentIndex((aBox_->currentIndex() + 1) % n);
    status_->setText(n >= 2 ? QString("%1 images loaded.").arg(n) : "Add at least two images to match.");
}

const MatchingTab::Features& MatchingTab::features(int idx) {
    Features& f = feat_[idx];
    if (!f.done) {
        double det = 0, desc = 0;
        f.kps = SiftDetector::run(store_->items()[idx].gray, SiftParams(), det, desc);
        f.siftMs = det + desc;
        f.done = true;
    }
    return f;
}

MatchParams MatchingTab::params(MatchMetric m) const {
    MatchParams p;
    p.metric = m;
    p.ratio = float(ratioS_->value());
    p.nccThreshold = float(nccS_->value());
    p.mutual = mutual_->isChecked();
    return p;
}

QImage MatchingTab::drawMatches(int a, int b, const std::vector<Match>& m) {
    const auto& A = store_->items()[a].color;
    const auto& B = store_->items()[b].color;
    const int W = A.width + B.width, H = std::max(A.height, B.height);
    QImage out(W, H, QImage::Format_RGB32);
    out.fill(QColor(20, 20, 30));
    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawImage(0, 0, toQImage(A));
    p.drawImage(A.width, 0, toQImage(B));

    const double t = std::max(1.0, std::max(W, H) / 700.0);     // scale line widths with image size
    const auto& fa = feat_[a].kps;
    const auto& fb = feat_[b].kps;
    for (size_t i = 0; i < m.size(); ++i) {
        const auto& ka = fa[m[i].a];
        const auto& kb = fb[m[i].b];
        QColor c = QColor::fromHsv(int((i * 47) % 360), 230, 255);
        QPointF pa(ka.x + 0.5, ka.y + 0.5), pb(kb.x + A.width + 0.5, kb.y + 0.5);
        p.setPen(QPen(c, 1.5 * t));
        p.drawLine(pa, pb);
        p.setBrush(c);
        p.setPen(QPen(QColor(10, 10, 30), 1.0 * t));
        p.drawEllipse(pa, 3.5 * t, 3.5 * t);
        p.drawEllipse(pb, 3.5 * t, 3.5 * t);
    }
    return out;
}

void MatchingTab::onMatchPair() {
    const int n = int(store_->items().size());
    const int a = aBox_->currentIndex(), b = bBox_->currentIndex();
    if (n < 2) { status_->setText("Add at least two images first."); return; }
    if (a < 0 || b < 0 || a == b) { status_->setText("Choose two different images."); return; }

    BusyCursor busy;
    const MatchMetric metric = metricBox_->currentIndex() == 0 ? MatchMetric::SSD : MatchMetric::NCC;
    const auto& fa = features(a);
    const auto& fb = features(b);
    double ms = 0;
    auto matches = Matcher::match(fa.kps, fb.kps, params(metric), ms);       // timed part

    lastResult_ = drawMatches(a, b, matches);
    lastName_ = QString("%1_%2_%3").arg(metric == MatchMetric::SSD ? "ssd" : "ncc",
                                        store_->items()[a].name, store_->items()[b].name);
    view_->setImage(lastResult_);
    view_->setBadge(QString("%1 matches").arg(matches.size()));
    matchTile_->setValue(QString::number(matches.size()));
    siftTile_->setValue(QString("%1 ms").arg(fa.siftMs + fb.siftMs, 0, 'f', 1));
    timeTile_->setValue(QString("%1 ms").arg(ms, 0, 'f', 2));
    status_->setText(QString("%1 vs %2: %3 keypoints / %4 keypoints, %5 matches (%6) in %7 ms")
                     .arg(store_->items()[a].name, store_->items()[b].name)
                     .arg(fa.kps.size()).arg(fb.kps.size()).arg(matches.size())
                     .arg(metric == MatchMetric::SSD ? "SSD" : "NCC").arg(ms, 0, 'f', 2));
}

void MatchingTab::onMatchAll() {
    const int N = int(store_->items().size());
    if (N < 2) { status_->setText("Add at least two images first."); return; }

    BusyCursor busy;
    const int pairs = N * (N - 1) / 2;
    progress_->setRange(0, pairs);
    progress_->setValue(0);
    progress_->show();
    table_->setRowCount(0);
    int done = 0;
    double totalSsd = 0, totalNcc = 0;
    for (int i = 0; i < N; ++i) {
        for (int j = i + 1; j < N; ++j) {
            const auto& fa = features(i);
            const auto& fb = features(j);
            double msS = 0, msN = 0;
            auto mS = Matcher::match(fa.kps, fb.kps, params(MatchMetric::SSD), msS);
            auto mN = Matcher::match(fa.kps, fb.kps, params(MatchMetric::NCC), msN);
            totalSsd += msS; totalNcc += msN;
            const int r = table_->rowCount();
            table_->insertRow(r);
            table_->setItem(r, 0, new QTableWidgetItem(store_->items()[i].name + "  \u2194  " + store_->items()[j].name));
            table_->setItem(r, 1, new QTableWidgetItem(QString::number(mS.size())));
            table_->setItem(r, 2, new QTableWidgetItem(QString::number(msS, 'f', 2)));
            table_->setItem(r, 3, new QTableWidgetItem(QString::number(mN.size())));
            table_->setItem(r, 4, new QTableWidgetItem(QString::number(msN, 'f', 2)));
            progress_->setValue(++done);
            status_->setText(QString("Matching... %1 / %2 pairs").arg(done).arg(pairs));
            QApplication::processEvents();
        }
    }
    progress_->hide();
    status_->setText(QString("Done: %1 pairs. Total matching time: SSD %2 ms, NCC %3 ms.")
                     .arg(pairs).arg(totalSsd, 0, 'f', 1).arg(totalNcc, 0, 'f', 1));
}

void MatchingTab::onSaveResult() {
    if (lastResult_.isNull()) {
        QMessageBox::information(this, "Matching", "Match a pair first, then save the result.");
        return;
    }
    QDir().mkpath(RESULTS_DIR);
    const QString def = QString("%1/match_%2.png").arg(RESULTS_DIR, lastName_);
    const QString path = QFileDialog::getSaveFileName(this, "Save result image", def, "PNG image (*.png)");
    if (path.isEmpty()) return;
    status_->setText(lastResult_.save(path) ? "Saved: " + path : "Could not save: " + path);
}
