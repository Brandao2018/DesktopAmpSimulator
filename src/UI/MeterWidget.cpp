#include "MeterWidget.h"

#include <QDateTime>
#include <QPainter>

#include "Shared/Constants.h"

namespace
{
    constexpr float kYellowZoneDb = -12.0f;
    constexpr float kRedZoneDb    = 0.0f;
}

MeterWidget::MeterWidget(const QString& label, QWidget* parent)
    : QWidget(parent),
      label_(label),
      levelL_(ampsim::kMeterFloorDb),
      levelR_(ampsim::kMeterFloorDb),
      peakHoldL_(ampsim::kMeterFloorDb),
      peakHoldR_(ampsim::kMeterFloorDb),
      peakTimeL_(0),
      peakTimeR_(0)
{
    setMinimumHeight(60);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(label_);
    setToolTip(label_);
}

QSize MeterWidget::minimumSizeHint() const
{
    return { 240, 60 };
}

void MeterWidget::setLevels(float leftDb, float rightDb)
{
    levelL_ = leftDb;
    levelR_ = rightDb;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 holdMs = static_cast<qint64>(ampsim::kPeakHoldSeconds * 1000.0f);

    if (leftDb >= peakHoldL_ || now - peakTimeL_ > holdMs)
    {
        peakHoldL_ = leftDb;
        peakTimeL_ = now;
    }
    if (rightDb >= peakHoldR_ || now - peakTimeR_ > holdMs)
    {
        peakHoldR_ = rightDb;
        peakTimeR_ = now;
    }

    update();
}

float MeterWidget::dbToNormalised(float db) const
{
    const float clamped = qBound(ampsim::kMeterMinDb, db, ampsim::kMeterMaxDb);
    return (clamped - ampsim::kMeterMinDb) / (ampsim::kMeterMaxDb - ampsim::kMeterMinDb);
}

void MeterWidget::drawChannel(QPainter& painter, const QRectF& rect,
                              float levelDb, float peakHoldDb) const
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(25, 25, 28));
    painter.drawRoundedRect(rect, 2.0, 2.0);

    const float norm = dbToNormalised(levelDb);
    if (norm > 0.0f)
    {
        const qreal filled = rect.width() * norm;
        const qreal yellowX = rect.width() * dbToNormalised(kYellowZoneDb);
        const qreal redX    = rect.width() * dbToNormalised(kRedZoneDb);

        // Green segment
        painter.setBrush(QColor(70, 200, 120));
        painter.drawRect(QRectF(rect.left(), rect.top(),
                                qMin(filled, yellowX), rect.height()));
        // Yellow segment
        if (filled > yellowX)
        {
            painter.setBrush(QColor(230, 200, 80));
            painter.drawRect(QRectF(rect.left() + yellowX, rect.top(),
                                    qMin(filled, redX) - yellowX, rect.height()));
        }
        // Red segment
        if (filled > redX)
        {
            painter.setBrush(QColor(230, 90, 80));
            painter.drawRect(QRectF(rect.left() + redX, rect.top(),
                                    filled - redX, rect.height()));
        }
    }

    // Peak-hold marker
    const float peakNorm = dbToNormalised(peakHoldDb);
    if (peakNorm > 0.0f)
    {
        painter.setBrush(QColor(240, 240, 240));
        const qreal x = rect.left() + rect.width() * peakNorm - 1.0;
        painter.drawRect(QRectF(x, rect.top(), 2.0, rect.height()));
    }
}

void MeterWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal labelWidth = 18.0;
    const qreal scaleHeight = 14.0;
    const qreal barHeight = (height() - scaleHeight - 6.0) / 2.0;

    painter.setPen(palette().color(QPalette::WindowText));
    painter.drawText(QRectF(0, 0, labelWidth, barHeight), Qt::AlignCenter, "L");
    painter.drawText(QRectF(0, barHeight + 2.0, labelWidth, barHeight), Qt::AlignCenter, "R");

    const QRectF barL(labelWidth, 0, width() - labelWidth, barHeight);
    const QRectF barR(labelWidth, barHeight + 2.0, width() - labelWidth, barHeight);

    drawChannel(painter, barL, levelL_, peakHoldL_);
    drawChannel(painter, barR, levelR_, peakHoldR_);

    // dB scale ticks
    painter.setPen(palette().color(QPalette::Mid));
    QFont f = painter.font();
    f.setPointSizeF(7.0);
    painter.setFont(f);

    const int ticks[] = { -60, -48, -36, -24, -12, -6, 0, 6 };
    const qreal scaleTop = barHeight * 2.0 + 4.0;
    for (int db : ticks)
    {
        const qreal x = labelWidth + (width() - labelWidth) * dbToNormalised(static_cast<float>(db));
        painter.drawText(QRectF(x - 14, scaleTop, 28, scaleHeight),
                         Qt::AlignCenter, QString::number(db));
    }
}
