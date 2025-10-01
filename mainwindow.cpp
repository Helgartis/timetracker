#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "eventdialog.h"
#include "analysisdialog.h"

#include <QListWidgetItem>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <QCoreApplication>   // ✅ добавлено для applicationDirPath()
#include <algorithm>

namespace {

// Папка для хранения данных приложения.
// ✅ Теперь — локальная ./data рядом с исполняемым файлом.
static QString appDataDir()
{
    const QString base = QCoreApplication::applicationDirPath() + "/data";
    QDir().mkpath(base);  // гарантируем наличие папки
    return base;
}

static QString eventsFileForDate(const QDate& date)
{
    return appDataDir() + "/" + date.toString("yyyy-MM-dd") + ".json";
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    currentDate = ui->calendarWidget->selectedDate();

    // Инициализация «пасхального режима» и подписка на чекбокс
    m_easterEnabled = ui->esteggcheckBox->isChecked();
    connect(ui->esteggcheckBox, &QCheckBox::toggled, this, [this](bool on){
        m_easterEnabled = on;
    });

    connect(ui->calendarWidget, &QCalendarWidget::selectionChanged, this, [this]() {
        currentDate = ui->calendarWidget->selectedDate();
        onDateChanged(currentDate);
    });

    connect(ui->pushButtonCreate,  &QPushButton::clicked, this, &MainWindow::onAddEventClicked);
    connect(ui->pushButtonEdit,    &QPushButton::clicked, this, &MainWindow::onEditEventClicked);
    connect(ui->pushButtonDelete,  &QPushButton::clicked, this, &MainWindow::onDeleteEventClicked);
    connect(ui->pushButtonAnalyze, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);

    onDateChanged(currentDate); // стартовая загрузка
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onAddEventClicked()
{
    EventDialog dialog(this);
    dialog.setEasterEnabled(m_easterEnabled);      // ✅ прокидываем состояние пасхального режима

    if (dialog.exec() == QDialog::Accepted) {
        Event e;
        e.id = QUuid::createUuid();                // ✅ стабильный идентификатор
        e.title = dialog.getTitle();
        e.start = dialog.getStartTime();
        e.end = dialog.getEndTime();
        e.tag = dialog.getTag();
        e.description = dialog.getDescription();

        eventsByDate[currentDate].append(e);
        saveEventsForDate(currentDate);
        rebuildEventList();
    }
}

void MainWindow::onEditEventClicked()
{
    QListWidgetItem *item = ui->listWidgetEvents->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Нет выбора", "Выберите событие для редактирования.");
        return;
    }

    // ✅ Получаем id из выбранного элемента списка
    const QUuid id = QUuid::fromString(item->data(Qt::UserRole).toString());
    if (id.isNull()) return;

    int index = findEventIndexById(id);
    if (index < 0 || index >= eventsByDate[currentDate].size())
        return;

    Event &e = eventsByDate[currentDate][index];
    EventDialog dialog(this);
    dialog.setEasterEnabled(m_easterEnabled);      // ✅ пасхальный режим в диалог
    dialog.setWindowTitle("Редактирование события");
    dialog.setTitle(e.title);
    dialog.setStartTime(e.start);
    dialog.setEndTime(e.end);
    dialog.setTag(e.tag);
    dialog.setDescription(e.description);

    if (dialog.exec() == QDialog::Accepted) {
        // Сохраняем тот же id
        e.title = dialog.getTitle();
        e.start = dialog.getStartTime();
        e.end = dialog.getEndTime();
        e.tag = dialog.getTag();
        e.description = dialog.getDescription();

        saveEventsForDate(currentDate);
        rebuildEventList();
    }
}

void MainWindow::onDeleteEventClicked()
{
    QListWidgetItem *item = ui->listWidgetEvents->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Нет выбора", "Выберите событие для удаления.");
        return;
    }

    // ✅ Удаляем по id
    const QUuid id = QUuid::fromString(item->data(Qt::UserRole).toString());
    if (id.isNull()) return;

    int index = findEventIndexById(id);
    if (index >= 0 && index < eventsByDate[currentDate].size()) {
        eventsByDate[currentDate].removeAt(index);
        saveEventsForDate(currentDate);
        rebuildEventList();
    }
}

void MainWindow::onDateChanged(const QDate &date)
{
    currentDate = date;
    loadEventsForDate(date);
    rebuildEventList();
}

void MainWindow::rebuildEventList()
{
    ui->listWidgetEvents->clear();
    QVector<Event> &events = eventsByDate[currentDate];

    std::sort(events.begin(), events.end(), [](const Event &a, const Event &b) {
        // если время невалидно, сортируем по названию, иначе по времени начала
        if (!a.start.isValid() || !b.start.isValid())
            return a.title.toLower() < b.title.toLower();
        return a.start < b.start;
    });

    for (const Event &e : events) {
        auto *item = new QListWidgetItem(e.toDisplayString());
        item->setData(Qt::UserRole, e.id.toString(QUuid::WithoutBraces)); // ✅ храним id в item
        ui->listWidgetEvents->addItem(item);
    }
}

void MainWindow::loadEventsForDate(const QDate &date)
{
    const QString filename = eventsFileForDate(date);
    QFile file(filename);

    eventsByDate[date].clear();

    if (!file.exists())
        return;

    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Ошибка чтения",
                             "Не удалось открыть файл: " + filename + "\n" + file.errorString());
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, "Ошибка JSON",
                             "Некорректный JSON в файле: " + filename + "\n" + err.errorString());
        return;
    }
    if (!doc.isArray()) {
        QMessageBox::warning(this, "Ошибка JSON",
                             "Ожидался массив событий в файле: " + filename);
        return;
    }

    bool generatedIds = false;

    const QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        const QJsonObject obj = val.toObject();
        Event e;

        // ✅ обратносовместимая загрузка id
        const QString idStr = obj.value("id").toString();
        e.id = QUuid::fromString(idStr);
        if (!obj.contains("id") || e.id.isNull()) {
            e.id = QUuid::createUuid();
            generatedIds = true;
        }

        e.title = obj.value("title").toString();
        e.start = QTime::fromString(obj.value("start").toString(), "HH:mm");
        e.end   = QTime::fromString(obj.value("end").toString(),   "HH:mm");
        e.tag = obj.value("tag").toString();
        e.description = obj.value("description").toString();
        eventsByDate[date].append(e);
    }

    // Если в файле не было id — пересохраним уже с id
    if (generatedIds) {
        saveEventsForDate(date);
    }
}

void MainWindow::saveEventsForDate(const QDate &date)
{
    const QString filename = eventsFileForDate(date);
    QDir().mkpath(QFileInfo(filename).absolutePath()); // гарантируем наличие папки

    QJsonArray arr;
    for (const Event &e : eventsByDate[date]) {
        QJsonObject obj;
        obj["id"]    = e.id.toString(QUuid::WithoutBraces);  // ✅ сохраняем id
        obj["title"] = e.title;
        obj["start"] = e.start.toString("HH:mm");
        obj["end"]   = e.end.toString("HH:mm");
        obj["tag"]   = e.tag;
        obj["description"] = e.description;
        arr.append(obj);
    }

    QSaveFile file(filename);               // атомарная запись
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Ошибка записи",
                             "Не удалось открыть файл для записи: " + filename + "\n" + file.errorString());
        return;
    }

    const QJsonDocument doc(arr);
    const QByteArray bytes = doc.toJson(QJsonDocument::Indented);

    if (file.write(bytes) != bytes.size()) {
        QMessageBox::warning(this, "Ошибка записи",
                             "Не удалось записать все данные в файл: " + filename);
        return;
    }

    if (!file.commit()) {
        QMessageBox::warning(this, "Ошибка сохранения",
                             "Не удалось завершить запись файла: " + filename + "\n" + file.errorString());
        return;
    }
}

void MainWindow::onAnalyzeClicked()
{
    AnalysisDialog dialog(this);
    dialog.setEasterEnabled(m_easterEnabled);      // ✅ пасхальный режим — в анализ
    dialog.exec();
}

// --- Приватный метод класса: поиск события по id ---
int MainWindow::findEventIndexById(const QUuid& id) const
{
    const auto it = eventsByDate.find(currentDate);
    if (it == eventsByDate.end()) return -1;
    const QVector<Event>& events = it.value();
    for (int i = 0; i < events.size(); ++i) {
        if (events[i].id == id) return i;
    }
    return -1;
}
