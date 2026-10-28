#pragma once
#include "Page.h"
#include <QTimer>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Borrow & return workflow: choose a student + book to lend, and return borrowed books.
class LoansPage : public Page {
    Q_OBJECT
public:
    explicit LoansPage(LibraryService& service, QWidget* parent = nullptr);
    void refresh() override;

private slots:
    void borrow();
    void returnSelected();

private:
    void reloadChoices();
    void reloadLoans();
    static QString idOfChoice(QComboBox* combo);   // id of the chosen item ("" if the text is not a list item)

    QComboBox *m_student, *m_book;
    QLabel *m_dueInfo, *m_count;
    QPushButton *m_borrow, *m_return;
    QLineEdit* m_search;
    QTableWidget* m_table;
    QTimer m_debounce;
};
