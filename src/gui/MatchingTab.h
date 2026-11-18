#pragma once
// ===== Person 4 : Feature Matching tab =====
// TODO (Person 4): build this tab's GUI here (same idea as HarrisTab).
#include <QWidget>
#include "ImageStore.h"

class MatchingTab : public QWidget {
    Q_OBJECT
public:
    explicit MatchingTab(ImageStore* store, QWidget* parent = nullptr);
private:
    ImageStore* store_;
};
