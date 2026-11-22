// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/startupprobe.h"
#include "core/version.h"
#include "ui/mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

#include <cstdio>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qkrellm"));
    QApplication::setApplicationVersion(qk::core::versionString());
    QApplication::setDesktopFileName(QStringLiteral("io.github.ferion11.qkrellm"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Lightweight Qt6 system monitor"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption exitAfterFirstPaint(
        QStringLiteral("exit-after-first-paint"),
        QStringLiteral("Print 'first-paint-ms=<n>' and quit after the first paint (benchmarks)."));
    parser.addOption(exitAfterFirstPaint);
    parser.process(app);

    qk::ui::MainWindow window;

    if (parser.isSet(exitAfterFirstPaint)) {
        QObject::connect(&window, &qk::ui::MainWindow::firstPaintDone, &app, [] {
            const auto ms = qk::app::msSinceProcessStart();
            std::printf("first-paint-ms=%lld\n", ms.value_or(-1));
            std::fflush(stdout);
            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        });
    }

    window.show();
    return QApplication::exec();
}
