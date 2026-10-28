#pragma once
#include "DatabaseManager.h"
#include "models/Loan.h"
#include <QHash>
#include <QVector>
#include <optional>

// All SQL that touches the Loans table (borrowing / returning transactions).
class LoanRepository {
public:
    explicit LoanRepository(DatabaseManager& db) : m_db(db) {}

    QVector<Loan> active(const QString& filterText = QString());   // not returned yet
    QVector<Loan> overdue(const QDate& today);
    QVector<Loan> history();                                        // every loan, newest first
    QVector<Loan> recent(int maxRows);
    std::optional<Loan> findOpen(int loanId);

    bool hasOpenLoan(const QString& bookId, const QString& studentId);
    int openLoansOfBook(const QString& bookId);
    int openLoansOfStudent(const QString& studentId);
    int totalLoansOfBook(const QString& bookId);
    int totalLoansOfStudent(const QString& studentId);
    QHash<QString, int> openLoanCountByStudent();

    void insert(const QString& bookId, const QString& studentId, const QDate& issued, const QDate& due);
    bool markReturned(int loanId, const QDate& returnedOn);   // false if it was not an open loan
    void removeForBook(const QString& bookId);
    void removeForStudent(const QString& studentId);

    int activeCount();
    int overdueCount(const QDate& today);

private:
    static Loan fromRow(const QSqlQuery& q);
    QVector<Loan> run(const QString& where, const QString& order, const QVariantList& params = {});
    int scalar(const QString& sql, const QVariantList& params = {});
    DatabaseManager& m_db;
};
