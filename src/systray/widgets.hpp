// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QtCore/QTimer>
#include <QtWidgets/QFrame>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

class QToolButtonWithMouseTracking : public QToolButton
{
    QFont _font;

public:
    QToolButtonWithMouseTracking(QWidget* const parent)
        : QToolButton(parent) {}

    void enterEvent(QEvent* const event) override
    {
        QFont _font = font();
        _font.setBold(true);
        setFont(_font);
        QToolButton::enterEvent(event);
    }

    void leaveEvent(QEvent* const event) override
    {
        QFont _font = font();
        _font.setBold(false);
        setFont(_font);
        QToolButton::leaveEvent(event);
    }
};

static inline
QWidget* getFullParent(QWidget* const w)
{
    if (QWidget* const p = dynamic_cast<QWidget*>(w->parent()))
        return getFullParent(p);

    return w;
}

class CollapsibleWidget : public QWidget
{
    QToolButtonWithMouseTracking* toggleButton = nullptr;

public:
    CollapsibleWidget(QWidget* const parent)
        : QWidget(parent) {}

    void setCheckedInit(const bool checked)
    {
        toggleButton->blockSignals(true);
        toggleButton->setChecked(checked);
        toggleButton->blockSignals(false);

        toggleButton->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
        setVisible(checked);
    }

    void setup(QToolButtonWithMouseTracking* const tb)
    {
        hide();
        toggleButton = tb;
        connect(toggleButton, &QToolButton::toggled, this, &CollapsibleWidget::toolButtonPressed);
    }

private slots:
    void reduceSize()
    {
        QWidget* const p = getFullParent(this);
        p->resize(p->width(), p->height() - height());
    }

    void toolButtonPressed(const bool toggled)
    {
        toggleButton->setArrowType(toggled ? Qt::DownArrow : Qt::RightArrow);
        setVisible(toggled);

        if (!toggled)
        {
            QTimer::singleShot(20, this, &CollapsibleWidget::reduceSize);
        }
    }
};
