#include "LoansPage.h"
#include "UiUtil.h"

#include <QCompleter>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>

namespace {
void setupSearchableCombo(QComboBox* c, const QString& placeholder) {
    c->setEditable(true);
    c->setInsertPolicy(QComboBox::NoInsert);
    c->completer()->setFilterMode(Qt::MatchContains);
    c->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    c->lineEdit()->setPlaceholderText(placeholder);
    c->setMinimumContentsLength(24);
}
}  // namespace

LoansPage::LoansPage(LibraryService& service, QWidget* parent) : Page(service, parent) {
    auto* layout = makePageLayout();
    layout->addWidget(makeHeader("Borrow & Return",
                                 "Lend books to students and take them back."));

    auto* body = new QHBoxLayout;
    body->setSpacing(20);

    // ---------------------------------------------------------------- borrow card
    auto* borrowCard = ui::makeCard();
    borrowCard->setFixedWidth(330);
    auto* bl = new QVBoxLayout(borrowCard);
    bl->setContentsMargins(22, 20, 22, 22);
    bl->setSpacing(10);

    auto* head = new QHBoxLayout;
    auto* headIcon = new QLabel;
    headIcon->setObjectName("cardIcon");
    headIcon->setPixmap(ui::iconPixmap("swap", "white", 20));
    headIcon->setAlignment(Qt::AlignCenter);
    headIcon->setFixedSize(38, 38);
    head->addWidget(headIcon);
    head->addWidget(ui::makeLabel("Borrow a book", "cardTitle"), 1);
    bl->addLayout(head);
    bl->addSpacing(6);

    bl->addWidget(ui::makeLabel("Student", "fieldLabel"));
    m_student = new QComboBox;
    setupSearchableCombo(m_student, "Type a name or ID...");
    bl->addWidget(m_student);

    bl->addWidget(ui::makeLabel("Book", "fieldLabel"));
    m_book = new QComboBox;
    setupSearchableCombo(m_book, "Type a title or ID...");
    bl->addWidget(m_book);

    m_dueInfo = ui::makeLabel(QString(), "infoBox");
    m_dueInfo->setWordWrap(true);
    bl->addSpacing(6);
    bl->addWidget(m_dueInfo);

    bl->addSpacing(6);
    m_borrow = ui::makeButton("Borrow book", "swap", "primary");
    m_borrow->setMinimumHeight(42);
    bl->addWidget(m_borrow);
    bl->addStretch();
    body->addWidget(borrowCard, 0);

    // ------------------------------------------------------------ borrowed books card
    auto* listCard = ui::makeCard();
    auto* ll = new QVBoxLayout(listCard);
    ll->setContentsMargins(20, 18, 20, 18);
    ll->setSpacing(14);

    auto* top = new QHBoxLayout;
    top->addWidget(ui::makeLabel("Borrowed books", "cardTitle"));
    top->addStretch();
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Filter by book or student...");
    m_search->setClearButtonEnabled(true);
    m_search->addAction(ui::icon("search", "dark"), QLineEdit::LeadingPosition);
    m_search->setMinimumWidth(260);
    top->addWidget(m_search);
    ll->addLayout(top);

    m_table = ui::makeTable({"Book", "Student", "Due", "Status"}, 2);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->setColumnWidth(1, 150);
    m_table->setColumnWidth(2, 112);
    m_table->setColumnWidth(3, 150);
    ll->addWidget(m_table, 1);

    auto* bottom = new QHBoxLayout;
    m_count = ui::makeLabel(QString(), "mutedLabel");
    m_return = ui::makeButton("Return selected book", "check");
    bottom->addWidget(m_count);
    bottom->addStretch();
    bottom->addWidget(m_return);
    ll->addLayout(bottom);
    body->addWidget(listCard, 1);

    layout->addLayout(body, 1);

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(220);
    connect(&m_debounce, &QTimer::timeout, this, &LoansPage::reloadLoans);
    connect(m_search, &QLineEdit::textChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(m_borrow, &QPushButton::clicked, this, &LoansPage::borrow);
    connect(m_return, &QPushButton::clicked, this, &LoansPage::returnSelected);
    connect(m_table, &QTableWidget::itemSelectionChanged, this,
            [this] { m_return->setEnabled(ui::selectedRow(m_table) >= 0); });
    m_return->setEnabled(false);
}

void LoansPage::refresh() {
    reloadChoices();
    reloadLoans();
}

QString LoansPage::idOfChoice(QComboBox* combo) {
    const int idx = combo->findText(combo->currentText(), Qt::MatchFixedString);
    return idx < 0 ? QString() : combo->itemData(idx).toString();
}

void LoansPage::reloadChoices() {
    const QString oldStudent = idOfChoice(m_student);
    const QString oldBook = idOfChoice(m_book);
    QVector<Student> students;
    QVector<Book> books;
    if (!ui::guard(this, [&] {
            students = m_svc.students();
            books = m_svc.availableBooks();
        }))
        return;

    m_student->clear();
    for (const Student& s : students)
        m_student->addItem(QString("%1  -  %2").arg(s.id(), s.fullName()), s.id());
    m_book->clear();
    for (const Book& b : books)
        m_book->addItem(QString("%1  -  %2  (%3 available)").arg(b.id(), b.title()).arg(b.copies()), b.id());

    auto restore = [](QComboBox* c, const QString& id) {
        const int i = c->findData(id);
        c->setCurrentIndex(i);          // -1 clears the box
        if (i < 0) c->clearEditText();
    };
    restore(m_student, oldStudent);
    restore(m_book, oldBook);

    m_dueInfo->setText(QString("Loan period: %1 days\nDue date: %2")
                           .arg(LibraryService::kLoanPeriodDays)
                           .arg(ui::dateText(LibraryService::today().addDays(LibraryService::kLoanPeriodDays))));
}

void LoansPage::reloadLoans() {
    QVector<Loan> loans;
    if (!ui::guard(this, [&] { loans = m_svc.activeLoans(m_search->text()); })) return;

    const QDate today = LibraryService::today();
    const int keep = ui::selectedKey(m_table).toInt();
    m_table->setSortingEnabled(false);
    m_table->setRowCount(loans.size());
    int reselect = -1;
    for (int r = 0; r < loans.size(); ++r) {
        const Loan& l = loans[r];
        auto* book = ui::makeCell(l.bookTitle());
        book->setToolTip(QString("%1  -  %2").arg(l.bookId(), l.bookTitle()));
        book->setData(Qt::UserRole, l.loanId());
        m_table->setItem(r, 0, book);
        m_table->setItem(r, 1, ui::makeCell(l.studentName()));
        m_table->setItem(r, 2, ui::makeDateCell(l.dueDate()));
        auto* status = ui::makeCell(l.statusText(today));
        QFont f = status->font();
        f.setBold(true);
        status->setFont(f);
        switch (l.status(today)) {
        case Loan::Status::Overdue:  status->setForeground(QColor("#C0392B")); break;
        case Loan::Status::DueToday: status->setForeground(QColor("#B7791F")); break;
        default:                     status->setForeground(QColor("#2E8B57")); break;
        }
        m_table->setItem(r, 3, status);
        if (l.loanId() == keep && keep != 0) reselect = r;
    }
    m_table->setSortingEnabled(true);
    if (reselect >= 0) m_table->selectRow(reselect);
    m_count->setText(QString("%1 borrowed book%2").arg(loans.size()).arg(loans.size() == 1 ? "" : "s"));
    m_return->setEnabled(ui::selectedRow(m_table) >= 0);
}

void LoansPage::borrow() {
    const QString studentId = idOfChoice(m_student);
    const QString bookId = idOfChoice(m_book);
    if (studentId.isEmpty()) {
        emit notify("Please choose a student from the list.", Toast::Error);
        m_student->setFocus();
        return;
    }
    if (bookId.isEmpty()) {
        emit notify("Please choose a book from the list.", Toast::Error);
        m_book->setFocus();
        return;
    }
    const Result r = m_svc.borrowBook(bookId, studentId);
    emit notify(r.message, r.ok ? Toast::Success : Toast::Error);
    if (r.ok) {
        m_student->setCurrentIndex(-1);
        m_student->clearEditText();
        m_book->setCurrentIndex(-1);
        m_book->clearEditText();
    }
    refresh();
}

void LoansPage::returnSelected() {
    const int row = ui::selectedRow(m_table);
    if (row < 0) return;
    const int loanId = ui::selectedKey(m_table).toInt();
    const QString book = m_table->item(row, 0)->text();
    const QString student = m_table->item(row, 1)->text();
    if (QMessageBox::question(this, "Return book",
                              QString("Mark \"%1\" as returned by %2?").arg(book, student),
                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes) != QMessageBox::Yes)
        return;
    const Result r = m_svc.returnBook(loanId);
    emit notify(r.message, r.ok ? Toast::Success : Toast::Error);
    refresh();
}
