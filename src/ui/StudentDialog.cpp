#include "StudentDialog.h"
#include "UiUtil.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

StudentDialog::StudentDialog(Submit submit, QWidget* parent, const std::optional<Student>& existing)
    : QDialog(parent), m_submit(std::move(submit)) {
    const bool editing = existing.has_value();
    setWindowTitle(editing ? "Edit student" : "Add student");
    setModal(true);
    setMinimumWidth(480);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 22);
    root->setSpacing(16);
    root->addWidget(ui::makeLabel(editing ? "Edit student" : "Register a student", "dialogTitle"));
    root->addWidget(ui::makeLabel(editing ? "Change the details and press Save."
                                          : "Fill in the details of the student.", "dialogSubtitle"));

    m_id = new QLineEdit;
    m_id->setPlaceholderText("e.g. 20230101");
    m_name = new QLineEdit;
    m_department = new QLineEdit;
    m_phone = new QLineEdit;
    m_phone->setPlaceholderText("optional");
    m_email = new QLineEdit;
    m_email->setPlaceholderText("optional");

    auto* form = new QFormLayout;
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(12);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->addRow("Student ID", m_id);
    form->addRow("Full name", m_name);
    form->addRow("Department", m_department);
    form->addRow("Phone", m_phone);
    form->addRow("Email", m_email);
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
    connect(save, &QPushButton::clicked, this, &StudentDialog::onSave);

    if (existing) {
        m_id->setText(existing->id());
        m_id->setReadOnly(true);
        m_name->setText(existing->fullName());
        m_department->setText(existing->department());
        m_phone->setText(existing->phone());
        m_email->setText(existing->email());
        m_name->setFocus();
    } else {
        m_id->setFocus();
    }
}

void StudentDialog::onSave() {
    const Student s(m_id->text(), m_name->text(), m_department->text(), m_phone->text(), m_email->text());
    const Result r = m_submit(s);
    if (r.ok) {
        accept();
        return;
    }
    m_error->setText(r.message);
    m_error->show();
}
