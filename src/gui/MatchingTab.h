#pragma once
// ===== Feature matching tab (SSD + NCC on SIFT descriptors) =====
#include <QWidget>
#include <QImage>
#include <vector>
#include "ImageStore.h"
#include "features/Matcher.h"

class QCheckBox; class QComboBox; class QLabel; class QProgressBar; class QTableWidget;
class ImageView; class ParamSlider; class StatTile;

class MatchingTab : public QWidget {
    Q_OBJECT
public:
    explicit MatchingTab(ImageStore* store, QWidget* parent = nullptr);

private slots:
    void onStoreChanged();
    void onMatchPair();
    void onMatchAll();
    void onSaveResult();

private:
    struct Features {
        bool done = false;
        std::vector<SiftKeypoint> kps;
        double siftMs = 0;
    };
    const Features& features(int idx);                 // computed once per image, then cached
    MatchParams params(MatchMetric m) const;
    QImage drawMatches(int a, int b, const std::vector<Match>& m);

    ImageStore* store_;
    std::vector<Features> feat_;
    QComboBox *aBox_, *bBox_, *metricBox_;
    ParamSlider *ratioS_, *nccS_;
    QCheckBox* mutual_;
    ImageView* view_;
    StatTile *matchTile_, *siftTile_, *timeTile_;
    QTableWidget* table_;
    QProgressBar* progress_;
    QLabel* status_;
    QImage lastResult_;
    QString lastName_;
};
