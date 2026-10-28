#pragma once
#include <QDate>
#include <QString>
#include <optional>

// One borrowing transaction (one row of the Loans table, joined with book/student names).
class Loan {
public:
    enum class Status { OnTime, DueToday, Overdue, Returned };

    Loan() = default;   // needed by QVector::resize
    Loan(int loanId, const QString& bookId, const QString& bookTitle,
         const QString& studentId, const QString& studentName,
         const QDate& issueDate, const QDate& dueDate, std::optional<QDate> returnDate)
        : m_loanId(loanId), m_bookId(bookId), m_bookTitle(bookTitle), m_studentId(studentId),
          m_studentName(studentName), m_issueDate(issueDate), m_dueDate(dueDate),
          m_returnDate(returnDate) {}

    int loanId() const { return m_loanId; }
    const QString& bookId() const { return m_bookId; }
    const QString& bookTitle() const { return m_bookTitle; }
    const QString& studentId() const { return m_studentId; }
    const QString& studentName() const { return m_studentName; }
    const QDate& issueDate() const { return m_issueDate; }
    const QDate& dueDate() const { return m_dueDate; }
    const std::optional<QDate>& returnDate() const { return m_returnDate; }

    bool isReturned() const { return m_returnDate.has_value(); }

    Status status(const QDate& today) const {
        if (isReturned()) return Status::Returned;
        if (today > m_dueDate) return Status::Overdue;
        if (today == m_dueDate) return Status::DueToday;
        return Status::OnTime;
    }

    // Whole days past the due date (0 if not overdue).
    int daysOverdue(const QDate& today) const {
        return status(today) == Status::Overdue ? int(m_dueDate.daysTo(today)) : 0;
    }

    QString statusText(const QDate& today) const {
        switch (status(today)) {
        case Status::Returned: return "Returned";
        case Status::Overdue: {
            int d = daysOverdue(today);
            return QString("Overdue (%1 day%2)").arg(d).arg(d == 1 ? "" : "s");
        }
        case Status::DueToday: return "Due today";
        case Status::OnTime: break;
        }
        return "On time";
    }

private:
    int m_loanId = 0;
    QString m_bookId, m_bookTitle, m_studentId, m_studentName;
    QDate m_issueDate, m_dueDate;
    std::optional<QDate> m_returnDate;
};
