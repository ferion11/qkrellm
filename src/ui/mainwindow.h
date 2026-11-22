// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

namespace qk::ui {

/// Top-level krell stack. M0: a placeholder that paints the app name.
class MainWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

signals:
    /// Emitted once, after the first paintEvent completes (used by the startup benchmark).
    void firstPaintDone();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    bool m_painted = false;
};

} // namespace qk::ui
