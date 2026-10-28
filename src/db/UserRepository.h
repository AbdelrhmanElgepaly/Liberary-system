#pragma once
#include "DatabaseManager.h"
#include <optional>

struct UserRecord {
    QString username, passwordHash, salt, fullName;
};

// All SQL that touches the Users table (administrator accounts).
class UserRepository {
public:
    explicit UserRepository(DatabaseManager& db) : m_db(db) {}

    int count();
    std::optional<UserRecord> find(const QString& username);
    void insert(const UserRecord& user);

private:
    DatabaseManager& m_db;
};
