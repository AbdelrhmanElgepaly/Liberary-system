#pragma once
#include "Page.h"

class QButtonGroup;
class QLabel;
class QTableWidget;

// Reports: currently borrowed, overdue and full history, with CSV export (usable from Excel / Power BI).
class ReportsPage : public Page {
    Q_OBJECT
public:
    explicit ReportsPage(LibraryService& service, QWidget* parent = nullptr);
    void refresh() override;

private slots:
    void exportCsv();

private:
    enum Report { Borrowed = 0, Overdue = 1, History = 2 };
    void reload();

    QButtonGroup* m_group;
    QTableWidget* m_table;
    QLabel* m_summary;
    QVector<Loan> m_loans;   // rows currently shown (the CSV export is built from these)
};
