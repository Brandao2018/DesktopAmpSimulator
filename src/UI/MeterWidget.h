#pragma once

#include <QWidget>

// Stereo level meter with a dB scale (-60 .. +6 dBFS), colour zones
// (green / yellow / red) and a peak-hold marker per channel.
//
// The UI thread feeds it values via setLevels(); it never touches the audio
// engine directly.

class MeterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MeterWidget(const QString& label, QWidget* parent = nullptr);

    // Values in dBFS. Called from the UI refresh timer (~30 Hz).
    void setLevels(float leftDb, float rightDb);

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    float dbToNormalised(float db) const;
    void drawChannel(class QPainter& painter, const QRectF& rect,
                     float levelDb, float peakHoldDb) const;

    QString label_;
    float levelL_;
    float levelR_;
    float peakHoldL_;
    float peakHoldR_;
    qint64 peakTimeL_;
    qint64 peakTimeR_;
};
