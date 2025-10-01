#ifndef EVENTDIALOG_H
#define EVENTDIALOG_H

#include <QDialog>
#include <QTime>
#include <QString>
#include <QStringList>

namespace Ui {
class EventDialog;
}

class EventDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EventDialog(QWidget *parent = nullptr);
    ~EventDialog();

    // Получение значений из формы
    QString getTitle() const;
    QTime   getStartTime() const;
    QTime   getEndTime() const;
    QString getTag() const;
    QString getDescription() const;

    // Установка значений в форму
    void setTitle(const QString &);
    void setStartTime(const QTime &);
    void setEndTime(const QTime &);
    void setTag(const QString &);
    void setDescription(const QString &);

    // Управление «пасхальным режимом»: при false — extraTags отключены
    void setEasterEnabled(bool on);
    bool  isEasterEnabled() const { return m_easterEnabled; }

protected:
    // ✅ Переопределяем accept() для валидации перед закрытием диалога
    void accept() override;

private:
    Ui::EventDialog *ui;

    // Состояние «пасхального режима» (вкл/выкл)
    bool m_easterEnabled = false;

    // Пересобирает список тегов в комбобоксе с учётом m_easterEnabled
    void rebuildTagList();
};

#endif // EVENTDIALOG_H
