// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "ui/panel.h"
#include "ui/theme.h"

#include <QWidget>

#include <memory>
#include <vector>

class QVBoxLayout;

namespace qk::ui {

class TextRenderer;

/// Top-level krell stack: a vertical column of panels inside a 1 px frame.
class MainWindow final : public QWidget
{
    Q_OBJECT

public:
    /// `registry` must outlive the window.
    explicit MainWindow(const core::MetricRegistry& registry, QWidget* parent = nullptr);
    ~MainWindow() override;
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    /// Appends a panel at the bottom of the stack (setup time).
    Panel* addPanel(PanelSpec spec);

    [[nodiscard]] const Theme& theme() const { return m_theme; }

public slots:
    /// Repaints every panel; connect to Sampler::sampled.
    void refresh();

signals:
    /// Emitted once, from the first paintEvent of the window (used by the startup benchmark,
    /// which waits for the event loop to finish the whole frame before reading the clock).
    void firstPaintDone();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    const core::MetricRegistry* m_registry;
    Theme m_theme;
    std::unique_ptr<TextRenderer> m_text;
    QVBoxLayout* m_layout;        // owned by this widget
    std::vector<Panel*> m_panels; // children, owned by Qt
    bool m_painted = false;
};

} // namespace qk::ui
