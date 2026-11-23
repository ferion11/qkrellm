// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/mainwindow.h"

#include "core/version.h"
#include "ui/bitmaptextrenderer.h"
#include "ui/systemfonttextrenderer.h"

#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>

#include <utility>

namespace qk::ui {

namespace {
constexpr int kFrame = 1;   // logical px border around the stack
constexpr int kSpacing = 1; // logical px between panels

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

MainWindow::MainWindow(const core::MetricRegistry& registry, QWidget* parent)
    : QWidget(parent)
    , m_registry(&registry)
    , m_text(makeTextRenderer(devicePixelRatioF()))
    , m_layout(new QVBoxLayout(this))
{
    setWindowTitle(QStringLiteral("qkrellm %1").arg(core::versionString()));
    // We paint every pixel ourselves: skip Qt's background erase.
    setAttribute(Qt::WA_OpaquePaintEvent);
    m_layout->setContentsMargins(kFrame, kFrame, kFrame, kFrame);
    m_layout->setSpacing(kSpacing);
    m_layout->setSizeConstraint(QLayout::SetFixedSize);
}

MainWindow::~MainWindow() = default;

Panel* MainWindow::addPanel(PanelSpec spec)
{
    auto* panel = new Panel(*m_registry, std::move(spec), *m_text, m_theme, this);
    m_layout->addWidget(panel);
    m_panels.push_back(panel);
    return panel;
}

void MainWindow::refresh()
{
    for (Panel* panel : m_panels) {
        panel->refresh();
    }
}

void MainWindow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), m_theme.chartGrid); // frame + gaps between panels
    painter.end();

    if (!m_painted) {
        m_painted = true;
        emit firstPaintDone();
    }
}

} // namespace qk::ui
