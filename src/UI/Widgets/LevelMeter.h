#pragma once

#include <QWidget>

// Professional stereo level meter: dB scale (-60 .. +6 dBFS), green/yellow/
// red zones, peak-hold markers, and a latching clip indicator per channel
// that lights when the level touches 0 dBFS (click the meter to reset).
//
// The UI thread feeds it values via setLevels(); it never touches the audio
// engine directly.

class LevelMeter : public QWidget
{
    Q_OBJECT

public:
    explicit LevelMeter(const QString& label, QWidget* parent = nullptr);

    // Values in dBFS. Called from the UI refresh timer (~30 Hz).
    void setLevels(float leftDb, float rightDb);

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;   // click resets clip LEDs

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
    bool clippedL_ = false;
    bool clippedR_ = false;
};
