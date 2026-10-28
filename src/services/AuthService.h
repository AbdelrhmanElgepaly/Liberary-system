#pragma once
#include "db/UserRepository.h"
#include "Result.h"

// Administrator login. Passwords are stored as salted SHA-256 hashes, never as plain text.
class AuthService {
public:
    explicit AuthService(DatabaseManager& db) : m_users(db) {}

    // Creates the default administrator (salah / root, as in the original project) on first run.
    void ensureDefaultAdmin();

    // On success the displayName is filled with the administrator's full name.
    Result authenticate(const QString& username, const QString& password, QString* displayName = nullptr);

private:
    static QString hashPassword(const QString& salt, const QString& password);
    UserRepository m_users;
};
