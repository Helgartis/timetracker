#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QDate>
#include <QVector>
#include <QUuid>
#include "event.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // CRUD
    void onAddEventClicked();
    void onEditEventClicked();
    void onDeleteEventClicked();

    // Навигация по датам
    void onDateChanged(const QDate &date);

    // Аналитика
    void onAnalyzeClicked();

private:
    Ui::MainWindow *ui;

    // Данные по дням
    QMap<QDate, QVector<Event>> eventsByDate;
    QDate currentDate;

    // «Пасхальный режим» (вкл/выкл через чекбокс esteggcheckBox на главном окне)
    bool m_easterEnabled = false;

    // Перестроение списка событий по текущей дате
    void rebuildEventList();

    // Загрузка/сохранение событий выбранного дня (JSON)
    void loadEventsForDate(const QDate &date);
    void saveEventsForDate(const QDate &date);

    // Поиск события по устойчивому идентификатору
    int findEventIndexById(const QUuid& id) const;
};

#endif // MAINWINDOW_H
