#include "LibraryService.h"

namespace {
Result dbFailure(const DbException& e) {
    return Result::failure("Database error: " + e.message());
}
Result invalid(const QStringList& errors) {
    return Result::failure(errors.join("\n"));
}
}  // namespace

LibraryService::LibraryService(DatabaseManager& db)
    : m_db(db), m_books(db), m_students(db), m_loans(db) {}

// ============================================================ books
QVector<Book> LibraryService::books(const QString& text, BookField field) { return m_books.search(text, field); }
QVector<Book> LibraryService::availableBooks() { return m_books.available(); }
QStringList LibraryService::genres() { return m_books.genres(); }

Result LibraryService::addBook(const Book& book) {
    if (const QStringList errors = book.validate(); !errors.isEmpty()) return invalid(errors);
    try {
        if (m_books.find(book.id()))
            return Result::failure(QString("A book with ID \"%1\" already exists.").arg(book.id()));
        m_books.insert(book);
        return Result::success(QString("\"%1\" was added to the library.").arg(book.title()));
    } catch (const DbException& e) {
        return dbFailure(e);
    }
}

Result LibraryService::updateBook(const Book& book) {
    if (const QStringList errors = book.validate(); !errors.isEmpty()) return invalid(errors);
    try {
        if (!m_books.find(book.id())) return Result::failure("This book no longer exists.");
        m_books.update(book);
        return Result::success(QString("\"%1\" was updated.").arg(book.title()));
    } catch (const DbException& e) {
        return dbFailure(e);
    }
}

Result LibraryService::changeCopies(const QString& bookId, int delta) {
    if (delta == 0) return Result::failure("Enter a number of copies greater than zero.");
    try {
        const auto book = m_books.find(bookId);
        if (!book) return Result::failure("This book no longer exists.");
        if (delta < 0 && -delta > book->copies())
            return Result::failure(QString("Cannot remove %1 copies: only %2 available.")
                                       .arg(-delta).arg(book->copies()));
        if (delta > 0 && book->copies() + delta > Book::kMaxCopies)
            return Result::failure(QString("Copies cannot exceed %1.").arg(Book::kMaxCopies));
        m_books.changeCopies(book->id(), delta);
        return Result::success(QString("Copies of \"%1\": %2 -> %3.")
                                   .arg(book->title()).arg(book->copies()).arg(book->copies() + delta));
    } catch (const DbException& e) {
        return dbFailure(e);
    }
}

DeleteInfo LibraryService::bookDeleteInfo(const QString& bookId) {
    return {m_loans.openLoansOfBook(bookId), m_loans.totalLoansOfBook(bookId)};
}

Result LibraryService::deleteBook(const QString& bookId, bool alsoDeleteHistory) {
    try {
        const auto book = m_books.find(bookId);
        if (!book) return Result::failure("This book no longer exists.");
        const DeleteInfo info = bookDeleteInfo(book->id());
        if (info.openLoans > 0)
            return Result::failure("This book is currently borrowed and cannot be deleted. "
                                   "Return it first.");
        if (info.totalLoans > 0 && !alsoDeleteHistory)
            return Result::failure("This book has borrowing history. Confirm to delete it as well.");
        DbTransaction tx(m_db);
        if (info.totalLoans > 0) m_loans.removeForBook(book->id());
        m_books.remove(book->id());
        tx.commit();
        return Result::success(QString("\"%1\" was deleted.").arg(book->title()));
    } catch (const DbException& e) {
        return dbFailure(e);
    }
}

// ============================================================ students
QVector<Student> LibraryService::students(const QString& text) { return m_students.search(text); }
QHash<QString, int> LibraryService::openLoanCountByStudent() { return m_loans.openLoanCountByStudent(); }

Result LibraryService::addStudent(const Student& s) {
    if (const QStringList errors = s.validate(); !errors.isEmpty()) return invalid(errors);
    try {
        if (m_students.find(s.id()))
            return Result::failure(QString("A student with ID \"%1\" already exists.").arg(s.id()));
        m_students.insert(s);
        return Result::success(QString("%1 was registered.").arg(s.fullName()));
    } catch (const DbException& e) {
        return dbFailure(e);
    }
}

Result LibraryService::updateStudent(const Student& s) {
    if (const QStringList errors = s.validate(); !errors.isEmpty()) return invalid(errors);
    try {
        if (!m_students.find(s.id())) return Result::failure("This student no longer exists.");
        m_students.update(s);
        return Result::success(QString("%1 was updated.").arg(s.fullName()));
    } catch (const DbException& e) {
        return dbFailure(e);
    }
}

DeleteInfo LibraryService::studentDeleteInfo(const QString& studentId) {
    return {m_loans.openLoansOfStudent(studentId), m_loans.totalLoansOfStudent(studentId)};
}

Result LibraryService::deleteStudent(const QString& studentId, bool alsoDeleteHistory) {
    try {
        const auto student = m_students.find(studentId);
        if (!student) return Result::failure("This student no longer exists.");
        const DeleteInfo info = studentDeleteInfo(student->id());
        if (info.openLoans > 0)
            return Result::failure("This student still has borrowed books and cannot be deleted. "
                                   "Return them first.");
        if (info.totalLoans > 0 && !alsoDeleteHistory)
            return Result::failure("This student has borrowing history. Confirm to delete it as well.");
        DbTransaction tx(m_db);
        if (info.totalLoans > 0) m_loans.removeForStudent(student->id());
        m_students.remove(student->id());
        tx.commit();
        return Result::success(QString("%1 was deleted.").arg(student->fullName()));
    } catch (const DbException& e) {
        return dbFailure(e);
    }
}

// ============================================================ borrowing
Result LibraryService::borrowBook(const QString& bookId, const QString& studentId) {
    try {
        const auto book = m_books.find(bookId);
        if (!book) return Result::failure("Please choose a book from the list.");
        const auto student = m_students.find(studentId);
        if (!student) return Result::failure("Please choose a student from the list.");
        if (book->copies() <= 0)
            return Result::failure(QString("\"%1\" is not available: there are 0 copies left.").arg(book->title()));
        if (m_loans.hasOpenLoan(book->id(), student->id()))
            return Result::failure(QString("%1 already has a copy of \"%2\".")
                                       .arg(student->fullName(), book->title()));

        const QDate issued = today();
        const QDate due = issued.addDays(kLoanPeriodDays);
        DbTransaction tx(m_db);
        if (!m_books.takeOneCopy(book->id()))
            return Result::failure(QString("\"%1\" is not available: there are 0 copies left.").arg(book->title()));
        m_loans.insert(book->id(), student->id(), issued, due);
        tx.commit();
        return Result::success(QString("\"%1\" borrowed by %2. Due on %3.")
                                   .arg(book->title(), student->fullName(), due.toString("dd MMM yyyy")));
    } catch (const DbException& e) {
        return dbFailure(e);
    }
}

Result LibraryService::returnBook(int loanId) {
    try {
        const auto loan = m_loans.findOpen(loanId);
        if (!loan) return Result::failure("This loan was already returned or no longer exists.");
        DbTransaction tx(m_db);
        if (!m_loans.markReturned(loanId, today()))
            return Result::failure("This loan was already returned.");
        m_books.changeCopies(loan->bookId(), +1);
        tx.commit();
        QString msg = QString("\"%1\" returned by %2.").arg(loan->bookTitle(), loan->studentName());
        if (const int late = loan->daysOverdue(today()); late > 0)
            msg += QString(" It was %1 day%2 late.").arg(late).arg(late == 1 ? "" : "s");
        return Result::success(msg);
    } catch (const DbException& e) {
        return dbFailure(e);
    }
}

// ============================================================ reports
QVector<Loan> LibraryService::activeLoans(const QString& filterText) { return m_loans.active(filterText); }
QVector<Loan> LibraryService::overdueLoans() { return m_loans.overdue(today()); }
QVector<Loan> LibraryService::loanHistory() { return m_loans.history(); }
QVector<Loan> LibraryService::recentLoans(int maxRows) { return m_loans.recent(maxRows); }

LibraryStats LibraryService::stats() {
    LibraryStats s;
    s.titles = m_books.titleCount();
    s.copies = m_books.copyCount();
    s.students = m_students.count();
    s.activeLoans = m_loans.activeCount();
    s.overdueLoans = m_loans.overdueCount(today());
    return s;
}

// ============================================================ demo data
bool LibraryService::seedDemoData() {
    try {
        if (m_books.titleCount() > 0 || m_students.count() > 0) return false;
        DbTransaction tx(m_db);
        const Book books[] = {
            {"B001", "Clean Code", "Robert C. Martin", "Programming", 5, "A1-01"},
            {"B002", "Database System Concepts", "Silberschatz", "Databases", 4, "A1-02"},
            {"B003", "Introduction to Algorithms", "Cormen", "Algorithms", 3, "A2-01"},
            {"B004", "Systems Analysis and Design", "Kendall", "Information Systems", 6, "B1-01"},
            {"B005", "The C++ Programming Language", "Bjarne Stroustrup", "Programming", 2, "A1-03"},
            {"B006", "Operating System Concepts", "Silberschatz", "Operating Systems", 3, "A2-02"},
            {"B007", "Computer Networks", "Tanenbaum", "Networking", 4, "A3-01"},
            {"B008", "Principles of Marketing", "Philip Kotler", "Management", 5, "C1-01"},
            {"B009", "Financial Accounting", "Weygandt", "Accounting", 3, "C2-01"},
            {"B010", "Design Patterns", "Erich Gamma", "Programming", 1, "A1-04"},
            {"B011", "Artificial Intelligence: A Modern Approach", "Stuart Russell", "AI", 2, "A4-01"},
            {"B012", "Business Intelligence", "Ramesh Sharda", "Information Systems", 0, "B1-02"},
        };
        for (const Book& b : books) m_books.insert(b);
        const Student students[] = {
            {"20230101", "Ahmed Hassan", "Information Systems", "01001234567", "ahmed.hassan@example.com"},
            {"20230102", "Mariam Ali", "Computer Science", "01112345678", "mariam.ali@example.com"},
            {"20230103", "Omar Khaled", "Information Systems", "01223456789", ""},
            {"20230104", "Sara Mahmoud", "Business Information Systems", "01554321098", "sara.m@example.com"},
            {"20230105", "Youssef Adel", "Computer Science", "", "youssef.adel@example.com"},
            {"20230106", "Nour Ibrahim", "Information Systems", "01099887766", ""},
        };
        for (const Student& s : students) m_students.insert(s);

        auto lend = [&](const char* book, const char* student, int daysAgo, int periodDays, bool returned) {
            const QDate issued = today().addDays(-daysAgo);
            m_loans.insert(book, student, issued, issued.addDays(periodDays));
            m_books.takeOneCopy(book);
            if (returned) {
                const auto open = m_loans.active(student);
                for (const Loan& l : open)
                    if (l.bookId() == book && l.studentId() == student) {
                        m_loans.markReturned(l.loanId(), issued.addDays(periodDays - 2));
                        m_books.changeCopies(book, +1);
                    }
            }
        };
        lend("B003", "20230101", 20, 7, false);   // overdue by 13 days
        lend("B005", "20230102", 10, 7, false);   // overdue by 3 days
        lend("B001", "20230103", 3, 7, false);
        lend("B007", "20230104", 1, 7, false);
        lend("B010", "20230105", 7, 7, false);    // due today
        lend("B002", "20230106", 30, 7, true);
        lend("B004", "20230101", 15, 7, true);
        tx.commit();
        return true;
    } catch (const DbException&) {
        return false;
    }
}
