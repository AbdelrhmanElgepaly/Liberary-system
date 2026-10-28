#include "MainWindow.h"
#include "BooksPage.h"
#include "Branding.h"
#include "DashboardPage.h"
#include "LoansPage.h"
#include "ReportsPage.h"
#include "StudentsPage.h"
#include "Toast.h"
#include "UiUtil.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(LibraryService& service, const QString& userName, QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QString("%1 - %2").arg(Branding::kAppName, Branding::kOrganisation));
    setWindowIcon(QIcon(":/images/emblem.png"));
    resize(1320, 820);
    setMinimumSize(1120, 700);

    auto* central = new QWidget;
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ------------------------------------------------------------------ sidebar
    auto* sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(248);
    auto* sl = new QVBoxLayout(sidebar);
    sl->setContentsMargins(16, 22, 16, 18);
    sl->setSpacing(4);

    auto* brand = new QHBoxLayout;
    brand->setContentsMargins(6, 0, 0, 0);
    brand->setSpacing(12);
    auto* logo = new QLabel;
    logo->setPixmap(QPixmap(":/images/emblem.png").scaled(44, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logo->setFixedSize(44, 44);
    brand->addWidget(logo);
    auto* brandText = new QVBoxLayout;
    brandText->setSpacing(0);
    brandText->addWidget(ui::makeLabel(Branding::kAppName, "brandName"));
    brandText->addWidget(ui::makeLabel(Branding::kOrganisation, "brandSub"));
    brand->addLayout(brandText, 1);
    sl->addLayout(brand);
    sl->addSpacing(26);

    sl->addWidget(ui::makeLabel("MENU", "navSection"));

    m_stack = new QStackedWidget;
    m_stack->setObjectName("pages");
    m_nav = new QButtonGroup(this);
    m_nav->setExclusive(true);

    struct Item { const char* text; const char* icon; };
    const Item items[] = {{"Dashboard", "dashboard"}, {"Books", "book"}, {"Students", "students"},
                          {"Borrow && Return", "swap"}, {"Reports", "report"}};
    m_pages = {new DashboardPage(service, userName), new BooksPage(service), new StudentsPage(service),
               new LoansPage(service), new ReportsPage(service)};

    for (int i = 0; i < m_pages.size(); ++i) {
        auto* b = new QPushButton(items[i].text);
        b->setObjectName("navButton");
        b->setCheckable(true);
        b->setIcon(ui::icon(items[i].icon, "white"));
        b->setIconSize(QSize(20, 20));
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(46);
        m_nav->addButton(b, i);
        sl->addWidget(b);

        m_stack->addWidget(m_pages[i]);
        connect(m_pages[i], &Page::notify, this, &MainWindow::showToast);
    }
    connect(m_nav, &QButtonGroup::idClicked, this, &MainWindow::showPage);

    sl->addStretch();

    // user box + logout
    auto* userBox = new QFrame;
    userBox->setObjectName("userBox");
    auto* ul = new QHBoxLayout(userBox);
    ul->setContentsMargins(10, 10, 10, 10);
    ul->setSpacing(10);
    auto* avatar = new QLabel(userName.left(1).toUpper());
    avatar->setObjectName("avatar");
    avatar->setFixedSize(36, 36);
    avatar->setAlignment(Qt::AlignCenter);
    ul->addWidget(avatar);
    auto* who = new QVBoxLayout;
    who->setSpacing(0);
    who->addWidget(ui::makeLabel(userName, "userName"));
    who->addWidget(ui::makeLabel("Administrator", "userRole"));
    ul->addLayout(who, 1);
    sl->addWidget(userBox);

    auto* logout = new QPushButton("Sign out");
    logout->setObjectName("logoutButton");
    logout->setIcon(ui::icon("logout", "white"));
    logout->setIconSize(QSize(18, 18));
    logout->setCursor(Qt::PointingHandCursor);
    logout->setMinimumHeight(42);
    sl->addWidget(logout);
    connect(logout, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, "Sign out", "Do you want to sign out?",
                                  QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes)
            emit logoutRequested();
    });

    root->addWidget(sidebar);
    root->addWidget(m_stack, 1);
    setCentralWidget(central);

    showPage(0);
}

void MainWindow::showPage(int index) {
    if (index < 0 || index >= m_pages.size()) return;
    m_nav->button(index)->setChecked(true);
    m_stack->setCurrentIndex(index);
    m_pages[index]->refresh();
}

void MainWindow::showToast(const QString& message, int kind) {
    Toast::popup(this, message, Toast::Kind(kind));
}
