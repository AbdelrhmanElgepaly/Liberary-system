#pragma once
#include "Page.h"

class QLabel;
class QTableWidget;

// Opening page: welcome banner, library statistics, recent activity and overdue books.
class DashboardPage : public Page {
    Q_OBJECT
public:
    explicit DashboardPage(LibraryService& service, const QString& userName, QWidget* parent = nullptr);
    void refresh() override;

private:
    QWidget* makeStatCard(const QString& iconName, const QString& colour, const QString& caption, QLabel** valueOut);
    void fillLoans(QTableWidget* table, const QVector<Loan>& loans, bool showStatus);

    QLabel *m_titles, *m_copies, *m_students, *m_borrowed, *m_overdue;
    QTableWidget *m_recent, *m_overdueTable;
    QLabel *m_overdueEmpty;
};
