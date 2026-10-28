#pragma once
#include "Page.h"
#include <QTimer>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Student management: searchable table + add / edit / delete.
class StudentsPage : public Page {
    Q_OBJECT
public:
    explicit StudentsPage(LibraryService& service, QWidget* parent = nullptr);
    void refresh() override;

private slots:
    void addStudent();
    void editStudent();
    void deleteStudent();

private:
    void reload();
    void updateButtons();

    QLineEdit* m_search;
    QTableWidget* m_table;
    QLabel* m_count;
    QPushButton *m_edit, *m_delete;
    QTimer m_debounce;
};
