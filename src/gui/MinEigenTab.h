#pragma once
// ===== Person 2 : Min Eigenvalue (lambda-) tab =====
// TODO (Person 2): build this tab's GUI here (same idea as HarrisTab).
#include <QWidget>
#include "ImageStore.h"

class MinEigenTab : public QWidget {
    Q_OBJECT
public:
    explicit MinEigenTab(ImageStore* store, QWidget* parent = nullptr);
private:
    ImageStore* store_;
};
