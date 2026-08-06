#pragma once

#include <QPushButton>
#include <QWidget>

#include "UI/PedalCatalog.h"

// Skeuomorphic guitar pedal chassis. The widget paints the enclosure (colored
// metal box, corner screws, name plate, status LED); the type combo, knobs
// and the footswitch are ordinary child widgets. Toggling the footswitch is
// the pedal's bypass; the LED tracks it. Cosmetics come from PedalCatalog.

// Round chrome footswitch, checkable (checked = effect engaged).
class FootswitchButton : public QPushButton
{
    Q_OBJECT

public:
    explicit FootswitchButton(QWidget* parent = nullptr);

    QSize sizeHint() const override { return { 44, 44 }; }

protected:
    void paintEvent(QPaintEvent* event) override;
};

class StompboxWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StompboxWidget(QWidget* parent = nullptr);

    void setPedalType(int type);
    void setEngaged(bool on);       // drives the LED

    FootswitchButton* footswitch() const { return footswitch_; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    const pedalcat::PedalStyle* style_;
    bool engaged_ = false;
    FootswitchButton* footswitch_;
};
