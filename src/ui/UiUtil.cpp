#include "UiUtil.h"

#include <QGraphicsDropShadowEffect>
#include <QHeaderView>

namespace ui {

QIcon icon(const QString& name, const QString& variant) {
    return QIcon(QString(":/icons/%1_%2.png").arg(name, variant));
}

QPixmap iconPixmap(const QString& name, const QString& variant, int size) {
    return QPixmap(QString(":/icons/%1_%2.png").arg(name, variant))
        .scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPushButton* makeButton(const QString& text, const QString& iconName, const QString& kind, QWidget* parent) {
    auto* b = new QPushButton(text, parent);
    b->setProperty("kind", kind);
    b->setCursor(Qt::PointingHandCursor);
    if (!iconName.isEmpty()) {
        const bool onColour = kind == "primary" || kind == "danger";
        b->setIcon(icon(iconName, onColour ? "white" : "dark"));
        b->setIconSize(QSize(18, 18));
    }
    return b;
}

QLabel* makeLabel(const QString& text, const char* objectName, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setObjectName(objectName);
    return l;
}

QFrame* makeCard(QWidget* parent) {
    auto* f = new QFrame(parent);
    f->setObjectName("card");
    auto* shadow = new QGraphicsDropShadowEffect(f);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(60, 40, 20, 34));
    f->setGraphicsEffect(shadow);
    return f;
}

QTableWidget* makeTable(const QStringList& headers, int sortColumn, Qt::SortOrder order, QWidget* parent) {
    auto* t = new QTableWidget(0, headers.size(), parent);
    t->setHorizontalHeaderLabels(headers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setAlternatingRowColors(true);
    t->setShowGrid(false);
    t->setFrameShape(QFrame::NoFrame);
    t->setWordWrap(false);
    t->setTextElideMode(Qt::ElideRight);
    t->verticalHeader()->setVisible(false);
    t->verticalHeader()->setDefaultSectionSize(46);
    t->horizontalHeader()->setHighlightSections(false);
    t->horizontalHeader()->setStretchLastSection(true);
    t->horizontalHeader()->setMinimumSectionSize(70);
    t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    t->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    t->sortByColumn(sortColumn, order);
    t->setSortingEnabled(true);
    return t;
}

QTableWidgetItem* makeCell(const QString& text, Qt::Alignment align) {
    auto* item = new QTableWidgetItem(text);
    item->setTextAlignment(align);
    item->setToolTip(text);   // full text when the cell is too narrow
    return item;
}

QTableWidgetItem* makeNumberCell(int value) {
    auto* item = new QTableWidgetItem;
    item->setData(Qt::DisplayRole, value);  // numeric, so the column sorts as numbers
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

namespace {
class DateItem : public QTableWidgetItem {
public:
    explicit DateItem(const QDate& d) : QTableWidgetItem(dateText(d)) {
        setData(Qt::UserRole + 1, d);
        setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    bool operator<(const QTableWidgetItem& other) const override {
        return data(Qt::UserRole + 1).toDate() < other.data(Qt::UserRole + 1).toDate();
    }
};
}  // namespace

QTableWidgetItem* makeDateCell(const QDate& date) { return new DateItem(date); }

int selectedRow(const QTableWidget* table) {
    const auto rows = table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

QString selectedKey(const QTableWidget* table) {
    const int row = selectedRow(table);
    if (row < 0 || !table->item(row, 0)) return QString();
    return table->item(row, 0)->data(Qt::UserRole).toString();
}

QString dateText(const QDate& date) {
    return date.isValid() ? date.toString("dd MMM yyyy") : QString("-");
}

}  // namespace ui
