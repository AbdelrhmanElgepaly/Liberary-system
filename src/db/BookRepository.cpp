#include "BookRepository.h"

namespace {
const char* kSelect = "SELECT BookID, Title, Author, Genre, Copies, ShelfLocation FROM Books ";
}

Book BookRepository::fromRow(const QSqlQuery& q) {
    return Book(q.value(0).toString(), q.value(1).toString(), q.value(2).toString(),
                q.value(3).toString(), q.value(4).toInt(), q.value(5).toString());
}

QVector<Book> BookRepository::all() {
    QSqlQuery q = m_db.exec(QString(kSelect) + "ORDER BY Title");
    QVector<Book> out;
    while (q.next()) out.append(fromRow(q));
    return out;
}

QVector<Book> BookRepository::search(const QString& text, BookField field) {
    const QString t = text.trimmed();
    if (t.isEmpty()) return all();
    const QString like = "%" + t + "%";

    QString where;
    QVariantList params;
    switch (field) {
    case BookField::Id:     where = "BookID LIKE ?";  params << like; break;
    case BookField::Title:  where = "Title LIKE ?";   params << like; break;
    case BookField::Author: where = "Author LIKE ?";  params << like; break;
    case BookField::Genre:  where = "Genre LIKE ?";   params << like; break;
    case BookField::Any:
        where = "BookID LIKE ? OR Title LIKE ? OR Author LIKE ? OR Genre LIKE ?";
        params << like << like << like << like;
        break;
    }
    QSqlQuery q = m_db.exec(QString(kSelect) + "WHERE " + where + " ORDER BY Title", params);
    QVector<Book> out;
    while (q.next()) out.append(fromRow(q));
    return out;
}

QVector<Book> BookRepository::available() {
    QSqlQuery q = m_db.exec(QString(kSelect) + "WHERE Copies > 0 ORDER BY Title");
    QVector<Book> out;
    while (q.next()) out.append(fromRow(q));
    return out;
}

std::optional<Book> BookRepository::find(const QString& id) {
    QSqlQuery q = m_db.exec(QString(kSelect) + "WHERE BookID = ?", {id.trimmed()});
    if (q.next()) return fromRow(q);
    return std::nullopt;
}

QStringList BookRepository::genres() {
    QSqlQuery q = m_db.exec("SELECT DISTINCT Genre FROM Books ORDER BY Genre");
    QStringList out;
    while (q.next()) out << q.value(0).toString();
    return out;
}

void BookRepository::insert(const Book& b) {
    m_db.exec("INSERT INTO Books (BookID, Title, Author, Genre, Copies, ShelfLocation) "
              "VALUES (?, ?, ?, ?, ?, ?)",
              {b.id(), b.title(), b.author(), b.genre(), b.copies(), b.location()});
}

void BookRepository::update(const Book& b) {
    m_db.exec("UPDATE Books SET Title = ?, Author = ?, Genre = ?, Copies = ?, ShelfLocation = ? "
              "WHERE BookID = ?",
              {b.title(), b.author(), b.genre(), b.copies(), b.location(), b.id()});
}

void BookRepository::remove(const QString& id) {
    m_db.exec("DELETE FROM Books WHERE BookID = ?", {id});
}

void BookRepository::changeCopies(const QString& id, int delta) {
    m_db.exec("UPDATE Books SET Copies = Copies + ? WHERE BookID = ?", {delta, id});
}

bool BookRepository::takeOneCopy(const QString& id) {
    QSqlQuery q = m_db.exec("UPDATE Books SET Copies = Copies - 1 WHERE BookID = ? AND Copies > 0", {id});
    return q.numRowsAffected() == 1;
}

int BookRepository::titleCount() {
    QSqlQuery q = m_db.exec("SELECT COUNT(*) FROM Books");
    return q.next() ? q.value(0).toInt() : 0;
}

int BookRepository::copyCount() {
    QSqlQuery q = m_db.exec("SELECT SUM(Copies) FROM Books");
    return q.next() ? q.value(0).toInt() : 0;
}
