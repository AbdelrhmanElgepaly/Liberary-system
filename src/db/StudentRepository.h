#pragma once
#include "DatabaseManager.h"
#include "models/Student.h"
#include <QVector>
#include <optional>

// All SQL that touches the Students table.
class StudentRepository {
public:
    explicit StudentRepository(DatabaseManager& db) : m_db(db) {}

    QVector<Student> all();
    QVector<Student> search(const QString& text);
    std::optional<Student> find(const QString& id);

    void insert(const Student& s);
    void update(const Student& s);
    void remove(const QString& id);

    int count();

private:
    static Student fromRow(const QSqlQuery& q);
    DatabaseManager& m_db;
};
