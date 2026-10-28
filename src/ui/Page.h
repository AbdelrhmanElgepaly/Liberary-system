#pragma once
#include "services/LibraryService.h"
#include "ui/Toast.h"

#include <QWidget>

class QVBoxLayout;

// Base class of every page in the main window.
class Page : public QWidget {
    Q_OBJECT
public:
    explicit Page(LibraryService& service, QWidget* parent = nullptr) : QWidget(parent), m_svc(service) {}

    // Reloads the data shown by the page (called every time the page is opened).
    virtual void refresh() = 0;

signals:
    void notify(const QString& message, int kind);   // kind = Toast::Kind

protected:
    // Page title + subtitle (and an optional widget on the right, e.g. the main action button).
    QWidget* makeHeader(const QString& title, const QString& subtitle, QWidget* rightWidget = nullptr);
    QVBoxLayout* makePageLayout();

    LibraryService& m_svc;
};
