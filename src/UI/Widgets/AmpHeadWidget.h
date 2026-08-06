#pragma once

#include <QPixmap>
#include <QWidget>

#include "UI/AmpCatalog.h"

class QHBoxLayout;

// Procedurally painted amp head: tolex-covered box with corner protectors,
// maker logo, model badge, a period-styled control panel, input jack and a
// jewel pilot lamp. The control knobs are ordinary child widgets laid out in
// knobRow(), which is positioned over the painted panel band.

class AmpHeadWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AmpHeadWidget(QWidget* parent = nullptr);

    void setSpec(const ampcat::AmpModelSpec* spec);
    void setLampOn(bool on);

    QHBoxLayout* knobRow() const { return knobRow_; }

    QSize sizeHint() const override { return { 900, 246 }; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void rebuildTolexTile();

    void paintPanel(QPainter& p, const QRectF& panel) const;
    void paintLogo(QPainter& p, const QRectF& band) const;
    void paintCornerProtectors(QPainter& p, const QRectF& body) const;
    void paintJack(QPainter& p, const QPointF& centre) const;
    void paintJewel(QPainter& p, const QPointF& centre) const;

    const ampcat::AmpModelSpec* spec_;
    bool lampOn_ = true;
    QPixmap tolexTile_;
    QHBoxLayout* knobRow_;
};
