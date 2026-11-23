// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

#include <memory>

namespace qk::ui {

class TextRenderer;

/// Top-level krell stack. M1: a placeholder that paints the app name.
class MainWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

signals:
    /// Emitted once, after the first paintEvent completes (used by the startup benchmark).
    void firstPaintDone();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::unique_ptr<TextRenderer> m_text;
    bool m_painted = false;
};

} // namespace qk::ui
