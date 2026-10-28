#pragma once
#include "Page.h"
#include <QTimer>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Book catalogue: searchable table + add / edit / delete / change number of copies.
class BooksPage : public Page {
    Q_OBJECT
public:
    explicit BooksPage(LibraryService& service, QWidget* parent = nullptr);
    void refresh() override;

private slots:
    void addBook();
    void editBook();
    void deleteBook();
    void changeCopies(bool add);

private:
    void reload();
    void updateButtons();

    QLineEdit* m_search;
    QComboBox* m_field;
    QTableWidget* m_table;
    QLabel* m_count;
    QPushButton *m_edit, *m_delete, *m_addCopies, *m_removeCopies;
    QTimer m_debounce;
};
