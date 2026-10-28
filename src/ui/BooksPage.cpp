#include "BooksPage.h"
#include "BookDialog.h"
#include "UiUtil.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

BooksPage::BooksPage(LibraryService& service, QWidget* parent) : Page(service, parent) {
    auto* layout = makePageLayout();

    auto* addButton = ui::makeButton("Add Book", "plus", "primary");
    layout->addWidget(makeHeader("Books", "Manage the catalogue: search, add, edit and track copies.", addButton));

    auto* card = ui::makeCard();
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(14);

    // --- search row
    auto* searchRow = new QHBoxLayout;
    searchRow->setSpacing(10);
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search books...");
    m_search->setClearButtonEnabled(true);
    m_search->addAction(ui::icon("search", "dark"), QLineEdit::LeadingPosition);
    m_search->setMinimumWidth(320);
    m_field = new QComboBox;
    m_field->addItem("All fields", int(BookField::Any));
    m_field->addItem("Book ID", int(BookField::Id));
    m_field->addItem("Title", int(BookField::Title));
    m_field->addItem("Author", int(BookField::Author));
    m_field->addItem("Genre", int(BookField::Genre));
    m_count = ui::makeLabel(QString(), "mutedLabel");
    searchRow->addWidget(m_search, 1);
    searchRow->addWidget(m_field);
    searchRow->addStretch(2);
    searchRow->addWidget(m_count);
    cardLayout->addLayout(searchRow);

    // --- table
    m_table = ui::makeTable({"Book ID", "Title", "Author", "Genre", "Copies", "Location"}, 1);
    m_table->setColumnWidth(0, 110);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(2, 200);
    m_table->setColumnWidth(3, 170);
    m_table->setColumnWidth(4, 90);
    cardLayout->addWidget(m_table, 1);

    // --- action row
    auto* actions = new QHBoxLayout;
    actions->setSpacing(10);
    m_edit = ui::makeButton("Edit", "edit");
    m_addCopies = ui::makeButton("Add copies", "plus");
    m_removeCopies = ui::makeButton("Remove copies", "minus");
    m_delete = ui::makeButton("Delete", "trash", "danger");
    actions->addWidget(m_edit);
    actions->addWidget(m_addCopies);
    actions->addWidget(m_removeCopies);
    actions->addStretch();
    actions->addWidget(m_delete);
    cardLayout->addLayout(actions);

    layout->addWidget(card, 1);

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(220);
    connect(&m_debounce, &QTimer::timeout, this, &BooksPage::reload);
    connect(m_search, &QLineEdit::textChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(m_field, &QComboBox::currentIndexChanged, this, &BooksPage::reload);
    connect(addButton, &QPushButton::clicked, this, &BooksPage::addBook);
    connect(m_edit, &QPushButton::clicked, this, &BooksPage::editBook);
    connect(m_delete, &QPushButton::clicked, this, &BooksPage::deleteBook);
    connect(m_addCopies, &QPushButton::clicked, this, [this] { changeCopies(true); });
    connect(m_removeCopies, &QPushButton::clicked, this, [this] { changeCopies(false); });
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &BooksPage::updateButtons);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this] { editBook(); });

    updateButtons();
}

void BooksPage::refresh() { reload(); }

void BooksPage::reload() {
    const QString keepSelected = ui::selectedKey(m_table);
    QVector<Book> books;
    if (!ui::guard(this, [&] {
            books = m_svc.books(m_search->text(), BookField(m_field->currentData().toInt()));
        }))
        return;

    m_table->setSortingEnabled(false);
    m_table->setRowCount(books.size());
    int reselect = -1;
    for (int r = 0; r < books.size(); ++r) {
        const Book& b = books[r];
        auto* idItem = ui::makeCell(b.id());
        idItem->setData(Qt::UserRole, b.id());
        QFont bold = idItem->font();
        bold.setBold(true);
        idItem->setFont(bold);
        m_table->setItem(r, 0, idItem);
        m_table->setItem(r, 1, ui::makeCell(b.title()));
        m_table->setItem(r, 2, ui::makeCell(b.author()));
        m_table->setItem(r, 3, ui::makeCell(b.genre()));
        auto* copies = ui::makeNumberCell(b.copies());
        if (b.copies() == 0) {
            copies->setForeground(QColor("#C0392B"));
            copies->setToolTip("Out of stock");
        } else if (b.copies() <= 2) {
            copies->setForeground(QColor("#B7791F"));
            copies->setToolTip("Low stock");
        }
        QFont f = copies->font();
        f.setBold(true);
        copies->setFont(f);
        m_table->setItem(r, 4, copies);
        m_table->setItem(r, 5, ui::makeCell(b.location()));
        if (b.id() == keepSelected) reselect = r;
    }
    m_table->setSortingEnabled(true);
    if (reselect >= 0) m_table->selectRow(reselect);

    const int n = books.size();
    m_count->setText(QString("%1 book%2").arg(n).arg(n == 1 ? "" : "s"));
    updateButtons();
}

void BooksPage::updateButtons() {
    const bool has = ui::selectedRow(m_table) >= 0;
    for (QPushButton* b : {m_edit, m_delete, m_addCopies, m_removeCopies}) b->setEnabled(has);
}

void BooksPage::addBook() {
    QStringList genres;
    if (!ui::guard(this, [&] { genres = m_svc.genres(); })) return;
    BookDialog dialog(genres, [this](const Book& b) { return m_svc.addBook(b); }, this);
    if (dialog.exec() == QDialog::Accepted) {
        emit notify("Book added successfully.", Toast::Success);
        reload();
    }
}

void BooksPage::editBook() {
    const QString id = ui::selectedKey(m_table);
    if (id.isEmpty()) return;
    QStringList genres;
    std::optional<Book> existing;
    if (!ui::guard(this, [&] {
            genres = m_svc.genres();
            const auto list = m_svc.books(id, BookField::Id);
            for (const Book& b : list)
                if (b.id().compare(id, Qt::CaseInsensitive) == 0) existing = b;
        }))
        return;
    if (!existing) {
        emit notify("This book no longer exists.", Toast::Error);
        reload();
        return;
    }
    BookDialog dialog(genres, [this](const Book& b) { return m_svc.updateBook(b); }, this, existing);
    if (dialog.exec() == QDialog::Accepted) {
        emit notify("Book updated successfully.", Toast::Success);
        reload();
    }
}

void BooksPage::deleteBook() {
    const int row = ui::selectedRow(m_table);
    if (row < 0) return;
    const QString id = ui::selectedKey(m_table);
    const QString title = m_table->item(row, 1)->text();

    DeleteInfo info;
    if (!ui::guard(this, [&] { info = m_svc.bookDeleteInfo(id); })) return;

    if (info.openLoans > 0) {
        QMessageBox::warning(this, "Cannot delete",
                             QString("\"%1\" is currently borrowed (%2 open loan%3).\nReturn it first, then delete it.")
                                 .arg(title).arg(info.openLoans).arg(info.openLoans == 1 ? "" : "s"));
        return;
    }
    QString text = QString("Delete \"%1\" (ID %2) from the library?").arg(title, id);
    if (info.totalLoans > 0)
        text += QString("\n\nIts %1 past borrowing record%2 will be deleted as well.")
                    .arg(info.totalLoans).arg(info.totalLoans == 1 ? "" : "s");
    if (QMessageBox::question(this, "Delete book", text, QMessageBox::Yes | QMessageBox::Cancel,
                              QMessageBox::Cancel) != QMessageBox::Yes)
        return;

    const Result r = m_svc.deleteBook(id, true);
    emit notify(r.ok ? "Book deleted." : r.message, r.ok ? Toast::Success : Toast::Error);
    reload();
}

void BooksPage::changeCopies(bool add) {
    const int row = ui::selectedRow(m_table);
    if (row < 0) return;
    const QString id = ui::selectedKey(m_table);
    const QString title = m_table->item(row, 1)->text();
    const int current = m_table->item(row, 4)->data(Qt::DisplayRole).toInt();

    bool ok = false;
    const int max = add ? Book::kMaxCopies - current : current;
    if (max <= 0) {
        emit notify(add ? "The maximum number of copies was reached." : "There are no copies to remove.",
                    Toast::Error);
        return;
    }
    const int n = QInputDialog::getInt(this, add ? "Add copies" : "Remove copies",
                                       QString("\"%1\" currently has %2 cop%3.\n%4")
                                           .arg(title).arg(current).arg(current == 1 ? "y" : "ies")
                                           .arg(add ? "How many copies do you want to add?"
                                                    : "How many copies do you want to remove?"),
                                       1, 1, max, 1, &ok);
    if (!ok) return;
    const Result r = m_svc.changeCopies(id, add ? n : -n);
    emit notify(r.message, r.ok ? Toast::Success : Toast::Error);
    reload();
}
