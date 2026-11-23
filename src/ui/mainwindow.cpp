// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/mainwindow.h"

#include "core/version.h"
#include "ui/bitmaptextrenderer.h"
#include "ui/systemfonttextrenderer.h"

#include <QPaintEvent>
#include <QPainter>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string_view>

namespace qk::ui {

namespace {
constexpr int kDefaultWidth = 120;
constexpr int kDefaultHeight = 48;

/// Formats "CPU nnn%" (or "CPU --%" with no data) into `buffer` without allocating.
QStringView formatCpu(std::array<char16_t, 16>& buffer, std::optional<float> fraction)
{
    std::size_t n = 0;
    for (const char16_t c : std::u16string_view(u"CPU ")) {
        buffer.at(n++) = c;
    }
    if (!fraction) {
        buffer.at(n++) = u'-';
        buffer.at(n++) = u'-';
    } else {
        const int percent = std::clamp(static_cast<int>(std::lround(*fraction * 100.0F)), 0, 100);
        std::array<char16_t, 3> digits{};
        std::size_t count = 0;
        for (int value = percent; count == 0 || value > 0; value /= 10) {
            digits.at(count++) = static_cast<char16_t>(u'0' + (value % 10));
        }
        while (count > 0) {
            buffer.at(n++) = digits.at(--count);
        }
    }
    buffer.at(n++) = u'%';
    return {buffer.data(), static_cast<qsizetype>(n)};
}

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

MainWindow::MainWindow(const core::MetricRegistry& registry, core::MetricHandle cpuBusy,
                       QWidget* parent)
    : QWidget(parent)
    , m_registry(&registry)
    , m_cpuBusy(cpuBusy)
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
    const QColor ink = palette().windowText().color();
    const int line = m_text->lineHeight();
    const int top = (height() - (2 * line)) / 2;
    m_text->draw(painter, QPoint((width() - m_text->horizontalAdvance(kLabel)) / 2, top), kLabel,
                 ink);

    const auto& history = m_registry->history(m_cpuBusy);
    std::array<char16_t, 16> buffer{};
    const QStringView cpu =
        formatCpu(buffer, history.empty() ? std::nullopt : std::optional(history.latest()));
    m_text->draw(painter, QPoint((width() - m_text->horizontalAdvance(cpu)) / 2, top + line), cpu,
                 ink);
    painter.end();

    if (!m_painted) {
        m_painted = true;
        emit firstPaintDone();
    }
}

} // namespace qk::ui
