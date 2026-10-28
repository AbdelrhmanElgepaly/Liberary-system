#pragma once
#include <QString>
#include <QStringList>

// A library member (one row of the Students table).
class Student {
public:
    Student() = default;
    Student(const QString& id, const QString& fullName, const QString& department,
            const QString& phone, const QString& email)
        : m_id(id.trimmed()), m_fullName(fullName.trimmed()), m_department(department.trimmed()),
          m_phone(phone.trimmed()), m_email(email.trimmed()) {}

    const QString& id() const { return m_id; }
    const QString& fullName() const { return m_fullName; }
    const QString& department() const { return m_department; }
    const QString& phone() const { return m_phone; }
    const QString& email() const { return m_email; }

    QStringList validate() const;

private:
    QString m_id, m_fullName, m_department, m_phone, m_email;
};
