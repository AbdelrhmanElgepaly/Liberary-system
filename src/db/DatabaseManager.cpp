#include "DatabaseManager.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>

#include <string>

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <odbcinst.h>
#endif

namespace {

const char* kAccessDriver = "Microsoft Access Driver (*.mdb, *.accdb)";

const char* kHelp =
    "\n\nHow to fix:\n"
    " - Install the \"Microsoft Access Database Engine 2016 Redistributable\" (or Microsoft Office).\n"
    " - The driver must have the same bitness as this program (64-bit program -> 64-bit driver).\n"
    " - Make sure the database file is not open exclusively in Microsoft Access.";

#ifdef Q_OS_WIN
// Creates an empty Access 2007+ (.accdb) file using the ODBC driver itself.
void createAccdbIfMissing(const QString& nativePath) {
    if (QFileInfo::exists(nativePath)) return;
    QDir().mkpath(QFileInfo(nativePath).absolutePath());
    // The project is built with UNICODE, so use the wide ODBC call explicitly.
    std::wstring attrs = L"CREATE_DBV12=" + nativePath.toStdWString();
    attrs.push_back(L'\0');  // together with the string's own terminator = double NUL
    const std::wstring driver = QString::fromLatin1(kAccessDriver).toStdWString();
    if (!SQLConfigDataSourceW(nullptr, ODBC_ADD_DSN, driver.c_str(), attrs.c_str()))
        throw DbException(QString("The Access database file could not be created:\n%1%2")
                              .arg(nativePath, kHelp));
}
#endif

}  // namespace

std::unique_ptr<DatabaseManager> DatabaseManager::openAccess(const QString& filePath) {
    const QString nativePath = QDir::toNativeSeparators(QFileInfo(filePath).absoluteFilePath());

    if (!QSqlDatabase::isDriverAvailable("QODBC"))
        throw DbException("The Qt ODBC database driver (QODBC) is not available.\n"
                          "Deploy the program with windeployqt so that sqldrivers/qsqlodbc.dll is next to it.");

#ifdef Q_OS_WIN
    createAccdbIfMissing(nativePath);
#else
    if (!QFileInfo::exists(nativePath))
        throw DbException("Database file not found:\n" + nativePath +
                          "\n(Automatic creation of .accdb files is only available on Windows.)");
#endif

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", "library_access");
    db.setDatabaseName(QString("Driver={%1};DBQ=%2;").arg(kAccessDriver, nativePath));
    if (!db.open())
        throw DbException("Could not open the Access database:\n" + nativePath + "\n\n" +
                          db.lastError().text() + kHelp);
    return std::make_unique<DatabaseManager>(db);
}

QSqlQuery DatabaseManager::exec(const QString& sql, const QVariantList& params) {
    QSqlQuery query(m_db);
    if (!query.prepare(sql))
        throw DbException("SQL error: " + query.lastError().text());
    for (const QVariant& p : params) query.addBindValue(p);
    if (!query.exec())
        throw DbException("SQL error: " + query.lastError().text() + "\n[" + sql + "]");
    return query;
}

QVariant DatabaseManager::nullableText(const QString& text) {
    const QString t = text.trimmed();
    return t.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : QVariant(t);
}

void DatabaseManager::ensureSchema() {
    const QStringList existing = m_db.tables(QSql::Tables);
    auto has = [&](const char* name) { return existing.contains(name, Qt::CaseInsensitive); };

    // Microsoft Access (Jet/ACE) SQL dialect: TEXT(n), LONG, DATETIME, COUNTER (= AutoNumber).
    if (!has("Books"))
        exec("CREATE TABLE Books ("
             "BookID TEXT(50) NOT NULL, "
             "Title TEXT(255) NOT NULL, "
             "Author TEXT(255) NOT NULL, "
             "Genre TEXT(100) NOT NULL, "
             "Copies LONG NOT NULL, "
             "ShelfLocation TEXT(100) NOT NULL, "
             "CONSTRAINT pk_Books PRIMARY KEY (BookID))");

    if (!has("Students"))
        exec("CREATE TABLE Students ("
             "StudentID TEXT(50) NOT NULL, "
             "FullName TEXT(150) NOT NULL, "
             "Department TEXT(100), "
             "Phone TEXT(30), "
             "Email TEXT(150), "
             "CONSTRAINT pk_Students PRIMARY KEY (StudentID))");

    if (!has("Users"))
        exec("CREATE TABLE Users ("
             "UserID COUNTER, "
             "Username TEXT(50) NOT NULL, "
             "PasswordHash TEXT(64) NOT NULL, "
             "Salt TEXT(32) NOT NULL, "
             "FullName TEXT(100), "
             "CONSTRAINT pk_Users PRIMARY KEY (UserID), "
             "CONSTRAINT uq_Users_Username UNIQUE (Username))");

    if (!has("Loans"))
        exec("CREATE TABLE Loans ("
             "LoanID COUNTER, "
             "BookID TEXT(50) NOT NULL, "
             "StudentID TEXT(50) NOT NULL, "
             "IssueDate DATETIME NOT NULL, "
             "DueDate DATETIME NOT NULL, "
             "ReturnDate DATETIME, "
             "CONSTRAINT pk_Loans PRIMARY KEY (LoanID), "
             "CONSTRAINT fk_Loans_Books FOREIGN KEY (BookID) REFERENCES Books (BookID), "
             "CONSTRAINT fk_Loans_Students FOREIGN KEY (StudentID) REFERENCES Students (StudentID))");

    // Saved queries used by Power BI / Excel / Access itself. Best effort: if the driver
    // refuses CREATE VIEW the application still works (see README for the manual SQL).
    const QString joins =
        "FROM (Loans AS l INNER JOIN Books AS b ON l.BookID = b.BookID) "
        "INNER JOIN Students AS s ON l.StudentID = s.StudentID ";
    struct View { const char* name; QString sql; };
    const View views[] = {
        {"qryBorrowedBooks",
         "CREATE VIEW qryBorrowedBooks AS SELECT l.LoanID, l.BookID, b.Title, l.StudentID, "
         "s.FullName, l.IssueDate, l.DueDate " + joins + "WHERE l.ReturnDate IS NULL"},
        {"qryOverdueBooks",
         "CREATE VIEW qryOverdueBooks AS SELECT l.LoanID, l.BookID, b.Title, l.StudentID, "
         "s.FullName, l.IssueDate, l.DueDate, DateDiff('d', l.DueDate, Date()) AS DaysOverdue " +
         joins + "WHERE l.ReturnDate IS NULL AND l.DueDate < Date()"},
    };
    const QStringList existingViews = m_db.tables(QSql::Views);
    for (const View& v : views) {
        if (existingViews.contains(v.name, Qt::CaseInsensitive)) continue;
        try {
            exec(v.sql);
        } catch (const DbException& e) {
            qWarning() << "Could not create saved query" << v.name << ":" << e.message();
        }
    }
}
