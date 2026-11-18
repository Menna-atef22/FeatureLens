#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <vector>
#include "common/Image.h"

// Shared set of loaded images + which one is currently selected.
// Every tab reads from here, so selecting an image in the gallery
// strip at the bottom selects it in every tab.
class ImageStore : public QObject {
    Q_OBJECT
public:
    struct Item {
        QString    name;     // file name without extension
        QString    path;
        ColorImage color;
        GrayImage  gray;
    };

    explicit ImageStore(QObject* parent = nullptr) : QObject(parent) {}

    int  load(const QStringList& paths);      // returns number of images loaded
    void clear();
    void remove(int index);
    const std::vector<Item>& items() const { return items_; }

    int  current() const { return current_; }          // -1 when empty
    void setCurrent(int index);

signals:
    void changed();              // the list changed (load / remove / clear); current() is already updated
    void currentChanged(int);    // only the selection changed

private:
    std::vector<Item> items_;
    int current_ = -1;
};
