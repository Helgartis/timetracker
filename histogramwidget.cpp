#include "histogramwidget.h"
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>

HistogramWidget::HistogramWidget(QWidget *parent)
    : QWidget(parent)
{
}

void HistogramWidget::setData(const QMap<QString, int> &data)
{
    timeByTag = data;
    update();
}

void HistogramWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (timeByTag.isEmpty()) {
        painter.drawText(rect(), Qt::AlignCenter, "Нет данных для отображения");
        return;
    }

    int w = width();
    int h = height();
    int margin = 30;
    int spacing = 15;
    int count = timeByTag.size();
    int barWidth = (w - 2 * margin) / count;

    int maxValue = std::max_element(timeByTag.begin(), timeByTag.end()).value();

    int i = 0;
    for (auto it = timeByTag.constBegin(); it != timeByTag.constEnd(); ++it, ++i) {
        int value = it.value();
        int barHeight = static_cast<int>((double)value / maxValue * (h - 2 * margin));

        QRect barRect(margin + i * barWidth, h - margin - barHeight, barWidth - spacing, barHeight);
        painter.setBrush(Qt::darkCyan);
        painter.drawRect(barRect);

        painter.drawText(barRect.x(), h - margin + 15, it.key());

        QString valueStr = QString::number(value) + " мин";
        painter.drawText(barRect.x(), barRect.y() - 5, valueStr);
    }
}
