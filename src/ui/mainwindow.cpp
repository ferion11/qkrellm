// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/mainwindow.h"

#include "core/version.h"
#include "ui/bitmaptextrenderer.h"
#include "ui/systemfonttextrenderer.h"

#include <QPaintEvent>
#include <QPainter>

namespace qk::ui {

namespace {
constexpr int kDefaultWidth = 120;
constexpr int kDefaultHeight = 48;

std::unique_ptr<TextRenderer> makeTextRenderer(qreal devicePixelRatio)
{
    if (auto bitmap =
            BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), devicePixelRatio)) {
        return bitmap;
    }
    // Only reachable with a broken build (resources missing): degrade, don't die.
    qWarning("qkrellm: bitmap font atlas unavailable, falling back to system fonts");
    return std::make_unique<SystemFontTextRenderer>(QFont());
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
    , m_text(makeTextRenderer(devicePixelRatioF()))
{
    setWindowTitle(QStringLiteral("qkrellm %1").arg(core::versionString()));
    // We paint every pixel ourselves: skip Qt's background erase.
    setAttribute(Qt::WA_OpaquePaintEvent);
    resize(kDefaultWidth, kDefaultHeight);
}

MainWindow::~MainWindow() = default;

void MainWindow::paintEvent(QPaintEvent* event)
{
    static constexpr QStringView kLabel = u"qkrellm";

    QPainter painter(this);
    painter.fillRect(event->rect(), palette().window());
    const QPoint topLeft((width() - m_text->horizontalAdvance(kLabel)) / 2,
                         (height() - m_text->lineHeight()) / 2);
    m_text->draw(painter, topLeft, kLabel, palette().windowText().color());
    painter.end();

    if (!m_painted) {
        m_painted = true;
        emit firstPaintDone();
    }
}

} // namespace qk::ui
