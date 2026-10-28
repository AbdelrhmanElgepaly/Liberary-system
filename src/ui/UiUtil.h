#pragma once
#include "db/DatabaseManager.h"

#include <QDate>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

// Small helpers shared by all pages so every screen looks and behaves the same.
namespace ui {

// Icons live in the resources (":/icons/<name>_<variant>.png"); variant = white | dark | gold.
QIcon icon(const QString& name, const QString& variant = "dark");
QPixmap iconPixmap(const QString& name, const QString& variant, int size);

// kind = "primary" | "secondary" | "danger"
QPushButton* makeButton(const QString& text, const QString& iconName = QString(),
                        const QString& kind = "secondary", QWidget* parent = nullptr);

QLabel* makeLabel(const QString& text, const char* objectName, QWidget* parent = nullptr);

// White rounded panel with a soft shadow.
QFrame* makeCard(QWidget* parent = nullptr);

// `sortColumn`/`order` = how the rows are sorted by default (the user can click a header to change it).
QTableWidget* makeTable(const QStringList& headers, int sortColumn = 0,
                        Qt::SortOrder order = Qt::AscendingOrder, QWidget* parent = nullptr);
QTableWidgetItem* makeCell(const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter);
QTableWidgetItem* makeNumberCell(int value);
// Shows the date as text but sorts by the real date.
QTableWidgetItem* makeDateCell(const QDate& date);

// The id stored with the first cell of the currently selected row ("" when nothing is selected).
QString selectedKey(const QTableWidget* table);
int selectedRow(const QTableWidget* table);

QString dateText(const QDate& date);

// Runs `fn` and shows a message box if the database raises an error.
template <typename F>
bool guard(QWidget* parent, F&& fn) {
    try {
        fn();
        return true;
    } catch (const DbException& e) {
        QMessageBox::critical(parent, "Database error", e.message());
        return false;
    }
}

}  // namespace ui
