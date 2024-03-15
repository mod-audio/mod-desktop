// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mod-desktop.hpp"
#include "qrc_mod-desktop.hpp"

// TODO split build
#include "utils.cpp"

int main(int argc, char* argv[])
{
    QApplication::setAttribute(Qt::AA_X11InitThreads);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    initEvironment();
    setupControlCloseSignal();

    // TODO set up all branding here
    QApplication app(argc, argv);
    app.setApplicationName("MOD Desktop");
    app.setOrganizationName("MOD Audio");
    app.setWindowIcon(QIcon(":/res/mod-logo.svg"));

    const bool darkMode = shouldUseDarkMode();

    if (darkMode)
        setupDarkModePalette(app);

    AppWindow window;

    if (darkMode)
        setupDarkModeWindow(window);

    return app.exec();
}
