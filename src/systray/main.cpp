// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mod-desktop-app.hpp"
#include "qrc_mod-desktop-app.hpp"

// TODO split build
#include "utils.cpp"

int main(int argc, char* argv[])
{
    initEvironment();
    setupControlCloseSignal();

    // TODO set up all branding here
    QApplication app(argc, argv);
    app.setApplicationName("MOD Desktop App");
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
