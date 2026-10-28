#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariantList>
#include <memory>
#include <stdexcept>

// Raised for every database problem (connection, SQL error, missing driver ...).
class DbException : public std::runtime_error {
public:
    explicit DbException(const QString& message)
        : std::runtime_error(message.toStdString()), m_message(message) {}
    const QString& message() const { return m_message; }

private:
    QString m_message;
};

// Owns the connection to the Microsoft Access (.accdb) file and offers small helpers
// for running parameterised SQL. All other database classes go through this one.
class DatabaseManager {
public:
    // Wraps an already opened connection (used by tests / alternative back-ends).
    explicit DatabaseManager(const QSqlDatabase& db) : m_db(db) {}

    // Opens (and on Windows creates, if missing) an Access database through ODBC.
    // Throws DbException with a readable explanation when that is not possible.
    static std::unique_ptr<DatabaseManager> openAccess(const QString& filePath);

    // Creates the Books / Students / Loans / Users tables and the report queries if missing.
    void ensureSchema();

    // Prepares + executes a statement with positional (?) parameters. Throws DbException on error.
    QSqlQuery exec(const QString& sql, const QVariantList& params = {});

    // Value to bind for an optional text column: empty text is stored as NULL.
    static QVariant nullableText(const QString& text);

    bool beginTransaction() { return m_db.transaction(); }
    bool commitTransaction() { return m_db.commit(); }
    bool rollbackTransaction() { return m_db.rollback(); }

    const QSqlDatabase& database() const { return m_db; }

private:
    QSqlDatabase m_db;
};

// RAII helper: rolls back automatically unless commit() was called.
class DbTransaction {
public:
    explicit DbTransaction(DatabaseManager& db) : m_db(db) {
        if (!m_db.beginTransaction())
            throw DbException("Could not start a database transaction.");
    }
    ~DbTransaction() {
        if (!m_done) m_db.rollbackTransaction();
    }
    void commit() {
        if (!m_db.commitTransaction())
            throw DbException("Could not save the changes to the database.");
        m_done = true;
    }
    DbTransaction(const DbTransaction&) = delete;
    DbTransaction& operator=(const DbTransaction&) = delete;

private:
    DatabaseManager& m_db;
    bool m_done = false;
};
