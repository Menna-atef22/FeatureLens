#include "SiftTab.h"
#include "UiKit.h"
#include <QVBoxLayout>

SiftTab::SiftTab(ImageStore* store, QWidget* parent) : QWidget(parent), store_(store) {
    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(makeComingSoonPage("SIFT", "Person 3",
        "Build this page like HarrisTab: parameter sliders on the left, linked image views on the right."));
}
