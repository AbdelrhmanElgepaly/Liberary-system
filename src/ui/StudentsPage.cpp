#include "StudentsPage.h"
#include "StudentDialog.h"
#include "UiUtil.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

StudentsPage::StudentsPage(LibraryService& service, QWidget* parent) : Page(service, parent) {
    auto* layout = makePageLayout();

    auto* addButton = ui::makeButton("Add Student", "plus", "primary");
    layout->addWidget(makeHeader("Students", "Library members who can borrow books.", addButton));

    auto* card = ui::makeCard();
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(14);

    auto* searchRow = new QHBoxLayout;
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search by ID, name, department, phone or email...");
    m_search->setClearButtonEnabled(true);
    m_search->addAction(ui::icon("search", "dark"), QLineEdit::LeadingPosition);
    m_search->setMinimumWidth(380);
    m_count = ui::makeLabel(QString(), "mutedLabel");
    searchRow->addWidget(m_search, 1);
    searchRow->addStretch(2);
    searchRow->addWidget(m_count);
    cardLayout->addLayout(searchRow);

    m_table = ui::makeTable({"Student ID", "Full name", "Department", "Phone", "Email", "Books out"}, 1);
    m_table->setColumnWidth(0, 120);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(2, 210);
    m_table->setColumnWidth(3, 140);
    m_table->setColumnWidth(4, 230);
    m_table->setColumnWidth(5, 100);
    cardLayout->addWidget(m_table, 1);

    auto* actions = new QHBoxLayout;
    actions->setSpacing(10);
    m_edit = ui::makeButton("Edit", "edit");
    m_delete = ui::makeButton("Delete", "trash", "danger");
    actions->addWidget(m_edit);
    actions->addStretch();
    actions->addWidget(m_delete);
    cardLayout->addLayout(actions);

    layout->addWidget(card, 1);

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(220);
    connect(&m_debounce, &QTimer::timeout, this, &StudentsPage::reload);
    connect(m_search, &QLineEdit::textChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(addButton, &QPushButton::clicked, this, &StudentsPage::addStudent);
    connect(m_edit, &QPushButton::clicked, this, &StudentsPage::editStudent);
    connect(m_delete, &QPushButton::clicked, this, &StudentsPage::deleteStudent);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &StudentsPage::updateButtons);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this] { editStudent(); });
    updateButtons();
}

void StudentsPage::refresh() { reload(); }

void StudentsPage::reload() {
    const QString keepSelected = ui::selectedKey(m_table);
    QVector<Student> students;
    QHash<QString, int> out;
    if (!ui::guard(this, [&] {
            students = m_svc.students(m_search->text());
            out = m_svc.openLoanCountByStudent();
        }))
        return;

    m_table->setSortingEnabled(false);
    m_table->setRowCount(students.size());
    int reselect = -1;
    for (int r = 0; r < students.size(); ++r) {
        const Student& s = students[r];
        auto* idItem = ui::makeCell(s.id());
        idItem->setData(Qt::UserRole, s.id());
        QFont bold = idItem->font();
        bold.setBold(true);
        idItem->setFont(bold);
        m_table->setItem(r, 0, idItem);
        m_table->setItem(r, 1, ui::makeCell(s.fullName()));
        m_table->setItem(r, 2, ui::makeCell(s.department()));
        m_table->setItem(r, 3, ui::makeCell(s.phone()));
        m_table->setItem(r, 4, ui::makeCell(s.email()));
        m_table->setItem(r, 5, ui::makeNumberCell(out.value(s.id().toLower(), 0)));
        if (s.id() == keepSelected) reselect = r;
    }
    m_table->setSortingEnabled(true);
    if (reselect >= 0) m_table->selectRow(reselect);

    const int n = students.size();
    m_count->setText(QString("%1 student%2").arg(n).arg(n == 1 ? "" : "s"));
    updateButtons();
}

void StudentsPage::updateButtons() {
    const bool has = ui::selectedRow(m_table) >= 0;
    m_edit->setEnabled(has);
    m_delete->setEnabled(has);
}

void StudentsPage::addStudent() {
    StudentDialog dialog([this](const Student& s) { return m_svc.addStudent(s); }, this);
    if (dialog.exec() == QDialog::Accepted) {
        emit notify("Student registered successfully.", Toast::Success);
        reload();
    }
}

void StudentsPage::editStudent() {
    const QString id = ui::selectedKey(m_table);
    if (id.isEmpty()) return;
    std::optional<Student> existing;
    if (!ui::guard(this, [&] {
            for (const Student& s : m_svc.students(id))
                if (s.id().compare(id, Qt::CaseInsensitive) == 0) existing = s;
        }))
        return;
    if (!existing) {
        emit notify("This student no longer exists.", Toast::Error);
        reload();
        return;
    }
    StudentDialog dialog([this](const Student& s) { return m_svc.updateStudent(s); }, this, existing);
    if (dialog.exec() == QDialog::Accepted) {
        emit notify("Student updated successfully.", Toast::Success);
        reload();
    }
}

void StudentsPage::deleteStudent() {
    const int row = ui::selectedRow(m_table);
    if (row < 0) return;
    const QString id = ui::selectedKey(m_table);
    const QString name = m_table->item(row, 1)->text();

    DeleteInfo info;
    if (!ui::guard(this, [&] { info = m_svc.studentDeleteInfo(id); })) return;

    if (info.openLoans > 0) {
        QMessageBox::warning(this, "Cannot delete",
                             QString("%1 still has %2 borrowed book%3.\nThe books must be returned first.")
                                 .arg(name).arg(info.openLoans).arg(info.openLoans == 1 ? "" : "s"));
        return;
    }
    QString text = QString("Delete %1 (ID %2)?").arg(name, id);
    if (info.totalLoans > 0)
        text += QString("\n\nTheir %1 past borrowing record%2 will be deleted as well.")
                    .arg(info.totalLoans).arg(info.totalLoans == 1 ? "" : "s");
    if (QMessageBox::question(this, "Delete student", text, QMessageBox::Yes | QMessageBox::Cancel,
                              QMessageBox::Cancel) != QMessageBox::Yes)
        return;

    const Result r = m_svc.deleteStudent(id, true);
    emit notify(r.ok ? "Student deleted." : r.message, r.ok ? Toast::Success : Toast::Error);
    reload();
}
