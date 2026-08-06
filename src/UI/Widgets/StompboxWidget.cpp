#include "StompboxWidget.h"

#include <QPainter>
#include <QPainterPath>

// --- FootswitchButton --------------------------------------------------------

FootswitchButton::FootswitchButton(QWidget* parent)
    : QPushButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(44, 44);
}

void FootswitchButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPointF c(width() / 2.0, height() / 2.0);
    const qreal r = qMin(width(), height()) / 2.0 - 2.0;
    const bool pressed = isDown();

    // Base ring bolted to the chassis.
    QRadialGradient base(c - QPointF(r * 0.2, r * 0.25), r * 1.9);
    base.setColorAt(0.0, QColor(0x9a, 0x9a, 0xa2));
    base.setColorAt(1.0, QColor(0x2e, 0x2e, 0x34));
    p.setPen(QPen(QColor(0x14, 0x14, 0x16), 1.2));
    p.setBrush(base);
    p.drawEllipse(c, r, r);

    // Chrome cap; drops slightly while pressed.
    const QPointF capC = pressed ? c + QPointF(0.0, 1.2) : c;
    const qreal capR = r * 0.72;
    QRadialGradient cap(capC - QPointF(capR * 0.35, capR * 0.4), capR * 2.2);
    cap.setColorAt(0.0, pressed ? QColor(0xb8, 0xb8, 0xc0) : QColor(0xe8, 0xe8, 0xf0));
    cap.setColorAt(0.55, QColor(0x8e, 0x8e, 0x98));
    cap.setColorAt(1.0, QColor(0x46, 0x46, 0x4e));
    p.setPen(QPen(QColor(0x26, 0x26, 0x2a), 1.0));
    p.setBrush(cap);
    p.drawEllipse(capC, capR, capR);

    // Machined concentric rings on the cap.
    p.setPen(QPen(QColor(0, 0, 0, 60), 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(capC, capR * 0.62, capR * 0.62);
    p.drawEllipse(capC, capR * 0.30, capR * 0.30);
}

// --- StompboxWidget ----------------------------------------------------------

StompboxWidget::StompboxWidget(QWidget* parent)
    : QWidget(parent),
      style_(&pedalcat::kPedalStyles[0])
{
    footswitch_ = new FootswitchButton(this);
    footswitch_->setAccessibleName(tr("Effect bypass footswitch"));
    setMinimumHeight(250);
}

void StompboxWidget::setPedalType(int type)
{
    style_ = &pedalcat::styleForType(type);
    footswitch_->setVisible(!style_->empty);
    update();
}

void StompboxWidget::setEngaged(bool on)
{
    if (engaged_ == on)
        return;
    engaged_ = on;
    update();
}

void StompboxWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF box(2.0, 1.0, width() - 4.0, height() - 4.0);

    if (style_->empty)
    {
        // Recessed pedalboard slot with velcro strips.
        p.setPen(QPen(QColor(0x0e, 0x0e, 0x10), 1.2));
        p.setBrush(QColor(0x1a, 0x1a, 0x1c));
        p.drawRoundedRect(box, 8.0, 8.0);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x24, 0x24, 0x27));
        const qreal stripW = box.width() * 0.62;
        for (qreal y : { box.height() * 0.32, box.height() * 0.62 })
            p.drawRoundedRect(QRectF(box.center().x() - stripW / 2.0,
                                     box.top() + y, stripW, 14.0), 3.0, 3.0);
        QFont f = font();
        f.setPointSizeF(8.0);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
        p.setFont(f);
        p.setPen(style_->text);
        p.drawText(box, Qt::AlignCenter, QString::fromLatin1(style_->name));
        return;
    }

    // Drop shadow + enclosure.
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 90));
    p.drawRoundedRect(box.translated(0.0, 3.0), 8.0, 8.0);

    QLinearGradient bodyGrad(box.topLeft(), box.bottomLeft());
    bodyGrad.setColorAt(0.0, style_->body.lighter(126));
    bodyGrad.setColorAt(0.18, style_->body);
    bodyGrad.setColorAt(1.0, style_->body.darker(142));
    p.setPen(QPen(style_->body.darker(220), 1.4));
    p.setBrush(bodyGrad);
    p.drawRoundedRect(box, 8.0, 8.0);

    // Brushed-metal sheen down the face.
    QLinearGradient sheen(box.topLeft(), box.topRight());
    sheen.setColorAt(0.0, QColor(255, 255, 255, 0));
    sheen.setColorAt(0.30, QColor(255, 255, 255, 24));
    sheen.setColorAt(0.42, QColor(255, 255, 255, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(sheen);
    p.drawRoundedRect(box, 8.0, 8.0);

    // Corner screws.
    for (const QPointF& s : { box.topLeft() + QPointF(11, 11),
                              box.topRight() + QPointF(-11, 11),
                              box.bottomLeft() + QPointF(11, -11),
                              box.bottomRight() + QPointF(-11, -11) })
    {
        QRadialGradient screw(s - QPointF(1.0, 1.2), 5.0);
        screw.setColorAt(0.0, QColor(0xd0, 0xd0, 0xd8));
        screw.setColorAt(1.0, QColor(0x54, 0x54, 0x5c));
        p.setPen(QPen(QColor(0x1a, 0x1a, 0x1e), 0.8));
        p.setBrush(screw);
        p.drawEllipse(s, 3.6, 3.6);
        p.setPen(QPen(QColor(0x2a, 0x2a, 0x30), 1.0));
        p.drawLine(s + QPointF(-2.0, -1.2), s + QPointF(2.0, 1.2));
    }

    // Name plate above the footswitch (anchored to its live geometry).
    const qreal swTop = footswitch_->geometry().top();
    const QRectF nameRect(box.left() + 6.0, swTop - 52.0, box.width() - 12.0, 20.0);
    const QRectF subRect(box.left() + 6.0, swTop - 33.0, box.width() - 12.0, 12.0);

    QFont f = font();
    f.setPointSizeF(10.0);
    f.setBold(true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.6);
    p.setFont(f);
    const QString name = QString::fromLatin1(style_->name);
    p.setPen(QColor(0, 0, 0, style_->text.lightness() > 128 ? 140 : 40));
    p.drawText(nameRect.translated(1.0, 1.0), Qt::AlignCenter, name);
    p.setPen(style_->text);
    p.drawText(nameRect, Qt::AlignCenter, name);

    f.setPointSizeF(6.0);
    f.setBold(false);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.8);
    p.setFont(f);
    QColor sub = style_->text;
    sub.setAlpha(190);
    p.setPen(sub);
    p.drawText(subRect, Qt::AlignCenter, QString::fromLatin1(style_->subtitle));

    // Status LED between the name and the footswitch.
    const QPointF led(box.center().x(), swTop - 12.0);
    if (engaged_)
    {
        QRadialGradient halo(led, 12.0);
        QColor glow = style_->led;
        glow.setAlpha(140);
        halo.setColorAt(0.0, glow);
        glow.setAlpha(0);
        halo.setColorAt(1.0, glow);
        p.setPen(Qt::NoPen);
        p.setBrush(halo);
        p.drawEllipse(led, 12.0, 12.0);
    }
    QRadialGradient bulb(led - QPointF(1.0, 1.2), 5.0);
    bulb.setColorAt(0.0, engaged_ ? style_->led.lighter(180) : style_->led.darker(300));
    bulb.setColorAt(1.0, engaged_ ? style_->led : style_->led.darker(420));
    p.setPen(QPen(QColor(0x14, 0x14, 0x16), 1.0));
    p.setBrush(bulb);
    p.drawEllipse(led, 4.0, 4.0);
}
