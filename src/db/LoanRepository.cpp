#include "LoanRepository.h"

namespace {
// Access requires parentheses around the first join when two tables are joined.
const char* kSelect =
    "SELECT l.LoanID, l.BookID, b.Title, l.StudentID, s.FullName, l.IssueDate, l.DueDate, l.ReturnDate "
    "FROM (Loans AS l INNER JOIN Books AS b ON l.BookID = b.BookID) "
    "INNER JOIN Students AS s ON l.StudentID = s.StudentID ";
}

Loan LoanRepository::fromRow(const QSqlQuery& q) {
    std::optional<QDate> returned;
    if (!q.value(7).isNull()) returned = q.value(7).toDate();
    return Loan(q.value(0).toInt(), q.value(1).toString(), q.value(2).toString(),
                q.value(3).toString(), q.value(4).toString(), q.value(5).toDate(),
                q.value(6).toDate(), returned);
}

QVector<Loan> LoanRepository::run(const QString& where, const QString& order, const QVariantList& params) {
    QString sql = kSelect;
    if (!where.isEmpty()) sql += "WHERE " + where + " ";
    sql += "ORDER BY " + order;
    QSqlQuery q = m_db.exec(sql, params);
    QVector<Loan> out;
    while (q.next()) out.append(fromRow(q));
    return out;
}

int LoanRepository::scalar(const QString& sql, const QVariantList& params) {
    QSqlQuery q = m_db.exec(sql, params);
    return q.next() ? q.value(0).toInt() : 0;
}

QVector<Loan> LoanRepository::active(const QString& filterText) {
    const QString t = filterText.trimmed();
    if (t.isEmpty()) return run("l.ReturnDate IS NULL", "l.DueDate, l.LoanID");
    const QString like = "%" + t + "%";
    return run("l.ReturnDate IS NULL AND (l.BookID LIKE ? OR b.Title LIKE ? OR l.StudentID LIKE ? OR s.FullName LIKE ?)",
               "l.DueDate, l.LoanID", {like, like, like, like});
}

QVector<Loan> LoanRepository::overdue(const QDate& today) {
    return run("l.ReturnDate IS NULL AND l.DueDate < ?", "l.DueDate, l.LoanID", {today});
}

QVector<Loan> LoanRepository::history() {
    return run(QString(), "l.LoanID DESC");
}

QVector<Loan> LoanRepository::recent(int maxRows) {
    // "TOP n" exists only in Access, so the limit is applied here instead.
    QVector<Loan> all = run(QString(), "l.LoanID DESC");
    if (all.size() > maxRows) all.resize(maxRows);
    return all;
}

std::optional<Loan> LoanRepository::findOpen(int loanId) {
    const QVector<Loan> rows = run("l.LoanID = ? AND l.ReturnDate IS NULL", "l.LoanID", {loanId});
    if (rows.isEmpty()) return std::nullopt;
    return rows.first();
}

bool LoanRepository::hasOpenLoan(const QString& bookId, const QString& studentId) {
    return scalar("SELECT COUNT(*) FROM Loans WHERE BookID = ? AND StudentID = ? AND ReturnDate IS NULL",
                  {bookId, studentId}) > 0;
}

int LoanRepository::openLoansOfBook(const QString& id) {
    return scalar("SELECT COUNT(*) FROM Loans WHERE BookID = ? AND ReturnDate IS NULL", {id});
}
int LoanRepository::openLoansOfStudent(const QString& id) {
    return scalar("SELECT COUNT(*) FROM Loans WHERE StudentID = ? AND ReturnDate IS NULL", {id});
}
int LoanRepository::totalLoansOfBook(const QString& id) {
    return scalar("SELECT COUNT(*) FROM Loans WHERE BookID = ?", {id});
}
int LoanRepository::totalLoansOfStudent(const QString& id) {
    return scalar("SELECT COUNT(*) FROM Loans WHERE StudentID = ?", {id});
}

QHash<QString, int> LoanRepository::openLoanCountByStudent() {
    QSqlQuery q = m_db.exec("SELECT StudentID, COUNT(*) FROM Loans WHERE ReturnDate IS NULL GROUP BY StudentID");
    QHash<QString, int> out;
    while (q.next()) out.insert(q.value(0).toString().toLower(), q.value(1).toInt());
    return out;
}

void LoanRepository::insert(const QString& bookId, const QString& studentId, const QDate& issued, const QDate& due) {
    m_db.exec("INSERT INTO Loans (BookID, StudentID, IssueDate, DueDate) VALUES (?, ?, ?, ?)",
              {bookId, studentId, issued, due});
}

bool LoanRepository::markReturned(int loanId, const QDate& returnedOn) {
    QSqlQuery q = m_db.exec("UPDATE Loans SET ReturnDate = ? WHERE LoanID = ? AND ReturnDate IS NULL",
                            {returnedOn, loanId});
    return q.numRowsAffected() == 1;
}

void LoanRepository::removeForBook(const QString& bookId) {
    m_db.exec("DELETE FROM Loans WHERE BookID = ?", {bookId});
}
void LoanRepository::removeForStudent(const QString& studentId) {
    m_db.exec("DELETE FROM Loans WHERE StudentID = ?", {studentId});
}

int LoanRepository::activeCount() {
    return scalar("SELECT COUNT(*) FROM Loans WHERE ReturnDate IS NULL");
}
int LoanRepository::overdueCount(const QDate& today) {
    return scalar("SELECT COUNT(*) FROM Loans WHERE ReturnDate IS NULL AND DueDate < ?", {today});
}
