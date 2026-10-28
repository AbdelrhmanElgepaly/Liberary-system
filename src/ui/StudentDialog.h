#pragma once
#include "models/Student.h"
#include "services/Result.h"

#include <QDialog>
#include <functional>
#include <optional>

class QLabel;
class QLineEdit;

// Form used both to register a new student and to edit an existing one.
class StudentDialog : public QDialog {
    Q_OBJECT
public:
    using Submit = std::function<Result(const Student&)>;

    StudentDialog(Submit submit, QWidget* parent, const std::optional<Student>& existing = std::nullopt);

private slots:
    void onSave();

private:
    Submit m_submit;
    QLineEdit *m_id, *m_name, *m_department, *m_phone, *m_email;
    QLabel* m_error;
};
