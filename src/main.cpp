#include "db/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/LibraryService.h"
#include "ui/Branding.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QIcon>
#include <QMessageBox>
#include <QStringList>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(Branding::kAppName);
    app.setOrganizationName(Branding::kOrganisation);
    app.setWindowIcon(QIcon(":/images/emblem.png"));
    app.setStyle("Fusion");   // same look on every Windows version; our style sheet does the rest

    QFont font;
    font.setFamilies({"Segoe UI", "Inter", "Noto Sans", "DejaVu Sans"});
    font.setPointSize(10);
    app.setFont(font);

    QFile qss(":/style.qss");
    if (qss.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    // Usage:  LibraryManager.exe [path\to\library.accdb] [--demo]
    const QStringList args = app.arguments().mid(1);
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("data/library.accdb");
    bool demo = false;
    for (const QString& a : args) {
        if (a == "--demo") demo = true;
        else dbPath = a;
    }

    std::unique_ptr<DatabaseManager> db;
    try {
        db = DatabaseManager::openAccess(dbPath);
        db->ensureSchema();
    } catch (const DbException& e) {
        QMessageBox::critical(nullptr, QString("%1 - database error").arg(Branding::kAppName), e.message());
        return 1;
    }

    LibraryService library(*db);
    AuthService auth(*db);
    try {
        auth.ensureDefaultAdmin();
        if (demo) library.seedDemoData();
    } catch (const DbException& e) {
        QMessageBox::critical(nullptr, QString("%1 - database error").arg(Branding::kAppName), e.message());
        return 1;
    }

    // Sign-in -> main window; "Sign out" returns to the sign-in window.
    while (true) {
        LoginDialog login(auth);
        if (login.exec() != QDialog::Accepted) break;

        MainWindow window(library, login.userName());
        bool signedOut = false;
        QObject::connect(&window, &MainWindow::logoutRequested, [&] {
            signedOut = true;
            window.close();
        });
        window.show();
        app.exec();
        if (!signedOut) break;
    }
    return 0;
}
