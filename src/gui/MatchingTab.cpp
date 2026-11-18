#include "MatchingTab.h"
#include "UiKit.h"
#include <QVBoxLayout>

MatchingTab::MatchingTab(ImageStore* store, QWidget* parent) : QWidget(parent), store_(store) {
    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(makeComingSoonPage("Feature Matching (SSD / NCC)", "Person 4",
        "Pick two images from the gallery, run SSD / NCC matching and draw the matches between them."));
}
