#include "LoginDialog.h"
#include "Branding.h"
#include "UiUtil.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace {

// Left side of the login window: cropped bookshelf picture with emblem and product name.
class ArtPanel : public QWidget {
public:
    explicit ArtPanel(QWidget* parent = nullptr)
        : QWidget(parent), m_art(":/images/login_art.png"), m_emblem(":/images/emblem.png") {
        setFixedWidth(440);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        const QPixmap scaled = m_art.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        p.drawPixmap((width() - scaled.width()) / 2, (height() - scaled.height()) / 2, scaled);

        QLinearGradient g(0, 0, 0, height());
        g.setColorAt(0.0, QColor(15, 9, 5, 120));
        g.setColorAt(0.55, QColor(15, 9, 5, 40));
        g.setColorAt(1.0, QColor(15, 9, 5, 235));
        p.fillRect(rect(), g);

        p.drawPixmap(40, 38, m_emblem.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        p.setPen(QColor("#F6C26B"));
        QFont org = font();
        org.setPointSize(10);
        org.setBold(true);
        org.setLetterSpacing(QFont::AbsoluteSpacing, 1.6);
        p.setFont(org);
        p.drawText(QRect(40, height() - 168, width() - 80, 22), Qt::AlignLeft | Qt::AlignVCenter,
                   QString(Branding::kOrganisation).toUpper());

        p.setPen(Qt::white);
        QFont title = font();
        title.setPointSize(26);
        title.setBold(true);
        p.setFont(title);
        p.drawText(QRect(40, height() - 142, width() - 80, 50), Qt::AlignLeft | Qt::AlignVCenter,
                   Branding::kAppName);

        p.setPen(QColor(232, 220, 204));
        QFont sub = font();
        sub.setPointSize(11);
        p.setFont(sub);
        p.drawText(QRect(40, height() - 92, width() - 80, 48), Qt::AlignLeft | Qt::TextWordWrap,
                   "Catalogue, students and borrowing in one place.");
    }

private:
    QPixmap m_art, m_emblem;
};

}  // namespace

LoginDialog::LoginDialog(AuthService& auth, QWidget* parent) : QDialog(parent), m_auth(auth) {
    setWindowTitle(QString("%1 - Sign in").arg(Branding::kAppName));
    setWindowIcon(QIcon(":/images/emblem.png"));
    setFixedSize(900, 560);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setObjectName("loginDialog");

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(new ArtPanel);

    auto* formSide = new QWidget;
    formSide->setObjectName("loginForm");
    auto* f = new QVBoxLayout(formSide);
    f->setContentsMargins(48, 56, 48, 40);
    f->setSpacing(8);

    f->addWidget(ui::makeLabel("Welcome back", "loginTitle"));
    f->addWidget(ui::makeLabel("Sign in to manage the library.", "pageSubtitle"));
    f->addSpacing(22);

    f->addWidget(ui::makeLabel("Username", "fieldLabel"));
    m_user = new QLineEdit;
    m_user->setPlaceholderText("Enter your username");
    m_user->addAction(ui::icon("user", "dark"), QLineEdit::LeadingPosition);
    m_user->setMinimumHeight(42);
    f->addWidget(m_user);
    f->addSpacing(6);

    f->addWidget(ui::makeLabel("Password", "fieldLabel"));
    m_pass = new QLineEdit;
    m_pass->setPlaceholderText("Enter your password");
    m_pass->setEchoMode(QLineEdit::Password);
    m_pass->addAction(ui::icon("lock", "dark"), QLineEdit::LeadingPosition);
    m_pass->setMinimumHeight(42);
    f->addWidget(m_pass);

    m_show = new QCheckBox("Show password");
    f->addWidget(m_show);

    m_error = ui::makeLabel(QString(), "errorLabel");
    m_error->setWordWrap(true);
    m_error->hide();
    f->addWidget(m_error);

    f->addSpacing(10);
    auto* signIn = ui::makeButton("Sign in", QString(), "primary");
    signIn->setMinimumHeight(46);
    signIn->setDefault(true);
    f->addWidget(signIn);
    f->addStretch();
    f->addWidget(ui::makeLabel(QString("%1  |  %2").arg(Branding::kOrganisation, Branding::kAppTagline),
                               "mutedLabel"));
    root->addWidget(formSide, 1);

    connect(signIn, &QPushButton::clicked, this, &LoginDialog::trySignIn);
    connect(m_pass, &QLineEdit::returnPressed, this, &LoginDialog::trySignIn);
    connect(m_user, &QLineEdit::returnPressed, m_pass, qOverload<>(&QWidget::setFocus));
    connect(m_show, &QCheckBox::toggled, this,
            [this](bool on) { m_pass->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password); });
    m_user->setFocus();
}

void LoginDialog::trySignIn() {
    const Result r = m_auth.authenticate(m_user->text(), m_pass->text(), &m_displayName);
    if (r.ok) {
        accept();
        return;
    }
    ++m_attempts;
    if (m_attempts >= 3) {
        // too many wrong attempts: tell the user, then close (the program ends)
        m_error->setText("Too many failed attempts. The program will close.");
        m_error->show();
        m_user->setEnabled(false);
        m_pass->setEnabled(false);
        QTimer::singleShot(1600, this, &QDialog::reject);
        return;
    }
    m_error->setText(QString("%1  (%2 attempt%3 left)")
                         .arg(r.message).arg(3 - m_attempts).arg(3 - m_attempts == 1 ? "" : "s"));
    m_error->show();
    m_pass->clear();
    m_pass->setFocus();
}
