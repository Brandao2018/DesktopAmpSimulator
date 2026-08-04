#pragma once

#include <QWidget>

// Professional rotary knob in the dark/orange studio theme: charcoal cap,
// orange value arc and pointer, label above and live value readout below.
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

    QSize sizeHint() const override { return { 72, 92 }; }
    QSize minimumSizeHint() const override { return { 62, 84 }; }

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
    int decimals_ = 1;
    int dragStartY_ = 0;
    float dragStartValue_ = 0.0f;
};
