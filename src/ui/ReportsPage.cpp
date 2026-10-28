#include "ReportsPage.h"
#include "UiUtil.h"

#include <QButtonGroup>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
const QStringList kHeaders = {"Loan #", "Book", "Student", "Issued", "Due", "Returned", "Status"};
const QStringList kCsvHeaders = {"LoanID", "BookID", "Title", "StudentID", "Student", "IssueDate", "DueDate",
                                 "ReturnDate", "Status"};

QString csvField(QString s) {
    s.replace('"', "\"\"");
    return '"' + s + '"';
}
}  // namespace

ReportsPage::ReportsPage(LibraryService& service, QWidget* parent) : Page(service, parent) {
    auto* layout = makePageLayout();
    auto* exportButton = ui::makeButton("Export CSV", "export");
    layout->addWidget(makeHeader("Reports", "Borrowed, overdue and complete borrowing history.", exportButton));

    auto* card = ui::makeCard();
    auto* cl = new QVBoxLayout(card);
    cl->setContentsMargins(20, 18, 20, 18);
    cl->setSpacing(14);

    // segmented control
    auto* seg = new QHBoxLayout;
    seg->setSpacing(0);
    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);
    const char* names[] = {"Currently borrowed", "Overdue", "Full history"};
    for (int i = 0; i < 3; ++i) {
        auto* b = new QPushButton(names[i]);
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setProperty("segment", i == 0 ? "first" : (i == 2 ? "last" : "middle"));
        m_group->addButton(b, i);
        seg->addWidget(b);
    }
    m_group->button(Borrowed)->setChecked(true);
    seg->addStretch();
    m_summary = ui::makeLabel(QString(), "mutedLabel");
    seg->addWidget(m_summary);
    cl->addLayout(seg);

    m_table = ui::makeTable(kHeaders, 0, Qt::DescendingOrder);
    m_table->setColumnWidth(0, 70);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(2, 225);
    m_table->setColumnWidth(3, 112);
    m_table->setColumnWidth(4, 112);
    m_table->setColumnWidth(5, 112);
    m_table->setColumnWidth(6, 150);
    cl->addWidget(m_table, 1);
    layout->addWidget(card, 1);

    connect(m_group, &QButtonGroup::idClicked, this, [this] { reload(); });
    connect(exportButton, &QPushButton::clicked, this, &ReportsPage::exportCsv);
}

void ReportsPage::refresh() { reload(); }

void ReportsPage::reload() {
    const int which = m_group->checkedId();
    if (!ui::guard(this, [&] {
            if (which == Overdue) m_loans = m_svc.overdueLoans();
            else if (which == History) m_loans = m_svc.loanHistory();
            else m_loans = m_svc.activeLoans();
        }))
        return;

    const QDate today = LibraryService::today();
    m_table->setSortingEnabled(false);
    m_table->setRowCount(m_loans.size());
    for (int r = 0; r < m_loans.size(); ++r) {
        const Loan& l = m_loans[r];
        m_table->setItem(r, 0, ui::makeNumberCell(l.loanId()));
        auto* book = ui::makeCell(QString("%1  -  %2").arg(l.bookId(), l.bookTitle()));
        m_table->setItem(r, 1, book);
        m_table->setItem(r, 2, ui::makeCell(QString("%1  (%2)").arg(l.studentName(), l.studentId())));
        m_table->setItem(r, 3, ui::makeDateCell(l.issueDate()));
        m_table->setItem(r, 4, ui::makeDateCell(l.dueDate()));
        m_table->setItem(r, 5, l.returnDate() ? ui::makeDateCell(*l.returnDate()) : ui::makeCell("-"));
        auto* status = ui::makeCell(l.statusText(today));
        QFont f = status->font();
        f.setBold(true);
        status->setFont(f);
        switch (l.status(today)) {
        case Loan::Status::Overdue:  status->setForeground(QColor("#C0392B")); break;
        case Loan::Status::DueToday: status->setForeground(QColor("#B7791F")); break;
        case Loan::Status::Returned: status->setForeground(QColor("#7B6E62")); break;
        case Loan::Status::OnTime:   status->setForeground(QColor("#2E8B57")); break;
        }
        m_table->setItem(r, 6, status);
    }
    m_table->setSortingEnabled(true);

    const char* labels[] = {"borrowed book", "overdue book", "loan record"};
    m_summary->setText(QString("%1 %2%3").arg(m_loans.size()).arg(labels[which]).arg(m_loans.size() == 1 ? "" : "s"));
}

void ReportsPage::exportCsv() {
    if (m_loans.isEmpty()) {
        emit notify("There is nothing to export in this report.", Toast::Info);
        return;
    }
    const char* names[] = {"borrowed_books", "overdue_books", "borrowing_history"};
    const QString suggested = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                  .filePath(QString("%1_%2.csv").arg(names[m_group->checkedId()],
                                                                     QDate::currentDate().toString("yyyyMMdd")));
    const QString path = QFileDialog::getSaveFileName(this, "Export report", suggested, "CSV files (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit notify("Could not write the file: " + file.errorString(), Toast::Error);
        return;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out.setGenerateByteOrderMark(true);   // so Excel reads UTF-8 correctly
    QStringList head;
    for (const QString& h : kCsvHeaders) head << csvField(h);
    out << head.join(',') << "\r\n";

    // Dates are written as yyyy-MM-dd (sorts correctly and is understood by Excel and Power BI).
    const QDate today = LibraryService::today();
    auto iso = [](const QDate& d) { return d.toString("yyyy-MM-dd"); };
    for (const Loan& l : m_loans) {
        const QStringList row = {QString::number(l.loanId()), l.bookId(), l.bookTitle(), l.studentId(),
                                 l.studentName(), iso(l.issueDate()), iso(l.dueDate()),
                                 l.returnDate() ? iso(*l.returnDate()) : QString(), l.statusText(today)};
        QStringList quoted;
        for (const QString& f : row) quoted << csvField(f);
        out << quoted.join(',') << "\r\n";
    }
    out.flush();
    emit notify(QString("Exported %1 rows.").arg(m_loans.size()), Toast::Success);
}
