#pragma once
#include "services/AuthService.h"

#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;

// Administrator sign-in window: library artwork on the left, form on the right.
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(AuthService& auth, QWidget* parent = nullptr);
    const QString& userName() const { return m_displayName; }

private slots:
    void trySignIn();

private:
    AuthService& m_auth;
    QLineEdit *m_user, *m_pass;
    QCheckBox* m_show;
    QLabel* m_error;
    QString m_displayName;
    int m_attempts = 0;
};
