#include "DashboardPage.h"
#include "Branding.h"
#include "UiUtil.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QStackedLayout>
#include <QVBoxLayout>

namespace {

// Rounded banner: library picture, dark gradient and a greeting.
class HeroBanner : public QWidget {
public:
    HeroBanner(const QString& userName, QWidget* parent = nullptr)
        : QWidget(parent), m_pixmap(":/images/hero.png"), m_user(userName) {
        setFixedHeight(168);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        QPainterPath clip;
        clip.addRoundedRect(rect(), 18, 18);
        p.setClipPath(clip);

        // "cover" scaling of the picture
        const QPixmap scaled = m_pixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        p.drawPixmap((width() - scaled.width()) / 2, (height() - scaled.height()) / 2, scaled);

        QLinearGradient g(0, 0, width(), 0);
        g.setColorAt(0.0, QColor(18, 11, 7, 235));
        g.setColorAt(0.55, QColor(18, 11, 7, 170));
        g.setColorAt(1.0, QColor(18, 11, 7, 70));
        p.fillRect(rect(), g);

        p.setPen(QColor("#F6C26B"));
        QFont small = font();
        small.setPointSize(10);
        small.setBold(true);
        small.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
        p.setFont(small);
        p.drawText(QRect(34, 28, width() - 68, 22), Qt::AlignLeft | Qt::AlignVCenter,
                   QString(Branding::kOrganisation).toUpper() + "  LIBRARY");

        p.setPen(Qt::white);
        QFont big = font();
        big.setPointSize(23);
        big.setBold(true);
        p.setFont(big);
        p.drawText(QRect(34, 58, width() - 68, 44), Qt::AlignLeft | Qt::AlignVCenter,
                   QString("Welcome back, %1").arg(m_user));

        p.setPen(QColor(235, 224, 210));
        QFont sub = font();
        sub.setPointSize(11);
        p.setFont(sub);
        p.drawText(QRect(34, 106, width() - 68, 28), Qt::AlignLeft | Qt::AlignVCenter,
                   "Here is what is happening in the library today.");
    }

private:
    QPixmap m_pixmap;
    QString m_user;
};

}  // namespace

DashboardPage::DashboardPage(LibraryService& service, const QString& userName, QWidget* parent)
    : Page(service, parent) {
    auto* layout = makePageLayout();
    layout->setSpacing(20);
    layout->addWidget(new HeroBanner(userName));

    // ------------------------------------------------------------ statistic cards
    auto* stats = new QHBoxLayout;
    stats->setSpacing(16);
    stats->addWidget(makeStatCard("book", "#C98A1B", "Book titles", &m_titles));
    stats->addWidget(makeStatCard("layers", "#3B82C4", "Copies on shelves", &m_copies));
    stats->addWidget(makeStatCard("students", "#1F9D8F", "Students", &m_students));
    stats->addWidget(makeStatCard("swap", "#7C5CD6", "Borrowed now", &m_borrowed));
    stats->addWidget(makeStatCard("alert", "#D6453D", "Overdue", &m_overdue));
    layout->addLayout(stats);

    // ------------------------------------------------------------ tables
    auto* row = new QHBoxLayout;
    row->setSpacing(20);

    auto* recentCard = ui::makeCard();
    auto* rl = new QVBoxLayout(recentCard);
    rl->setContentsMargins(20, 18, 20, 18);
    rl->setSpacing(10);
    rl->addWidget(ui::makeLabel("Recent activity", "cardTitle"));
    m_recent = ui::makeTable({"Book", "Student", "Issued", "Status"});
    m_recent->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_recent->setColumnWidth(1, 130);
    m_recent->setColumnWidth(2, 112);
    m_recent->setColumnWidth(3, 150);
    m_recent->setSortingEnabled(false);
    m_recent->setSelectionMode(QAbstractItemView::NoSelection);
    rl->addWidget(m_recent, 1);
    row->addWidget(recentCard, 5);

    auto* overdueCard = ui::makeCard();
    auto* ol = new QVBoxLayout(overdueCard);
    ol->setContentsMargins(20, 18, 20, 18);
    ol->setSpacing(10);
    ol->addWidget(ui::makeLabel("Overdue books", "cardTitle"));
    m_overdueTable = ui::makeTable({"Book", "Student", "Late"});
    m_overdueTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_overdueTable->setColumnWidth(1, 120);
    m_overdueTable->setColumnWidth(2, 64);
    m_overdueTable->setSortingEnabled(false);
    m_overdueTable->setSelectionMode(QAbstractItemView::NoSelection);
    ol->addWidget(m_overdueTable, 1);
    m_overdueEmpty = ui::makeLabel("No overdue books. Great!", "emptyState");
    m_overdueEmpty->setAlignment(Qt::AlignCenter);
    ol->addWidget(m_overdueEmpty);
    row->addWidget(overdueCard, 3);

    layout->addLayout(row, 1);
}

QWidget* DashboardPage::makeStatCard(const QString& iconName, const QString& colour, const QString& caption,
                                     QLabel** valueOut) {
    auto* card = ui::makeCard();
    auto* l = new QHBoxLayout(card);
    l->setContentsMargins(18, 16, 18, 16);
    l->setSpacing(14);

    auto* iconBox = new QLabel;
    iconBox->setFixedSize(48, 48);
    iconBox->setAlignment(Qt::AlignCenter);
    iconBox->setPixmap(ui::iconPixmap(iconName, "white", 26));
    iconBox->setStyleSheet(QString("background:%1; border-radius:14px;").arg(colour));
    l->addWidget(iconBox);

    auto* texts = new QVBoxLayout;
    texts->setSpacing(0);
    *valueOut = ui::makeLabel("0", "statValue");
    texts->addWidget(*valueOut);
    texts->addWidget(ui::makeLabel(caption, "statCaption"));
    l->addLayout(texts, 1);
    return card;
}

void DashboardPage::refresh() {
    ui::guard(this, [&] {
        const LibraryStats s = m_svc.stats();
        m_titles->setText(QString::number(s.titles));
        m_copies->setText(QString::number(s.copies));
        m_students->setText(QString::number(s.students));
        m_borrowed->setText(QString::number(s.activeLoans));
        m_overdue->setText(QString::number(s.overdueLoans));

        fillLoans(m_recent, m_svc.recentLoans(8), true);
        const QVector<Loan> overdue = m_svc.overdueLoans();
        fillLoans(m_overdueTable, overdue.mid(0, 8), false);
        m_overdueTable->setVisible(!overdue.isEmpty());
        m_overdueEmpty->setVisible(overdue.isEmpty());
    });
}

void DashboardPage::fillLoans(QTableWidget* table, const QVector<Loan>& loans, bool showStatus) {
    const QDate today = LibraryService::today();
    table->setRowCount(loans.size());
    for (int r = 0; r < loans.size(); ++r) {
        const Loan& l = loans[r];
        table->setItem(r, 0, ui::makeCell(l.bookTitle()));
        table->setItem(r, 1, ui::makeCell(l.studentName()));
        if (showStatus) {
            table->setItem(r, 2, ui::makeDateCell(l.issueDate()));
            auto* status = ui::makeCell(l.statusText(today));
            QFont f = status->font();
            f.setBold(true);
            status->setFont(f);
            switch (l.status(today)) {
            case Loan::Status::Overdue:  status->setForeground(QColor("#C0392B")); break;
            case Loan::Status::DueToday: status->setForeground(QColor("#B7791F")); break;
            case Loan::Status::Returned: status->setForeground(QColor("#7B6E62")); break;
            case Loan::Status::OnTime:   status->setForeground(QColor("#2E8B57")); break;
            }
            table->setItem(r, 3, status);
        } else {
            auto* late = ui::makeCell(QString("%1 d").arg(l.daysOverdue(today)));
            late->setForeground(QColor("#C0392B"));
            QFont f = late->font();
            f.setBold(true);
            late->setFont(f);
            table->setItem(r, 2, late);
        }
    }
}
