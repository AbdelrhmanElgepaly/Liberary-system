#include "Toast.h"
#include "UiUtil.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

QPointer<Toast> Toast::s_current;

Toast::Toast(QWidget* host, const QString& text, Kind kind) : QFrame(host->window()) {
    setObjectName("toast");
    static const char* kinds[] = {"success", "error", "info"};
    static const char* icons[] = {"check", "cross", "info"};
    setProperty("kind", kinds[kind]);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 12, 20, 12);
    layout->setSpacing(12);

    auto* iconLabel = new QLabel(this);
    iconLabel->setPixmap(ui::iconPixmap(icons[kind], "white", 24));
    iconLabel->setFixedSize(24, 24);
    layout->addWidget(iconLabel, 0, Qt::AlignTop);

    auto* msg = new QLabel(text, this);
    msg->setObjectName("toastText");
    msg->setWordWrap(true);
    msg->setFixedWidth(330);
    layout->addWidget(msg);

    adjustSize();
    QWidget* w = host->window();
    move(w->width() - width() - 28, w->height() - height() - 28);
}

void Toast::popup(QWidget* host, const QString& text, Kind kind) {
    if (!host) return;
    if (s_current) s_current->deleteLater();
    auto* t = new Toast(host, text, kind);
    s_current = t;
    t->show();
    t->raise();

    auto* effect = new QGraphicsOpacityEffect(t);
    t->setGraphicsEffect(effect);
    auto* fadeIn = new QPropertyAnimation(effect, "opacity", t);
    fadeIn->setDuration(180);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    QTimer::singleShot(kind == Error ? 5000 : 3200, t, [t] { t->fadeOutAndClose(); });
}

void Toast::fadeOutAndClose() {
    auto* effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect());
    if (!effect) {
        deleteLater();
        return;
    }
    auto* fadeOut = new QPropertyAnimation(effect, "opacity", this);
    fadeOut->setDuration(260);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    connect(fadeOut, &QPropertyAnimation::finished, this, &QObject::deleteLater);
    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}
