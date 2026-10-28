#include "StudentRepository.h"

namespace {
const char* kSelect = "SELECT StudentID, FullName, Department, Phone, Email FROM Students ";
}

Student StudentRepository::fromRow(const QSqlQuery& q) {
    return Student(q.value(0).toString(), q.value(1).toString(), q.value(2).toString(),
                   q.value(3).toString(), q.value(4).toString());
}

QVector<Student> StudentRepository::all() {
    QSqlQuery q = m_db.exec(QString(kSelect) + "ORDER BY FullName");
    QVector<Student> out;
    while (q.next()) out.append(fromRow(q));
    return out;
}

QVector<Student> StudentRepository::search(const QString& text) {
    const QString t = text.trimmed();
    if (t.isEmpty()) return all();
    const QString like = "%" + t + "%";
    QSqlQuery q = m_db.exec(QString(kSelect) +
                            "WHERE StudentID LIKE ? OR FullName LIKE ? OR Department LIKE ? "
                            "OR Phone LIKE ? OR Email LIKE ? ORDER BY FullName",
                            {like, like, like, like, like});
    QVector<Student> out;
    while (q.next()) out.append(fromRow(q));
    return out;
}

std::optional<Student> StudentRepository::find(const QString& id) {
    QSqlQuery q = m_db.exec(QString(kSelect) + "WHERE StudentID = ?", {id.trimmed()});
    if (q.next()) return fromRow(q);
    return std::nullopt;
}

void StudentRepository::insert(const Student& s) {
    m_db.exec("INSERT INTO Students (StudentID, FullName, Department, Phone, Email) VALUES (?, ?, ?, ?, ?)",
              {s.id(), s.fullName(), DatabaseManager::nullableText(s.department()),
               DatabaseManager::nullableText(s.phone()), DatabaseManager::nullableText(s.email())});
}

void StudentRepository::update(const Student& s) {
    m_db.exec("UPDATE Students SET FullName = ?, Department = ?, Phone = ?, Email = ? WHERE StudentID = ?",
              {s.fullName(), DatabaseManager::nullableText(s.department()),
               DatabaseManager::nullableText(s.phone()), DatabaseManager::nullableText(s.email()), s.id()});
}

void StudentRepository::remove(const QString& id) {
    m_db.exec("DELETE FROM Students WHERE StudentID = ?", {id});
}

int StudentRepository::count() {
    QSqlQuery q = m_db.exec("SELECT COUNT(*) FROM Students");
    return q.next() ? q.value(0).toInt() : 0;
}
