#pragma once
#include "DatabaseManager.h"
#include "models/Book.h"
#include <QVector>
#include <optional>

enum class BookField { Any, Id, Title, Author, Genre };

// All SQL that touches the Books table.
class BookRepository {
public:
    explicit BookRepository(DatabaseManager& db) : m_db(db) {}

    QVector<Book> all();
    QVector<Book> search(const QString& text, BookField field);
    QVector<Book> available();                 // books with at least one copy on the shelf
    std::optional<Book> find(const QString& id);
    QStringList genres();

    void insert(const Book& book);
    void update(const Book& book);
    void remove(const QString& id);

    void changeCopies(const QString& id, int delta);
    bool takeOneCopy(const QString& id);       // false when no copy is left

    int titleCount();
    int copyCount();

private:
    static Book fromRow(const QSqlQuery& q);
    DatabaseManager& m_db;
};
