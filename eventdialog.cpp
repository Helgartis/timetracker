#include "eventdialog.h"
#include "ui_eventdialog.h"

#include <QStringList>
#include <algorithm>
#include <QMessageBox>
#include <QComboBox>

namespace {
// Пасхальные теги — выключаются чекбоксом на главном окне
const QStringList kExtraTags = {
    QStringLiteral("Прокрастинировать над ассемблером"),
    QStringLiteral("Плакать над ассемблером"),
    QStringLiteral("Готовиться к пересдаче по ассемблеру"),
    QStringLiteral("Пить вкусный бабл ти перед ассемблером"),
    QStringLiteral("Истерически смеяться изза ассемблера"),
    QStringLiteral("Смотреть страшные сны про ассемблер"),
    QStringLiteral("На коленях просить не отчислять изза ассемблера"),
    QStringLiteral("Ловить инсульт во аремя пересдачи по ассемблеру"),
    QStringLiteral("Нервно курить перед пересдачей по ассемблеру"),
    QStringLiteral("Грустно пить энергетик вместе с Ромой")
};

// Снимем базовые теги из .ui при первом создании диалога,
// чтобы не хардкодить их здесь.
QStringList g_initialBaseTags;
bool g_baseTagsCaptured = false;

static QStringList captureBaseTagsFromUi(QComboBox* box) {
    QStringList out;
    const int n = box->count();
    out.reserve(n);
    for (int i = 0; i < n; ++i) out << box->itemText(i);
    out.removeAll(QString());     // на всякий
    out.removeDuplicates();
    return out;
}
} // namespace

EventDialog::EventDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::EventDialog)
{
    ui->setupUi(this);

    // Один раз за время жизни приложения запомним базовый набор тегов из .ui
    if (!g_baseTagsCaptured) {
        g_initialBaseTags = captureBaseTagsFromUi(ui->comboBoxTags);
        g_baseTagsCaptured = true;
    }

    // Построим список тегов с учётом текущего состояния m_easterEnabled (по умолчанию false)
    rebuildTagList();

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

EventDialog::~EventDialog() {
    delete ui;
}

void EventDialog::setEasterEnabled(bool on)
{
    if (m_easterEnabled == on) return;
    m_easterEnabled = on;
    rebuildTagList();
}

void EventDialog::rebuildTagList()
{
    // Сохраним текущий текст, чтобы попытаться восстановить выбор
    const QString current = ui->comboBoxTags->currentText();

    // Пересобираем: базовые из .ui + опционально пасхальные
    QStringList tags = g_initialBaseTags;

    if (m_easterEnabled) {
        for (const auto& t : kExtraTags) {
            if (!tags.contains(t)) tags << t;
        }
    }

    tags.removeAll(QString());
    tags.removeDuplicates();
    std::sort(tags.begin(), tags.end(), [](const QString& a, const QString& b){
        return a.localeAwareCompare(b) < 0;
    });

    // Обновляем комбобокс
    ui->comboBoxTags->clear();
    ui->comboBoxTags->addItems(tags);

    // Вернуть предыдущий выбор, если возможно
    if (!current.isEmpty()) {
        int idx = ui->comboBoxTags->findText(current, Qt::MatchFixedString);
        if (idx >= 0) ui->comboBoxTags->setCurrentIndex(idx);
        else ui->comboBoxTags->setEditText(current); // если комбобокс редактируемый
    }
}

// --- Геттеры ---
QString EventDialog::getTitle() const {
    return ui->lineEditTitle->text();
}

QTime EventDialog::getStartTime() const {
    return ui->timeEditStart->time();
}

QTime EventDialog::getEndTime() const {
    return ui->timeEditEnd->time();
}

QString EventDialog::getTag() const {
    return ui->comboBoxTags->currentText();
}

QString EventDialog::getDescription() const {
    return ui->textEditDescription->toPlainText();
}

// --- Сеттеры ---
void EventDialog::setTitle(const QString &text) {
    ui->lineEditTitle->setText(text);
}

void EventDialog::setStartTime(const QTime &time) {
    ui->timeEditStart->setTime(time);
}

void EventDialog::setEndTime(const QTime &time) {
    ui->timeEditEnd->setTime(time);
}

void EventDialog::setTag(const QString &tag) {
    int index = ui->comboBoxTags->findText(tag);
    if (index >= 0)
        ui->comboBoxTags->setCurrentIndex(index);
    else
        ui->comboBoxTags->setEditText(tag);
}

void EventDialog::setDescription(const QString &text) {
    ui->textEditDescription->setPlainText(text);
}

// --- Валидация перед закрытием диалога ---
void EventDialog::accept()
{
    const QString title = ui->lineEditTitle->text().trimmed();
    const QTime start   = ui->timeEditStart->time();
    const QTime end     = ui->timeEditEnd->time();

    // 1) Заголовок обязателен
    if (title.isEmpty()) {
        QMessageBox::warning(this, tr("Пустой заголовок"),
                             tr("Пожалуйста, укажите название события."));
        ui->lineEditTitle->setFocus();
        return; // НЕ закрываем диалог
    }

    // 2) Валидные времена
    if (!start.isValid() || !end.isValid()) {
        QMessageBox::warning(this, tr("Некорректное время"),
                             tr("Проверьте время начала и окончания."));
        return;
    }

    // 3) Интервал: разрешаем end > start (обычно) или «через полночь» (end < start).
    // Ровно равные значения запрещаем (0 минут).
    if (start == end) {
        QMessageBox::warning(this, tr("Нулевой интервал"),
                             tr("Время начала и окончания совпадают. Укажите ненулевую длительность."));
        ui->timeEditEnd->setFocus();
        return;
    }
    // end < start трактуется как «через полночь» — это ОК, ничего не делаем.
    // end > start — обычный случай — тоже ОК.

    // Если все проверки прошли — закрываем диалог как Accepted
    QDialog::accept();
}
