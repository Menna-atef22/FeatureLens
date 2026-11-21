#pragma once
// ===== Min Eigenvalue (lambda-) tab =====
// This file is also the TEMPLATE for the other tabs: copy its structure
// (left = parameter cards, right = stats + linked image views + results table).
#include <QWidget>
#include <QImage>
#include <vector>
#include "ImageStore.h"
#include "features/MinEigenDetector.h"

class QCheckBox; class QLabel; class QProgressBar; class QTableWidget; class QTimer;
class ImageView; class ParamSlider; class StatTile;

class MinEigenTab : public QWidget {
    Q_OBJECT
public:
    explicit MinEigenTab(ImageStore* store, QWidget* parent = nullptr);

private slots:
    void onStoreChanged();
    void onCurrentChanged();
    void onParamChanged();
    void onRunSelected();
    void onRunAll();
    void onSaveResult();
    void onReset();

private:
    MinEigenParams params() const;
    // Runs the detector on image idx. Fills outImg (with corners drawn), count, ms.
    void runOne(int idx, QImage& outImg, int& count, double& ms);
    void showImage(int idx);
    void setRow(int row, const QString& corners, const QString& ms);
    void resetTable();

    ImageStore* store_;
    ParamSlider *thrS_, *radiusS_, *sigmaS_, *nmsS_;
    QCheckBox* live_;
    ImageView *origView_, *resView_;
    StatTile *cornersTile_, *timeTile_, *sizeTile_;
    QTableWidget* table_;
    QProgressBar* progress_;
    QLabel* status_;
    QTimer* debounce_;
    std::vector<QImage> cache_;       // result image per input image
};
