#include "AmpHeadWidget.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>

using namespace ampcat;

namespace
{
    // Geometry shared between painting and the knob-row layout margins.
    constexpr int kLogoH   = 58;   // tolex band above the panel (logo lives here)
    constexpr int kBottomH = 24;   // tolex band below the panel
    constexpr int kSideM   = 22;   // tolex left/right of the panel
    constexpr int kJackW   = 54;   // panel space reserved for the input jack
    constexpr int kJewelW  = 50;   // panel space reserved for the pilot lamp

    // Small deterministic PRNG so the tolex grain never shimmers on repaint.
    struct Lcg
    {
        quint32 s = 0x2545f491u;
        quint32 next() { s = s * 1664525u + 1013904223u; return s >> 8; }
        int range(int n) { return static_cast<int>(next() % static_cast<quint32>(n)); }
    };
}

AmpHeadWidget::AmpHeadWidget(QWidget* parent)
    : QWidget(parent),
      spec_(&kAmpModels[0])
{
    setMinimumHeight(236);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    knobRow_ = new QHBoxLayout(this);
    knobRow_->setContentsMargins(kSideM + kJackW, kLogoH + 10,
                                 kSideM + kJewelW, kBottomH + 14);
    knobRow_->setSpacing(4);

    rebuildTolexTile();
}

void AmpHeadWidget::setSpec(const AmpModelSpec* spec)
{
    if (spec_ == spec)
        return;
    spec_ = spec;
    rebuildTolexTile();
    update();
}

void AmpHeadWidget::setLampOn(bool on)
{
    if (lampOn_ == on)
        return;
    lampOn_ = on;
    update();
}

void AmpHeadWidget::rebuildTolexTile()
{
    const QColor base = spec_->tolex;
    const QColor dark = spec_->tolexDark;

    QPixmap tile(96, 96);
    tile.fill(base);
    QPainter p(&tile);
    Lcg rng;

    switch (spec_->tolexStyle)
    {
        case TolexStyle::Smooth:
            // Fine speckle grain.
            for (int i = 0; i < 900; ++i)
            {
                QColor c = (i % 3 == 0) ? base.lighter(125) : dark;
                c.setAlpha(50 + rng.range(60));
                p.setPen(c);
                p.drawPoint(rng.range(96), rng.range(96));
            }
            break;

        case TolexStyle::Levant:
            // Coarse leather-grain blotches.
            for (int i = 0; i < 260; ++i)
            {
                QColor c = (i % 4 == 0) ? base.lighter(120) : dark;
                c.setAlpha(40 + rng.range(55));
                p.setPen(QPen(c, 1.0));
                const int x = rng.range(96), y = rng.range(96);
                const int w = 2 + rng.range(4), h = 1 + rng.range(3);
                p.drawArc(x, y, w, h, rng.range(360) * 16, (120 + rng.range(200)) * 16);
            }
            break;

        case TolexStyle::Tweed:
        {
            // Two-tone diagonal weave.
            p.setRenderHint(QPainter::Antialiasing);
            QPen light(base.lighter(122), 3.0);
            QPen shade(dark, 3.0);
            for (int o = -96; o <= 192; o += 8)
            {
                p.setPen((o / 8) % 2 == 0 ? light : shade);
                p.drawLine(o, 0, o + 96, 96);
            }
            QColor cross = dark;
            cross.setAlpha(70);
            p.setPen(QPen(cross, 2.0));
            for (int o = -96; o <= 192; o += 12)
                p.drawLine(o + 96, 0, o, 96);
            break;
        }
    }
    p.end();
    tolexTile_ = tile;
}

void AmpHeadWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF body(3.0, 2.0, width() - 6.0, height() - 5.0);
    const QRectF panel(kSideM, kLogoH, width() - 2.0 * kSideM,
                       height() - kLogoH - kBottomH);
    const QRectF logoBand(kSideM, 4.0, width() - 2.0 * kSideM, kLogoH - 8.0);

    // Drop shadow under the cabinet.
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 90));
    p.drawRoundedRect(body.translated(0.0, 3.0), 10.0, 10.0);

    // Tolex-covered box.
    QPainterPath bodyPath;
    bodyPath.addRoundedRect(body, 10.0, 10.0);
    p.save();
    p.setClipPath(bodyPath);
    p.drawTiledPixmap(body.toRect(), tolexTile_);

    // Edge shading: darker at the borders, faint sheen up top.
    QLinearGradient shade(body.topLeft(), body.bottomLeft());
    shade.setColorAt(0.0, QColor(255, 255, 255, 26));
    shade.setColorAt(0.12, QColor(255, 255, 255, 0));
    shade.setColorAt(0.85, QColor(0, 0, 0, 0));
    shade.setColorAt(1.0, QColor(0, 0, 0, 90));
    p.fillRect(body, shade);
    QLinearGradient sideShade(body.topLeft(), body.topRight());
    sideShade.setColorAt(0.0, QColor(0, 0, 0, 70));
    sideShade.setColorAt(0.06, QColor(0, 0, 0, 0));
    sideShade.setColorAt(0.94, QColor(0, 0, 0, 0));
    sideShade.setColorAt(1.0, QColor(0, 0, 0, 70));
    p.fillRect(body, sideShade);
    p.restore();

    p.setPen(QPen(spec_->tolexDark.darker(160), 1.6));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(body, 10.0, 10.0);

    // Piping / trim line framing the control panel.
    p.setPen(QPen(spec_->piping, 2.0));
    p.drawRoundedRect(panel.adjusted(-3.0, -3.0, 3.0, 3.0), 5.0, 5.0);

    paintPanel(p, panel);
    paintLogo(p, logoBand);
    paintCornerProtectors(p, body);
    paintJack(p, QPointF(panel.left() + kJackW * 0.52,
                         panel.center().y() + 6.0));
    paintJewel(p, QPointF(panel.right() - kJewelW * 0.5,
                          panel.center().y() - 2.0));
}

void AmpHeadWidget::paintPanel(QPainter& p, const QRectF& panel) const
{
    switch (spec_->panelStyle)
    {
        case PanelStyle::Blackface:
        {
            p.setPen(Qt::NoPen);
            p.setBrush(spec_->panel);
            p.drawRoundedRect(panel, 4.0, 4.0);
            // Aluminum trim strips top and bottom.
            QLinearGradient alu(panel.topLeft(), panel.topLeft() + QPointF(0, 5));
            alu.setColorAt(0.0, QColor(0xd8, 0xd8, 0xdc));
            alu.setColorAt(1.0, QColor(0x88, 0x88, 0x8e));
            p.fillRect(QRectF(panel.left(), panel.top(), panel.width(), 4.0), alu);
            p.fillRect(QRectF(panel.left(), panel.bottom() - 4.0, panel.width(), 4.0), alu);
            break;
        }

        case PanelStyle::ChromeStrip:
        {
            QLinearGradient chrome(panel.topLeft(), panel.bottomLeft());
            chrome.setColorAt(0.0, QColor(0xf0, 0xf0, 0xf4));
            chrome.setColorAt(0.45, QColor(0xc2, 0xc2, 0xc8));
            chrome.setColorAt(0.55, QColor(0x8e, 0x8e, 0x96));
            chrome.setColorAt(1.0, QColor(0xd4, 0xd4, 0xda));
            p.setPen(QPen(QColor(0x50, 0x50, 0x56), 1.2));
            p.setBrush(chrome);
            p.drawRoundedRect(panel, 3.0, 3.0);
            break;
        }

        case PanelStyle::GoldBrushed:
        {
            QLinearGradient gold(panel.topLeft(), panel.bottomLeft());
            gold.setColorAt(0.0, spec_->panel.lighter(130));
            gold.setColorAt(0.5, spec_->panel);
            gold.setColorAt(1.0, spec_->panel.darker(135));
            p.setPen(QPen(spec_->panel.darker(180), 1.2));
            p.setBrush(gold);
            p.drawRoundedRect(panel, 3.0, 3.0);
            // Horizontal brush marks.
            QColor brush = spec_->panel.darker(150);
            brush.setAlpha(26);
            p.setPen(QPen(brush, 1.0));
            for (qreal y = panel.top() + 3.0; y < panel.bottom() - 2.0; y += 2.0)
                p.drawLine(QPointF(panel.left() + 3.0, y), QPointF(panel.right() - 3.0, y));
            break;
        }

        case PanelStyle::DiamondPlate:
        case PanelStyle::BrushedSteel:
        {
            QLinearGradient steel(panel.topLeft(), panel.bottomLeft());
            steel.setColorAt(0.0, spec_->panel.lighter(125));
            steel.setColorAt(0.5, spec_->panel);
            steel.setColorAt(1.0, spec_->panel.darker(140));
            p.setPen(QPen(spec_->panel.darker(200), 1.2));
            p.setBrush(steel);
            p.drawRoundedRect(panel, 3.0, 3.0);

            if (spec_->panelStyle == PanelStyle::DiamondPlate)
            {
                // Staggered diamond tread marks.
                p.save();
                p.setClipRect(panel.adjusted(2, 2, -2, -2));
                const QColor hi = spec_->panel.lighter(160);
                const QColor lo = spec_->panel.darker(170);
                int row = 0;
                for (qreal y = panel.top() + 8.0; y < panel.bottom(); y += 14.0, ++row)
                    for (qreal x = panel.left() + 10.0 + (row % 2) * 11.0;
                         x < panel.right(); x += 22.0)
                    {
                        p.setPen(QPen(lo, 1.6, Qt::SolidLine, Qt::RoundCap));
                        p.drawLine(QPointF(x - 3.0, y + 3.0), QPointF(x + 3.0, y - 3.0));
                        p.setPen(QPen(hi, 1.2, Qt::SolidLine, Qt::RoundCap));
                        p.drawLine(QPointF(x - 3.5, y + 2.5), QPointF(x + 2.5, y - 3.5));
                    }
                p.restore();
            }
            else
            {
                QColor brush = spec_->panel.darker(140);
                brush.setAlpha(30);
                p.setPen(QPen(brush, 1.0));
                for (qreal y = panel.top() + 3.0; y < panel.bottom() - 2.0; y += 2.0)
                    p.drawLine(QPointF(panel.left() + 3.0, y),
                               QPointF(panel.right() - 3.0, y));
            }
            break;
        }

        case PanelStyle::CreamHieroglyph:
        {
            p.setPen(QPen(QColor(0x2a, 0x24, 0x1a), 1.4));
            p.setBrush(spec_->panel);
            p.drawRoundedRect(panel, 4.0, 4.0);
            p.setPen(QPen(QColor(0x2a, 0x24, 0x1a), 1.0));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(panel.adjusted(4.0, 4.0, -4.0, -4.0), 3.0, 3.0);
            break;
        }
    }
}

void AmpHeadWidget::paintLogo(QPainter& p, const QRectF& band) const
{
    QFont f = font();

    // Maker logo, left.
    switch (spec_->logoStyle)
    {
        case LogoStyle::Script:
        {
            f.setFamily(QStringLiteral("Brush Script MT"));
            f.setPointSizeF(26.0);
            f.setItalic(true);
            p.setFont(f);
            const QString text = QString::fromLatin1(spec_->maker);
            const QRectF textRect(band.left() + 16.0, band.top(), 300.0, band.height());
            // Soft shadow, then the script itself.
            p.setPen(QColor(0, 0, 0, 130));
            p.drawText(textRect.translated(1.5, 1.5), Qt::AlignVCenter | Qt::AlignLeft, text);
            p.setPen(spec_->logoColor);
            p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);
            // Underline swoosh.
            const qreal w = QFontMetricsF(f).horizontalAdvance(text);
            QPainterPath tail;
            tail.moveTo(textRect.left() + 6.0, band.center().y() + 14.0);
            tail.cubicTo(textRect.left() + w * 0.4, band.center().y() + 20.0,
                         textRect.left() + w * 0.8, band.center().y() + 18.0,
                         textRect.left() + w + 10.0, band.center().y() + 8.0);
            p.setPen(QPen(spec_->logoColor, 1.6));
            p.drawPath(tail);
            break;
        }

        case LogoStyle::BlockSerif:
        {
            // Bright plate with heavy lettering (the reference's framed look).
            f.setFamily(QStringLiteral("Georgia"));
            f.setPointSizeF(19.0);
            f.setBold(true);
            f.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
            p.setFont(f);
            const QString text = QString::fromLatin1(spec_->maker).toUpper();
            const qreal w = QFontMetricsF(p.font()).horizontalAdvance(text) + 34.0;
            const QRectF plate(band.left() + 10.0, band.top() + 4.0, w, band.height() - 8.0);
            QLinearGradient plateGrad(plate.topLeft(), plate.bottomLeft());
            plateGrad.setColorAt(0.0, QColor(0xfb, 0xf6, 0xe8));
            plateGrad.setColorAt(1.0, QColor(0xe4, 0xdb, 0xc2));
            p.setPen(QPen(QColor(0x2a, 0x24, 0x1a), 1.4));
            p.setBrush(plateGrad);
            p.drawRoundedRect(plate, 4.0, 4.0);
            p.setPen(spec_->logoColor);
            p.drawText(plate, Qt::AlignCenter, text);
            break;
        }

        case LogoStyle::MetalPlate:
        {
            f.setFamily(QStringLiteral("Arial"));
            f.setPointSizeF(13.0);
            f.setBold(true);
            f.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
            p.setFont(f);
            const QString text = QString::fromLatin1(spec_->maker).toUpper();
            const qreal w = QFontMetricsF(p.font()).horizontalAdvance(text) + 40.0;
            const QRectF plate(band.left() + 10.0, band.top() + 8.0, w, band.height() - 16.0);
            QLinearGradient steel(plate.topLeft(), plate.bottomLeft());
            steel.setColorAt(0.0, QColor(0x6e, 0x6e, 0x76));
            steel.setColorAt(0.5, QColor(0x3a, 0x3a, 0x40));
            steel.setColorAt(1.0, QColor(0x56, 0x56, 0x5e));
            p.setPen(QPen(QColor(0x14, 0x14, 0x16), 1.4));
            p.setBrush(steel);
            p.drawRoundedRect(plate, 3.0, 3.0);
            // Engraved: dark text with a light offset ghost.
            p.setPen(QColor(0, 0, 0, 160));
            p.drawText(plate.translated(0.0, 1.0), Qt::AlignCenter, text);
            p.setPen(spec_->logoColor);
            p.drawText(plate, Qt::AlignCenter, text);
            break;
        }
    }

    // Model badge, right.
    QFont badgeFont = font();
    badgeFont.setPointSizeF(9.0);
    badgeFont.setItalic(true);
    badgeFont.setBold(true);
    p.setFont(badgeFont);
    const QString model = QString::fromLatin1(spec_->model);
    const qreal bw = QFontMetricsF(badgeFont).horizontalAdvance(model) + 26.0;
    const QRectF badge(band.right() - bw - 8.0, band.center().y() - 12.0, bw, 24.0);
    QLinearGradient badgeGrad(badge.topLeft(), badge.bottomLeft());
    badgeGrad.setColorAt(0.0, spec_->panel.lighter(112));
    badgeGrad.setColorAt(1.0, spec_->panel.darker(112));
    p.setPen(QPen(spec_->panel.darker(170), 1.2));
    p.setBrush(badgeGrad);
    p.drawRoundedRect(badge, 4.0, 4.0);
    p.setPen(spec_->panelText);
    p.drawText(badge, Qt::AlignCenter, model);
}

void AmpHeadWidget::paintCornerProtectors(QPainter& p, const QRectF& body) const
{
    QLinearGradient metal(0.0, 0.0, 18.0, 18.0);
    metal.setColorAt(0.0, QColor(0xc6, 0xc6, 0xce));
    metal.setColorAt(0.5, QColor(0x8a, 0x8a, 0x94));
    metal.setColorAt(1.0, QColor(0x5a, 0x5a, 0x64));

    const struct { QPointF corner; qreal startAngle; } corners[4] = {
        { body.topLeft(),     90.0 },
        { body.topRight(),     0.0 },
        { body.bottomRight(), 270.0 },
        { body.bottomLeft(),  180.0 },
    };

    for (const auto& c : corners)
    {
        QPainterPath path;
        path.moveTo(c.corner);
        path.arcTo(QRectF(c.corner.x() - 22.0, c.corner.y() - 22.0, 44.0, 44.0),
                   c.startAngle, 90.0);
        path.closeSubpath();
        p.setPen(QPen(QColor(0x32, 0x32, 0x38), 1.0));
        p.setBrush(metal);
        p.drawPath(path);
    }
}

void AmpHeadWidget::paintJack(QPainter& p, const QPointF& centre) const
{
    // Chrome washer.
    QRadialGradient ring(centre - QPointF(2.0, 2.5), 14.0);
    ring.setColorAt(0.0, QColor(0xd8, 0xd8, 0xde));
    ring.setColorAt(1.0, QColor(0x62, 0x62, 0x6a));
    p.setPen(QPen(QColor(0x22, 0x22, 0x26), 1.0));
    p.setBrush(ring);
    p.drawEllipse(centre, 10.0, 10.0);
    // Socket hole.
    p.setBrush(QColor(0x0a, 0x0a, 0x0c));
    p.drawEllipse(centre, 5.0, 5.0);

    QFont f = font();
    f.setPointSizeF(6.0);
    f.setBold(true);
    p.setFont(f);
    p.setPen(spec_->panelText);
    p.drawText(QRectF(centre.x() - 24.0, centre.y() + 12.0, 48.0, 12.0),
               Qt::AlignCenter, tr("INPUT"));
}

void AmpHeadWidget::paintJewel(QPainter& p, const QPointF& centre) const
{
    const QColor lit = spec_->jewel;
    const QColor offCol = lit.darker(320);

    if (lampOn_)
    {
        // Glow halo.
        QRadialGradient halo(centre, 20.0);
        QColor glow = lit;
        glow.setAlpha(110);
        halo.setColorAt(0.0, glow);
        glow.setAlpha(0);
        halo.setColorAt(1.0, glow);
        p.setPen(Qt::NoPen);
        p.setBrush(halo);
        p.drawEllipse(centre, 20.0, 20.0);
    }

    // Bezel.
    QRadialGradient bezel(centre - QPointF(2.0, 2.5), 13.0);
    bezel.setColorAt(0.0, QColor(0xb4, 0xb4, 0xbc));
    bezel.setColorAt(1.0, QColor(0x4a, 0x4a, 0x52));
    p.setPen(QPen(QColor(0x1c, 0x1c, 0x20), 1.0));
    p.setBrush(bezel);
    p.drawEllipse(centre, 9.5, 9.5);

    // Faceted jewel.
    QRadialGradient jewel(centre - QPointF(1.5, 2.0), 8.0);
    jewel.setColorAt(0.0, lampOn_ ? lit.lighter(170) : offCol.lighter(130));
    jewel.setColorAt(1.0, lampOn_ ? lit.darker(130) : offCol);
    p.setPen(Qt::NoPen);
    p.setBrush(jewel);
    p.drawEllipse(centre, 6.5, 6.5);

    QFont f = font();
    f.setPointSizeF(6.0);
    f.setBold(true);
    p.setFont(f);
    p.setPen(spec_->panelText);
    p.drawText(QRectF(centre.x() - 24.0, centre.y() + 12.0, 48.0, 12.0),
               Qt::AlignCenter, tr("POWER"));
}
