#pragma once
#include "services/LibraryService.h"

#include <QButtonGroup>
#include <QMainWindow>
#include <QVector>

class Page;
class QStackedWidget;

// Main window: dark sidebar with navigation on the left, pages on the right.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(LibraryService& service, const QString& userName, QWidget* parent = nullptr);

    void showPage(int index);

signals:
    void logoutRequested();

private slots:
    void showToast(const QString& message, int kind);

private:
    QStackedWidget* m_stack;
    QButtonGroup* m_nav;
    QVector<Page*> m_pages;
};
