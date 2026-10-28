#pragma once
#include <QString>
#include <QStringList>

// A book title in the catalogue (one row of the Books table).
class Book {
public:
    Book() = default;
    Book(const QString& id, const QString& title, const QString& author,
         const QString& genre, int copies, const QString& location)
        : m_id(id.trimmed()), m_title(title.trimmed()), m_author(author.trimmed()),
          m_genre(genre.trimmed()), m_copies(copies), m_location(location.trimmed()) {}

    const QString& id() const { return m_id; }
    const QString& title() const { return m_title; }
    const QString& author() const { return m_author; }
    const QString& genre() const { return m_genre; }
    int copies() const { return m_copies; }
    const QString& location() const { return m_location; }

    // Returns a list of human readable problems (empty list = valid).
    QStringList validate() const;

    static constexpr int kMaxCopies = 100000;

private:
    QString m_id, m_title, m_author, m_genre;
    int m_copies = 0;
    QString m_location;
};
