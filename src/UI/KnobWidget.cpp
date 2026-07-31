#include "KnobWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QtMath>

namespace
{
    // Sweep range of the pointer: 7 o'clock to 5 o'clock.
    constexpr float kStartAngleDeg = 225.0f;
    constexpr float kSweepDeg      = 270.0f;
}

KnobWidget::KnobWidget(const QString& label, float minValue, float maxValue,
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

void KnobWidget::setValue(float v)
{
    const float clamped = qBound(min_, v, max_);
    if (qFuzzyCompare(clamped, value_))
        return;

    value_ = clamped;
    update();
    emit valueChanged(value_);
}

float KnobWidget::normalised() const
{
    return (value_ - min_) / (max_ - min_);
}

void KnobWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal side = qMin(width(), height() - 34);
    const QRectF knobRect((width() - side) / 2.0 + 8, 4 + 8, side - 16, side - 16);
    const QPointF centre = knobRect.center();
    const qreal radius = knobRect.width() / 2.0;

    // Tick marks around the bezel.
    p.setPen(QPen(QColor(120, 130, 150), 1.4));
    for (int i = 0; i <= 10; ++i)
    {
        const float angle = qDegreesToRadians(kStartAngleDeg - kSweepDeg * i / 10.0f);
        const QPointF dir(std::cos(angle), -std::sin(angle));
        p.drawLine(centre + dir * (radius + 3.0), centre + dir * (radius + 7.0));
    }

    // Dark bezel.
    QRadialGradient bezel(centre, radius + 2.0);
    bezel.setColorAt(0.0, QColor(60, 62, 70));
    bezel.setColorAt(1.0, QColor(20, 22, 28));
    p.setPen(Qt::NoPen);
    p.setBrush(bezel);
    p.drawEllipse(centre, radius + 1.0, radius + 1.0);

    // Cream cap.
    QRadialGradient cap(centre - QPointF(radius * 0.3, radius * 0.3), radius * 1.6);
    cap.setColorAt(0.0, QColor(245, 238, 220));
    cap.setColorAt(1.0, QColor(202, 192, 168));
    p.setBrush(cap);
    p.drawEllipse(centre, radius * 0.82, radius * 0.82);

    // Pointer line.
    const float angle = qDegreesToRadians(kStartAngleDeg - kSweepDeg * normalised());
    const QPointF dir(std::cos(angle), -std::sin(angle));
    p.setPen(QPen(QColor(40, 40, 46), 3.0, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(centre + dir * (radius * 0.25), centre + dir * (radius * 0.72));

    // Label and value.
    p.setPen(QColor(214, 218, 228));
    QFont f = font();
    f.setPointSizeF(8.5);
    f.setBold(true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    p.setFont(f);
    p.drawText(QRectF(0, knobRect.bottom() + 10, width(), 14),
               Qt::AlignHCenter, label_.toUpper());

    f.setBold(false);
    f.setPointSizeF(7.5);
    p.setFont(f);
    p.setPen(QColor(150, 158, 172));
    p.drawText(QRectF(0, knobRect.bottom() + 24, width(), 12), Qt::AlignHCenter,
               QStringLiteral("%1 %2").arg(value_, 0, 'f', 1).arg(unit_));
}

void KnobWidget::mousePressEvent(QMouseEvent* event)
{
    dragStartY_ = event->pos().y();
    dragStartValue_ = value_;
}

void KnobWidget::mouseMoveEvent(QMouseEvent* event)
{
    // 150 px of vertical drag = full range.
    const float delta = static_cast<float>(dragStartY_ - event->pos().y()) / 150.0f;
    setValue(dragStartValue_ + delta * (max_ - min_));
}

void KnobWidget::mouseDoubleClickEvent(QMouseEvent*)
{
    setValue(default_);
}

void KnobWidget::wheelEvent(QWheelEvent* event)
{
    const float step = (max_ - min_) / 40.0f;
    setValue(value_ + (event->angleDelta().y() > 0 ? step : -step));
    event->accept();
}
