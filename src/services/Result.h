#pragma once
#include <QString>

// Outcome of a business operation: success/failure plus a message that can be shown to the user.
struct Result {
    bool ok = false;
    QString message;

    static Result success(const QString& msg = QString()) { return {true, msg}; }
    static Result failure(const QString& msg) { return {false, msg}; }
    explicit operator bool() const { return ok; }
};
