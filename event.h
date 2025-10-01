#ifndef EVENT_H
#define EVENT_H

#include <QString>
#include <QTime>
#include <QUuid>

struct Event {
    QUuid  id;           // ✅ стабильный идентификатор события
    QString title;
    QTime   start;
    QTime   end;
    QString tag;
    QString description;

    QString toDisplayString() const {
        return QString("%1 — %2 | %3 [%4]")
            .arg(start.isValid() ? start.toString("HH:mm") : "--:--")
            .arg(end.isValid()   ? end.toString("HH:mm")   : "--:--")
            .arg(title)
            .arg(tag);
    }
};

Q_DECLARE_METATYPE(Event)

#endif // EVENT_H
