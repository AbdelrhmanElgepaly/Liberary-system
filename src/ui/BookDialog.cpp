#include "BookDialog.h"
#include "UiUtil.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

BookDialog::BookDialog(const QStringList& knownGenres, Submit submit, QWidget* parent,
                       const std::optional<Book>& existing)
    : QDialog(parent), m_submit(std::move(submit)), m_editing(existing.has_value()) {
    setWindowTitle(m_editing ? "Edit book" : "Add book");
    setModal(true);
    setMinimumWidth(480);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 22);
    root->setSpacing(16);
    root->addWidget(ui::makeLabel(m_editing ? "Edit book" : "Add a new book", "dialogTitle"));
    root->addWidget(ui::makeLabel(m_editing ? "Change the details and press Save."
                                            : "Fill in the details of the book.", "dialogSubtitle"));

    m_id = new QLineEdit;
    m_id->setPlaceholderText("e.g. B101");
    m_title = new QLineEdit;
    m_author = new QLineEdit;
    m_genre = new QComboBox;
    m_genre->setEditable(true);
    m_genre->addItems(knownGenres);
    m_genre->setCurrentIndex(-1);
    m_genre->lineEdit()->setPlaceholderText("Choose or type a genre");
    m_copies = new QSpinBox;
    m_copies->setRange(0, Book::kMaxCopies);
    m_copies->setValue(1);
    m_location = new QLineEdit;
    m_location->setPlaceholderText("e.g. A1-03");

    auto* form = new QFormLayout;
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(12);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->addRow("Book ID", m_id);
    form->addRow("Title", m_title);
    form->addRow("Author", m_author);
    form->addRow("Genre", m_genre);
    form->addRow("Copies", m_copies);
    form->addRow("Location", m_location);
    root->addLayout(form);

    m_error = ui::makeLabel(QString(), "errorLabel");
    m_error->setWordWrap(true);
    m_error->hide();
    root->addWidget(m_error);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancel = ui::makeButton("Cancel");
    auto* save = ui::makeButton("Save", "check", "primary");
    save->setDefault(true);
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    root->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(save, &QPushButton::clicked, this, &BookDialog::onSave);

    if (existing) {
        m_id->setText(existing->id());
        m_id->setReadOnly(true);   // the ID is the primary key and cannot change
        m_title->setText(existing->title());
        m_author->setText(existing->author());
        m_genre->setCurrentText(existing->genre());
        m_copies->setValue(existing->copies());
        m_location->setText(existing->location());
        m_title->setFocus();
    } else {
        m_id->setFocus();
    }
}

void BookDialog::onSave() {
    const Book book(m_id->text(), m_title->text(), m_author->text(), m_genre->currentText(),
                    m_copies->value(), m_location->text());
    const Result r = m_submit(book);
    if (r.ok) {
        accept();
        return;
    }
    m_error->setText(r.message);
    m_error->show();
}
