#pragma once
// ===== Person 3 : SIFT tab =====
// TODO (Person 3): build this tab's GUI here (same idea as HarrisTab).
#include <QWidget>
#include "ImageStore.h"

class SiftTab : public QWidget {
    Q_OBJECT
public:
    explicit SiftTab(ImageStore* store, QWidget* parent = nullptr);
private:
    ImageStore* store_;
};
