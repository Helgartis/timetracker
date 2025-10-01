#include "analysisdialog.h"
#include "ui_analysisdialog.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QTime>
#include <QDebug>
#include <QStandardPaths>
#include <QFileInfo>
#include <QRegularExpression>
#include <QHeaderView>

AnalysisDialog::AnalysisDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AnalysisDialog)
{
    ui->setupUi(this);

    // --- Настройка таблицы: заголовки и авто-растяжение столбцов ---
    if (ui->tableWidgetSummary->columnCount() < 2)
        ui->tableWidgetSummary->setColumnCount(2);
    ui->tableWidgetSummary->setHorizontalHeaderLabels(
        { tr("Тег"), tr("Доля времени (%)") });

    // Первая колонка подстраивается под содержимое (теги разной длины),
    // вторая — растягивается, чтобы заголовок и длинные фразы влезали.
    auto *hh = ui->tableWidgetSummary->horizontalHeader();
    hh->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(1, QHeaderView::Stretch);

    // Высота строк под содержание (на случай переноса строк).
    auto *vh = ui->tableWidgetSummary->verticalHeader();
    vh->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Не обрезаем текст троеточием и позволяем перенос строк.
    ui->tableWidgetSummary->setTextElideMode(Qt::ElideNone);
    ui->tableWidgetSummary->setWordWrap(true);

    // Установка текущих дат
    ui->dateEditSingle->setDate(QDate::currentDate());
    ui->dateEditFrom->setDate(QDate::currentDate().addDays(-7));
    ui->dateEditTo->setDate(QDate::currentDate());

    connect(ui->pushButtonAnalyze, &QPushButton::clicked,
            this, &AnalysisDialog::onAnalyzeClicked);
}

AnalysisDialog::~AnalysisDialog()
{
    delete ui;
}

void AnalysisDialog::onAnalyzeClicked()
{
    QDate from, to;
    if (ui->radioButtonSingleDate->isChecked()) {
        from = to = ui->dateEditSingle->date();
    } else {
        from = ui->dateEditFrom->date();
        to   = ui->dateEditTo->date();
        if (from > to) std::swap(from, to);
    }

    const QMap<QString, int> durations = loadDataByTags(from, to);

    int totalMinutes = 0;
    for (int v : durations.values())
        totalMinutes += v;

    displaySummaryTable(durations, totalMinutes);
}

QMap<QString, int> AnalysisDialog::loadDataByTags(const QDate &from, const QDate &to)
{
    QMap<QString, int> tagDurations;

    // Базовые каталоги: 1) системный AppDataLocation (как при сохранении), 2) относительный ./data (совместимость)
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString fallbackDir = QDir::current().filePath(QStringLiteral("data"));

    auto filePathForDate = [&](const QDate& d) -> QString {
        const QString baseName  = d.toString("yyyy-MM-dd") + ".json";
        const QString primary   = QDir(appDataDir).filePath(baseName);
        if (QFileInfo::exists(primary))
            return primary;
        const QString secondary = QDir(fallbackDir).filePath(baseName);
        return secondary; // может и не существовать — проверим при открытии
    };

    QDate date = from;
    while (date <= to) {
        const QString fileName = filePathForDate(date);
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            date = date.addDays(1);
            continue;
        }

        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();
        if (err.error != QJsonParseError::NoError) {
            date = date.addDays(1);
            continue;
        }

        // Поддерживаем два формата: массив верхнего уровня ИЛИ объект с массивом "events"
        QJsonArray eventsArray;
        if (doc.isArray()) {
            eventsArray = doc.array();
        } else if (doc.isObject()) {
            eventsArray = doc.object().value(QStringLiteral("events")).toArray();
        } else {
            date = date.addDays(1);
            continue;
        }

        for (const QJsonValue &val : eventsArray) {
            if (!val.isObject())
                continue;

            const QJsonObject obj = val.toObject();

            const QString rawTag = obj.value(QStringLiteral("tag")).toString().trimmed();
            const QString tag = rawTag.isEmpty() ? QStringLiteral("Без тега") : rawTag;

            const QTime start = QTime::fromString(obj.value(QStringLiteral("start")).toString(), QStringLiteral("HH:mm"));
            const QTime end   = QTime::fromString(obj.value(QStringLiteral("end")).toString(),   QStringLiteral("HH:mm"));
            if (!start.isValid() || !end.isValid())
                continue;

            int duration = start.secsTo(end) / 60;
            if (duration < 0)
                duration += 24 * 60; // переход через полночь

            tagDurations[tag] += duration;
        }

        date = date.addDays(1);
    }

    return tagDurations;
}

void AnalysisDialog::displaySummaryTable(const QMap<QString, int> &durations, int totalMinutes)
{
    // Таблица уже сконфигурирована в конструкторе; здесь — только данные.
    ui->tableWidgetSummary->clearContents();
    ui->tableWidgetSummary->setRowCount(0);

    // ===== Вспомогательные функции для "ассемблер"-тегов =====
    auto containsAssembler = [](const QString& s) -> bool {
        const QString t = s.toLower();
        return t.contains(QStringLiteral("ассембл"))
               || t.contains(QStringLiteral("асембл"))
               || t.contains(QStringLiteral("assembler"));
    };

    auto isResitPrepTag = [&](const QString& s) -> bool {
        const QString t = s.toLower().trimmed();
        // Уберём возможный числовой префикс "3." / "3)" и т.п.
        QString norm = t;
        norm.remove(QRegularExpression(QStringLiteral(R"(^\s*\d+[\.\)]\s*)")));
        return norm.contains(QStringLiteral("готов"))
               && norm.contains(QStringLiteral("пересдач"))
               && containsAssembler(norm);
    };

    // Суммарная доля всех «ассемблер-тегов» (для глобальной пасхалки)
    int assemblerMinutes = 0;
    for (auto it = durations.begin(); it != durations.end(); ++it) {
        if (containsAssembler(it.key()))
            assemblerMinutes += it.value();
    }
    const double assemblerPercent = (totalMinutes > 0)
                                        ? (assemblerMinutes * 100.0 / totalMinutes)
                                        : 0.0;
    // ==========================================================

    int row = 0;
    for (auto it = durations.begin(); it != durations.end(); ++it) {
        const QString &tag = it.key();
        const int minutes  = it.value();
        const double percent = (totalMinutes > 0) ? (minutes * 100.0 / totalMinutes) : 0.0;

        // Базовый текст — числовой процент
        QString percentText = QString::number(percent, 'f', 1);

        // Пасхальные подмены — ТОЛЬКО если режим включён
        if (m_easterEnabled) {
            // Приоритет: сперва спец-тег «готовиться к пересдаче по ассемблеру» (>30%),
            // затем — глобальная пасхалка по всем ассемблер-тегам (>70%).
            if (isResitPrepTag(tag) && percent > 30.0) {
                percentText = QStringLiteral(" Мальчик.... который... выжил... пришёл... умереть.... ");
            } else if (assemblerPercent > 70.0) {
                percentText = QStringLiteral("Это грустно. Иди поспи");
            }
        }

        ui->tableWidgetSummary->insertRow(row);

        auto *tagItem = new QTableWidgetItem(tag);
        tagItem->setToolTip(tag); // если не влезет — увидим полный текст
        ui->tableWidgetSummary->setItem(row, 0, tagItem);

        auto *pctItem = new QTableWidgetItem(percentText);
        pctItem->setToolTip(percentText);
        ui->tableWidgetSummary->setItem(row, 1, pctItem);

        ++row;
    }

    // Подгоняем размеры после заполнения (на случай длинных значений)
    ui->tableWidgetSummary->resizeRowsToContents();
    // Для первой колонки (ResizeToContents) ширина пересчитается автоматически;
    // вторая растягивается на оставшееся место (Stretch).
}
