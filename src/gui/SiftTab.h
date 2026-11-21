#pragma once
// ===== SIFT tab: keypoints + descriptors, with computation times =====
#include <QWidget>
#include <QImage>
#include <vector>
#include "ImageStore.h"
#include "features/SiftDetector.h"

class QLabel; class QProgressBar; class QTableWidget; class QCheckBox;
class ImageView; class ParamSlider; class StatTile;

class SiftTab : public QWidget {
    Q_OBJECT
public:
    explicit SiftTab(ImageStore* store, QWidget* parent = nullptr);

private slots:
    void onStoreChanged();
    void onCurrentChanged();
    void onParamChanged();
    void onRunSelected();
    void onRunAll();
    void onSaveResult();
    void onReset();

private:
    SiftParams params() const;
    void runOne(int idx, QImage& outImg, int& count, double& detMs, double& descMs);
    void showImage(int idx);
    void setRow(int row, const QString& kp, const QString& det, const QString& desc, const QString& total);
    void resetTable();

    ImageStore* store_;
    ParamSlider *octS_, *scaleS_, *sigmaS_, *contrastS_, *edgeS_, *maxS_;
    ImageView *origView_, *resView_;
    StatTile *kpTile_, *detTile_, *descTile_;
    QTableWidget* table_;
    QProgressBar* progress_;
    QLabel* status_;
    std::vector<QImage> cache_;
};
