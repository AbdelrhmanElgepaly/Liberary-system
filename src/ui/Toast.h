#pragma once
#include <QFrame>
#include <QPointer>

// Small notification that fades in at the bottom-right corner of the window and disappears by itself.
class Toast : public QFrame {
    Q_OBJECT
public:
    enum Kind { Success = 0, Error = 1, Info = 2 };

    static void popup(QWidget* host, const QString& text, Kind kind = Success);

private:
    Toast(QWidget* host, const QString& text, Kind kind);
    void fadeOutAndClose();

    static QPointer<Toast> s_current;
};
