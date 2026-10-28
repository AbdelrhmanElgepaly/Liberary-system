#include "AuthService.h"

#include <QCryptographicHash>
#include <QUuid>

QString AuthService::hashPassword(const QString& salt, const QString& password) {
    return QString::fromLatin1(
        QCryptographicHash::hash((salt + password).toUtf8(), QCryptographicHash::Sha256).toHex());
}

void AuthService::ensureDefaultAdmin() {
    if (m_users.count() > 0) return;
    const QString salt = QUuid::createUuid().toString(QUuid::Id128);
    m_users.insert({"salah", hashPassword(salt, "root"), salt, "Salah"});
}

Result AuthService::authenticate(const QString& username, const QString& password, QString* displayName) {
    if (username.trimmed().isEmpty() || password.isEmpty())
        return Result::failure("Please enter your username and password.");
    try {
        const auto user = m_users.find(username);
        // Same message for "unknown user" and "wrong password".
        if (!user || hashPassword(user->salt, password) != user->passwordHash)
            return Result::failure("Invalid username or password.");
        if (displayName) *displayName = user->fullName.isEmpty() ? user->username : user->fullName;
        return Result::success();
    } catch (const DbException& e) {
        return Result::failure("Database error: " + e.message());
    }
}
