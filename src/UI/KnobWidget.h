#pragma once

#include <QWidget>

// Rotary amp-style knob: cream cap on a dark bezel with a tick-mark scale,
// like a classic guitar amplifier. Drag vertically (or scroll) to change,
// double-click to reset to default.

class KnobWidget : public QWidget
{
    Q_OBJECT

public:
    KnobWidget(const QString& label, float minValue, float maxValue,
               float defaultValue, const QString& unit, QWidget* parent = nullptr);

    float value() const { return value_; }
    void setValue(float v);

    QSize sizeHint() const override { return { 84, 110 }; }
    QSize minimumSizeHint() const override { return { 72, 100 }; }

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

    QString label_;
    QString unit_;
    float min_;
    float max_;
    float default_;
    float value_;
    int dragStartY_ = 0;
    float dragStartValue_ = 0.0f;
};
