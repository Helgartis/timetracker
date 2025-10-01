#ifndef HISTOGRAMWIDGET_H
#define HISTOGRAMWIDGET_H

#include <QWidget>
#include <QMap>
#include <QString>

class HistogramWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HistogramWidget(QWidget *parent = nullptr);
    void setData(const QMap<QString, int> &data);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QMap<QString, int> timeByTag;
};

#endif // HISTOGRAMWIDGET_H
