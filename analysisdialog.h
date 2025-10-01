#ifndef ANALYSISDIALOG_H
#define ANALYSISDIALOG_H

#include <QDialog>
#include <QDate>
#include <QMap>
#include <QString>

namespace Ui {
class AnalysisDialog;
}

class AnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnalysisDialog(QWidget *parent = nullptr);
    ~AnalysisDialog();

    // Вкл/выкл «пасхального режима» (добавляет/убирает пасхальные выводы)
    void setEasterEnabled(bool on) { m_easterEnabled = on; }
    bool isEasterEnabled() const   { return m_easterEnabled; }

private slots:
    void onAnalyzeClicked();

private:
    Ui::AnalysisDialog *ui;

    // Флаг «пасхального режима». Управляется из MainWindow через setEasterEnabled()
    bool m_easterEnabled = false;

    // Агрегация минут по тегам за диапазон дат
    QMap<QString, int> loadDataByTags(const QDate &from, const QDate &to);

    // Отрисовка таблицы сводки (внутри реализации проверяется m_easterEnabled)
    void displaySummaryTable(const QMap<QString, int> &durations, int totalMinutes);

    // (опционально) если в .cpp будет нужен отдельный блок для пасхальных сообщений —
    // их проще держать в отдельном методе, но объявлять не обязательно.
    // void appendEasterNotes(QString& text) const;
};

#endif // ANALYSISDIALOG_H
