#pragma once
#include "models/Book.h"
#include "services/Result.h"

#include <QDialog>
#include <functional>
#include <optional>

class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

// Form used both to add a new book and to edit an existing one.
class BookDialog : public QDialog {
    Q_OBJECT
public:
    // `submit` saves the book and tells whether it worked; the dialog stays open on failure
    // and shows the message under the form.
    using Submit = std::function<Result(const Book&)>;

    BookDialog(const QStringList& knownGenres, Submit submit, QWidget* parent,
               const std::optional<Book>& existing = std::nullopt);

private slots:
    void onSave();

private:
    Submit m_submit;
    bool m_editing;
    QLineEdit *m_id, *m_title, *m_author, *m_location;
    QComboBox* m_genre;
    QSpinBox* m_copies;
    QLabel* m_error;
};
