#include "UserRepository.h"

int UserRepository::count() {
    QSqlQuery q = m_db.exec("SELECT COUNT(*) FROM Users");
    return q.next() ? q.value(0).toInt() : 0;
}

std::optional<UserRecord> UserRepository::find(const QString& username) {
    QSqlQuery q = m_db.exec("SELECT Username, PasswordHash, Salt, FullName FROM Users WHERE Username = ?",
                            {username.trimmed()});
    if (!q.next()) return std::nullopt;
    return UserRecord{q.value(0).toString(), q.value(1).toString(), q.value(2).toString(),
                      q.value(3).toString()};
}

void UserRepository::insert(const UserRecord& u) {
    m_db.exec("INSERT INTO Users (Username, PasswordHash, Salt, FullName) VALUES (?, ?, ?, ?)",
              {u.username, u.passwordHash, u.salt, DatabaseManager::nullableText(u.fullName)});
}
