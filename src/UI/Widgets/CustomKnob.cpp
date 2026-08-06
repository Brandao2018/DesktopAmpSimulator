#include "CustomKnob.h"

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

    const QColor kAccent(0xdc, 0x64, 0x28);          // orange
    const QColor kAccentDim(0x5a, 0x32, 0x1c);
    const QColor kText(0xff, 0xff, 0xff);
    const QColor kTextDim(0x9a, 0x9a, 0x9a);

    QPointF angleDir(float screenAngleDeg)
    {
        const float rad = qDegreesToRadians(screenAngleDeg);
        return { std::cos(rad), -std::sin(rad) };
    }
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

void CustomKnob::setKnobStyle(KnobStyle style)
{
    if (style_ == style)
        return;
    style_ = style;
    updateGeometry();
    update();
}

void CustomKnob::setTextColors(const QColor& label, const QColor& valueText)
{
    labelColor_ = label;
    valueColor_ = valueText;
    update();
}

QSize CustomKnob::sizeHint() const
{
    return style_ == KnobStyle::Stomp ? QSize(56, 76) : QSize(72, 96);
}

QSize CustomKnob::minimumSizeHint() const
{
    return style_ == KnobStyle::Stomp ? QSize(50, 70) : QSize(62, 86);
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

    const QColor labelCol = labelColor_.isValid() ? labelColor_ : kText;
    const QColor valueCol = valueColor_.isValid() ? valueColor_ : kTextDim;

    // Label above.
    QFont f = font();
    f.setPointSizeF(style_ == KnobStyle::Stomp ? 6.5 : 7.5);
    f.setBold(true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    p.setFont(f);
    p.setPen(isEnabled() ? labelCol : kTextDim);
    p.drawText(QRectF(0, 0, width(), labelH), Qt::AlignCenter, label_.toUpper());

    switch (style_)
    {
        case KnobStyle::Studio:        paintStudio(p, centre, radius); break;
        case KnobStyle::FenderSkirted: paintSkirted(p, centre, radius); break;
        case KnobStyle::TweedChicken:
            paintChickenHead(p, centre, radius,
                             QColor(0x3c, 0x2c, 0x16), QColor(0xe9, 0xdf, 0xc4));
            break;
        case KnobStyle::OrangeChicken:
            paintChickenHead(p, centre, radius,
                             QColor(0x18, 0x18, 0x1a), QColor(0xf2, 0xf2, 0xf2));
            break;
        case KnobStyle::MarshallGold:  paintKnurled(p, centre, radius, true); break;
        case KnobStyle::MesaMetal:     paintKnurled(p, centre, radius, false); break;
        case KnobStyle::Stomp:         paintStomp(p, centre, radius); break;
    }

    // Value readout below.
    f.setBold(false);
    f.setPointSizeF(style_ == KnobStyle::Stomp ? 6.5 : 7.0);
    p.setFont(f);
    QColor dimValue = valueCol;
    dimValue.setAlpha(200);
    p.setPen(dimValue);
    p.drawText(QRectF(0, height() - valueH, width(), valueH), Qt::AlignCenter,
               QStringLiteral("%1 %2").arg(value_, 0, 'f', decimals_).arg(unit_));
}

void CustomKnob::paintStudio(QPainter& p, const QPointF& centre, qreal radius) const
{
    // Background value arc (full sweep, dim) + active arc up to the value.
    QPen arcPen(kAccentDim, 3.0, Qt::SolidLine, Qt::FlatCap);
    p.setPen(arcPen);
    const QRectF arcRect(centre.x() - radius - 4.0, centre.y() - radius - 4.0,
                         (radius + 4.0) * 2.0, (radius + 4.0) * 2.0);
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
    const QPointF dir = angleDir(kStartAngleDeg - kSweepDeg * normalised());
    p.setPen(QPen(isEnabled() ? kAccent : kTextDim, 2.6, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(centre + dir * (radius * 0.35), centre + dir * (radius * 0.85));
}

void CustomKnob::paintSkirted(QPainter& p, const QPointF& centre, qreal radius) const
{
    // Numbered skirt ring (1-10), like a blackface panel knob.
    QFont numFont = p.font();
    numFont.setBold(false);
    numFont.setPointSizeF(5.0);
    p.setFont(numFont);
    p.setPen(labelColor_.isValid() ? labelColor_ : kText);
    for (int i = 1; i <= 10; ++i)
    {
        const QPointF dir = angleDir(kStartAngleDeg - kSweepDeg * (i - 1) / 9.0f);
        const QPointF pos = centre + dir * (radius + 3.5);
        p.drawText(QRectF(pos.x() - 6, pos.y() - 5, 12, 10), Qt::AlignCenter,
                   QString::number(i));
    }

    // Skirt: glossy black cone.
    QRadialGradient skirt(centre - QPointF(radius * 0.25, radius * 0.3), radius * 2.0);
    skirt.setColorAt(0.0, QColor(0x3a, 0x3a, 0x3e));
    skirt.setColorAt(0.55, QColor(0x1a, 0x1a, 0x1d));
    skirt.setColorAt(1.0, QColor(0x08, 0x08, 0x0a));
    p.setPen(QPen(QColor(0x05, 0x05, 0x06), 1.0));
    p.setBrush(skirt);
    p.drawEllipse(centre, radius * 0.86, radius * 0.86);

    // Raised centre dome.
    QRadialGradient dome(centre - QPointF(radius * 0.18, radius * 0.22), radius * 0.9);
    dome.setColorAt(0.0, QColor(0x4c, 0x4c, 0x52));
    dome.setColorAt(1.0, QColor(0x14, 0x14, 0x17));
    p.setPen(QPen(QColor(0x2e, 0x2e, 0x33), 0.8));
    p.setBrush(dome);
    p.drawEllipse(centre, radius * 0.48, radius * 0.48);

    // White indicator line across the skirt.
    const QPointF dir = angleDir(kStartAngleDeg - kSweepDeg * normalised());
    p.setPen(QPen(QColor(0xf4, 0xf2, 0xea), 2.2, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(centre + dir * (radius * 0.30), centre + dir * (radius * 0.82));
}

void CustomKnob::paintChickenHead(QPainter& p, const QPointF& centre, qreal radius,
                                  const QColor& body, const QColor& nose) const
{
    const float angle = kStartAngleDeg - kSweepDeg * normalised();

    // Tick marks around the sweep.
    p.setPen(QPen(labelColor_.isValid() ? labelColor_ : kText, 1.2));
    for (int i = 0; i <= 10; ++i)
    {
        const QPointF dir = angleDir(kStartAngleDeg - kSweepDeg * i / 10.0f);
        p.drawLine(centre + dir * (radius * 0.98), centre + dir * (radius * 1.08));
    }

    p.save();
    p.translate(centre);
    p.rotate(-angle);   // painter rotation is clockwise; screen angle is CCW

    // Pointer body: round base with a tapered nose, drawn along +x.
    QPainterPath path;
    path.moveTo(radius * 0.95, 0.0);
    path.lineTo(radius * 0.18, -radius * 0.52);
    path.arcTo(QRectF(-radius * 0.58, -radius * 0.58, radius * 1.16, radius * 1.16),
               110.0, 140.0);
    path.lineTo(radius * 0.18, radius * 0.52);
    path.closeSubpath();

    QLinearGradient grad(0.0, -radius * 0.6, 0.0, radius * 0.6);
    grad.setColorAt(0.0, body.lighter(190));
    grad.setColorAt(0.35, body);
    grad.setColorAt(1.0, body.darker(160));
    p.setPen(QPen(body.darker(240), 1.0));
    p.setBrush(grad);
    p.drawPath(path);

    // Nose indicator dot.
    p.setPen(Qt::NoPen);
    p.setBrush(nose);
    p.drawEllipse(QPointF(radius * 0.74, 0.0), radius * 0.075, radius * 0.075);

    // Centre screw cap.
    QRadialGradient cap(QPointF(-radius * 0.1, -radius * 0.1), radius * 0.5);
    cap.setColorAt(0.0, body.lighter(220));
    cap.setColorAt(1.0, body.darker(130));
    p.setBrush(cap);
    p.drawEllipse(QPointF(0.0, 0.0), radius * 0.16, radius * 0.16);

    p.restore();
}

void CustomKnob::paintKnurled(QPainter& p, const QPointF& centre, qreal radius,
                              bool gold) const
{
    const QColor base = gold ? QColor(0xc9, 0xa5, 0x44) : QColor(0x3e, 0x3e, 0x44);
    const QColor hi   = gold ? QColor(0xf3, 0xdd, 0x92) : QColor(0x74, 0x74, 0x7c);
    const QColor lo   = gold ? QColor(0x77, 0x5b, 0x1c) : QColor(0x16, 0x16, 0x19);

    // Knurled rim.
    QConicalGradient rim(centre, 60.0);
    rim.setColorAt(0.00, hi);
    rim.setColorAt(0.25, base);
    rim.setColorAt(0.50, lo);
    rim.setColorAt(0.75, base);
    rim.setColorAt(1.00, hi);
    p.setPen(QPen(lo.darker(140), 1.0));
    p.setBrush(rim);
    p.drawEllipse(centre, radius * 0.92, radius * 0.92);

    // Knurl teeth: short radial dashes.
    p.setPen(QPen(QColor(0, 0, 0, 90), 1.4));
    for (int i = 0; i < 28; ++i)
    {
        const QPointF dir = angleDir(i * (360.0f / 28.0f));
        p.drawLine(centre + dir * (radius * 0.78), centre + dir * (radius * 0.92));
    }

    // Domed top.
    QRadialGradient dome(centre - QPointF(radius * 0.2, radius * 0.25), radius * 1.1);
    dome.setColorAt(0.0, hi);
    dome.setColorAt(0.6, base);
    dome.setColorAt(1.0, lo);
    p.setPen(QPen(lo, 0.8));
    p.setBrush(dome);
    p.drawEllipse(centre, radius * 0.72, radius * 0.72);

    // Indicator.
    const QPointF dir = angleDir(kStartAngleDeg - kSweepDeg * normalised());
    const QColor pointer = gold ? QColor(0x18, 0x12, 0x04) : QColor(0xf2, 0xf2, 0xf2);
    p.setPen(QPen(pointer, 2.4, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(centre + dir * (radius * 0.18), centre + dir * (radius * 0.66));
}

void CustomKnob::paintStomp(QPainter& p, const QPointF& centre, qreal radius) const
{
    // Min/max ticks.
    p.setPen(QPen(labelColor_.isValid() ? labelColor_ : kText, 1.2));
    for (float t : { 0.0f, 1.0f })
    {
        const QPointF dir = angleDir(kStartAngleDeg - kSweepDeg * t);
        p.drawLine(centre + dir * (radius * 0.98), centre + dir * (radius * 1.10));
    }

    // Fluted silver base ring.
    QRadialGradient ring(centre - QPointF(radius * 0.2, radius * 0.25), radius * 1.6);
    ring.setColorAt(0.0, QColor(0xa8, 0xa8, 0xb0));
    ring.setColorAt(1.0, QColor(0x3c, 0x3c, 0x42));
    p.setPen(QPen(QColor(0x18, 0x18, 0x1b), 1.0));
    p.setBrush(ring);
    p.drawEllipse(centre, radius * 0.92, radius * 0.92);

    p.setPen(QPen(QColor(0, 0, 0, 110), 1.6));
    for (int i = 0; i < 16; ++i)
    {
        const QPointF dir = angleDir(i * (360.0f / 16.0f));
        p.drawLine(centre + dir * (radius * 0.74), centre + dir * (radius * 0.92));
    }

    // Black cap.
    QRadialGradient cap(centre - QPointF(radius * 0.2, radius * 0.25), radius * 1.2);
    cap.setColorAt(0.0, QColor(0x3e, 0x3e, 0x42));
    cap.setColorAt(1.0, QColor(0x0e, 0x0e, 0x10));
    p.setPen(QPen(QColor(0x08, 0x08, 0x0a), 1.0));
    p.setBrush(cap);
    p.drawEllipse(centre, radius * 0.70, radius * 0.70);

    // Indicator.
    const QPointF dir = angleDir(kStartAngleDeg - kSweepDeg * normalised());
    p.setPen(QPen(QColor(0xf2, 0xf2, 0xf2), 2.2, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(centre + dir * (radius * 0.20), centre + dir * (radius * 0.62));
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
