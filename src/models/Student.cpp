#include "Student.h"
#include <QRegularExpression>

QStringList Student::validate() const {
    QStringList errors;
    if (m_id.isEmpty())        errors << "Student ID is required.";
    else if (m_id.size() > 50) errors << "Student ID must be 50 characters or fewer.";
    if (m_fullName.isEmpty())  errors << "Full name is required.";
    else if (m_fullName.size() > 150) errors << "Full name is too long (max 150 characters).";
    if (m_department.size() > 100) errors << "Department is too long (max 100 characters).";
    if (!m_phone.isEmpty()) {
        static const QRegularExpression phoneRx("^[0-9+\\-\\s()]{6,30}$");
        if (!phoneRx.match(m_phone).hasMatch())
            errors << "Phone may contain digits, spaces, + - ( ) only (6-30 characters).";
    }
    if (!m_email.isEmpty()) {
        static const QRegularExpression mailRx("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$");
        if (!mailRx.match(m_email).hasMatch() || m_email.size() > 150)
            errors << "Email address is not valid.";
    }
    return errors;
}
