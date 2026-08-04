#include "CustomKnob.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QtMath>

namespace
{
    // Sweep range of the pointer: 7 o'clock to 5 o'clock.
    constexpr float kStartAngleDeg = 225.0f;
    constexpr float kSweepDeg      = 270.0f;

    const QColor kAccent(0xdc, 0x64, 0x28);          // orange
    const QColor kAccentDim(0x5a, 0x32, 0x1c);
    const QColor kText(0xff, 0xff, 0xff);
    const QColor kTextDim(0x9a, 0x9a, 0x9a);
}

CustomKnob::CustomKnob(const QString& label, float minValue, float maxValue,
                       float defaultValue, const QString& unit, QWidget* parent)
    : QWidget(parent),
      label_(label),
      unit_(unit),
      min_(minValue),
      max_(maxValue),
      default_(defaultValue),
      value_(defaultValue)
{
    setCursor(Qt::PointingHandCursor);
    setAccessibleName(label);
    setToolTip(label);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void CustomKnob::setValue(float v)
{
    const float clamped = qBound(min_, v, max_);
    if (qFuzzyCompare(clamped, value_))
        return;

    value_ = clamped;
    update();
    emit valueChanged(value_);
}

float CustomKnob::normalised() const
{
    return (value_ - min_) / (max_ - min_);
}

void CustomKnob::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal labelH = 13.0;
    const qreal valueH = 13.0;
    const qreal side = qMin<qreal>(width(), height() - labelH - valueH - 4.0);
    const QRectF knobRect((width() - side) / 2.0 + 6.0, labelH + 2.0 + 6.0,
                          side - 12.0, side - 12.0);
    const QPointF centre = knobRect.center();
    const qreal radius = knobRect.width() / 2.0;

    // Label above.
    QFont f = font();
    f.setPointSizeF(7.5);
    f.setBold(true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    p.setFont(f);
    p.setPen(isEnabled() ? kText : kTextDim);
    p.drawText(QRectF(0, 0, width(), labelH), Qt::AlignCenter, label_.toUpper());

    // Background value arc (full sweep, dim) + active arc up to the value.
    QPen arcPen(kAccentDim, 3.0, Qt::SolidLine, Qt::FlatCap);
    p.setPen(arcPen);
    const QRectF arcRect = knobRect.adjusted(-4.0, -4.0, 4.0, 4.0);
    p.drawArc(arcRect, static_cast<int>((kStartAngleDeg - kSweepDeg) * 16),
              static_cast<int>(kSweepDeg * 16));

    arcPen.setColor(isEnabled() ? kAccent : kAccentDim);
    p.setPen(arcPen);
    const float sweep = kSweepDeg * normalised();
    p.drawArc(arcRect, static_cast<int>((kStartAngleDeg - sweep) * 16),
              static_cast<int>(sweep * 16));

    // Charcoal cap with a subtle top-light gradient.
    QRadialGradient cap(centre - QPointF(radius * 0.3, radius * 0.35), radius * 1.8);
    cap.setColorAt(0.0, QColor(0x48, 0x48, 0x48));
    cap.setColorAt(1.0, QColor(0x1c, 0x1c, 0x1c));
    p.setPen(QPen(QColor(0x0c, 0x0c, 0x0c), 1.2));
    p.setBrush(cap);
    p.drawEllipse(centre, radius, radius);

    // Pointer.
    const float angle = qDegreesToRadians(kStartAngleDeg - kSweepDeg * normalised());
    const QPointF dir(std::cos(angle), -std::sin(angle));
    p.setPen(QPen(isEnabled() ? kAccent : kTextDim, 2.6, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(centre + dir * (radius * 0.35), centre + dir * (radius * 0.85));

    // Value readout below.
    f.setBold(false);
    f.setPointSizeF(7.0);
    p.setFont(f);
    p.setPen(kTextDim);
    p.drawText(QRectF(0, height() - valueH, width(), valueH), Qt::AlignCenter,
               QStringLiteral("%1 %2").arg(value_, 0, 'f', decimals_).arg(unit_));
}

void CustomKnob::mousePressEvent(QMouseEvent* event)
{
    dragStartY_ = event->pos().y();
    dragStartValue_ = value_;
}

void CustomKnob::mouseMoveEvent(QMouseEvent* event)
{
    // 150 px of vertical drag = full range.
    const float delta = static_cast<float>(dragStartY_ - event->pos().y()) / 150.0f;
    setValue(dragStartValue_ + delta * (max_ - min_));
}

void CustomKnob::mouseDoubleClickEvent(QMouseEvent*)
{
    setValue(default_);
}

void CustomKnob::wheelEvent(QWheelEvent* event)
{
    const float step = (max_ - min_) / 40.0f;
    setValue(value_ + (event->angleDelta().y() > 0 ? step : -step));
    event->accept();
}
