#include "MinEigenTab.h"
#include "UiKit.h"
#include <QVBoxLayout>

MinEigenTab::MinEigenTab(ImageStore* store, QWidget* parent) : QWidget(parent), store_(store) {
    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(makeComingSoonPage("Min Eigenvalue (\u03BB-)", "Person 2",
        "Build this page like HarrisTab: parameter sliders on the left, linked image views on the right."));
}
