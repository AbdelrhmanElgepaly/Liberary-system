#include "Page.h"
#include "UiUtil.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

QVBoxLayout* Page::makePageLayout() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(18);
    return layout;
}

QWidget* Page::makeHeader(const QString& title, const QString& subtitle, QWidget* rightWidget) {
    auto* header = new QWidget(this);
    auto* row = new QHBoxLayout(header);
    row->setContentsMargins(0, 0, 0, 0);

    auto* texts = new QVBoxLayout;
    texts->setSpacing(2);
    texts->addWidget(ui::makeLabel(title, "pageTitle"));
    texts->addWidget(ui::makeLabel(subtitle, "pageSubtitle"));
    row->addLayout(texts, 1);

    if (rightWidget) row->addWidget(rightWidget, 0, Qt::AlignVCenter);
    return header;
}
