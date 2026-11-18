#include "ImageStore.h"
#include "ImageUtils.h"
#include <QFileInfo>
#include <QImage>
#include <algorithm>

int ImageStore::load(const QStringList& paths) {
    const int first = int(items_.size());
    int n = 0;
    for (const QString& p : paths) {
        QImage q(p);
        if (q.isNull()) continue;
        Item it;
        it.path  = p;
        it.name  = QFileInfo(p).completeBaseName();
        it.color = fromQImage(q);
        it.gray  = toGray(it.color);
        items_.push_back(std::move(it));
        ++n;
    }
    if (n) {
        current_ = first;            // jump to the first newly added image
        emit changed();
    }
    return n;
}

void ImageStore::clear() {
    items_.clear();
    current_ = -1;
    emit changed();
}

void ImageStore::remove(int index) {
    if (index < 0 || index >= int(items_.size())) return;
    items_.erase(items_.begin() + index);
    if (items_.empty()) current_ = -1;
    else current_ = std::min(index, int(items_.size()) - 1);
    emit changed();
}

void ImageStore::setCurrent(int index) {
    if (index < 0 || index >= int(items_.size()) || index == current_) return;
    current_ = index;
    emit currentChanged(current_);
}
