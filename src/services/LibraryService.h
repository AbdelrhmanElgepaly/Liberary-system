#pragma once
#include "Result.h"
#include "db/BookRepository.h"
#include "db/DatabaseManager.h"
#include "db/LoanRepository.h"
#include "db/StudentRepository.h"

struct LibraryStats {
    int titles = 0;
    int copies = 0;
    int students = 0;
    int activeLoans = 0;
    int overdueLoans = 0;
};

// What would be lost if a book / student is deleted.
struct DeleteInfo {
    int openLoans = 0;
    int totalLoans = 0;
};

// Business rules of the library. The user interface talks only to this class.
// Mutating operations return a Result (never throw); read operations may throw DbException.
class LibraryService {
public:
    static constexpr int kLoanPeriodDays = 7;

    explicit LibraryService(DatabaseManager& db);

    // ---- books ----
    QVector<Book> books(const QString& text = QString(), BookField field = BookField::Any);
    QVector<Book> availableBooks();
    QStringList genres();
    Result addBook(const Book& book);
    Result updateBook(const Book& book);
    Result changeCopies(const QString& bookId, int delta);     // delta > 0 add, < 0 remove
    DeleteInfo bookDeleteInfo(const QString& bookId);
    Result deleteBook(const QString& bookId, bool alsoDeleteHistory);

    // ---- students ----
    QVector<Student> students(const QString& text = QString());
    QHash<QString, int> openLoanCountByStudent();
    Result addStudent(const Student& student);
    Result updateStudent(const Student& student);
    DeleteInfo studentDeleteInfo(const QString& studentId);
    Result deleteStudent(const QString& studentId, bool alsoDeleteHistory);

    // ---- borrowing ----
    Result borrowBook(const QString& bookId, const QString& studentId);
    Result returnBook(int loanId);

    // ---- reports ----
    QVector<Loan> activeLoans(const QString& filterText = QString());
    QVector<Loan> overdueLoans();
    QVector<Loan> loanHistory();
    QVector<Loan> recentLoans(int maxRows);
    LibraryStats stats();

    // Fills an empty database with sample data (used by the --demo switch).
    bool seedDemoData();

    static QDate today() { return QDate::currentDate(); }

private:
    DatabaseManager& m_db;
    BookRepository m_books;
    StudentRepository m_students;
    LoanRepository m_loans;
};
