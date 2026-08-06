#pragma once

#include <QColor>
#include <QWidget>

#include "UI/Widgets/KnobStyle.h"

// Rotary knob with several skeuomorphic hardware looks. The default Studio
// style is the dark/orange arc knob used on the rack sections; the amp
// faceplates and stompboxes pick period-correct hardware instead.
// Drag vertically (or scroll) to change, double-click to reset to default.

class CustomKnob : public QWidget
{
    Q_OBJECT

public:
    CustomKnob(const QString& label, float minValue, float maxValue,
               float defaultValue, const QString& unit, QWidget* parent = nullptr);

    float value() const { return value_; }
    void setValue(float v);

    // Number of decimals shown in the value readout (default 1).
    void setDecimals(int decimals) { decimals_ = decimals; update(); }

    void setKnobStyle(KnobStyle style);

    // Colors for the label above and the value readout below, so text stays
    // readable on cream/gold/steel panels as well as on dark ones.
    void setTextColors(const QColor& label, const QColor& valueText);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void valueChanged(float newValue);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    float normalised() const;

    void paintStudio(QPainter& p, const QPointF& centre, qreal radius) const;
    void paintSkirted(QPainter& p, const QPointF& centre, qreal radius) const;
    void paintChickenHead(QPainter& p, const QPointF& centre, qreal radius,
                          const QColor& body, const QColor& nose) const;
    void paintKnurled(QPainter& p, const QPointF& centre, qreal radius,
                      bool gold) const;
    void paintStomp(QPainter& p, const QPointF& centre, qreal radius) const;

    QString label_;
    QString unit_;
    float min_;
    float max_;
    float default_;
    float value_;
    int decimals_ = 1;
    int dragStartY_ = 0;
    float dragStartValue_ = 0.0f;

    KnobStyle style_ = KnobStyle::Studio;
    QColor labelColor_;
    QColor valueColor_;
};
