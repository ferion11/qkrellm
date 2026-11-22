// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/mainwindow.h"

#include "core/version.h"

#include <QPaintEvent>
#include <QPainter>

namespace qk::ui {

namespace {
constexpr int kDefaultWidth = 120;
constexpr int kDefaultHeight = 48;
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("qkrellm %1").arg(core::versionString()));
    // We paint every pixel ourselves: skip Qt's background erase.
    setAttribute(Qt::WA_OpaquePaintEvent);
    resize(kDefaultWidth, kDefaultHeight);
}

void MainWindow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), palette().window());
    painter.setPen(palette().windowText().color());
    painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("qkrellm"));
    painter.end();

    if (!m_painted) {
        m_painted = true;
        emit firstPaintDone();
    }
}

} // namespace qk::ui
