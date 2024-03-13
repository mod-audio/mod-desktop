// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "DistrhoPluginInfo.h"

#include "../systray/qrc_mod-desktop.hpp"

// TODO split build
#include "../systray/utils.cpp"

#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWebEngineWidgets/QWebEngineView>

// -----------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    QApplication::setAttribute(Qt::AA_X11InitThreads);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    setupControlCloseSignal();

    // TODO set up all branding here
    QApplication app(argc, argv);
    app.setApplicationName("MOD Desktop");
    app.setOrganizationName("MOD Audio");
    app.setWindowIcon(QIcon(":/res/mod-logo.svg"));

    const bool darkMode = shouldUseDarkMode();

    if (darkMode)
        setupDarkModePalette(app);

    printf("'%s' '%s'\n", argv[1], argv[2]);

    if (argc == 3 && std::strcmp(argv[1], "-xembed") == 0)
    {
        const uintptr_t parentId = std::atoll(argv[2]);
        QWindow* const parentWindow = QWindow::fromWinId(parentId);

        QWebEngineView webview;
        webview.move(0, kVerticalOffset);
        webview.setFixedSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset);
        webview.winId();
        webview.windowHandle()->setParent(parentWindow);
        webview.setUrl(QUrl("http://127.0.0.1:18181/"));
        webview.show();
        return app.exec();
    }
    else
    {
        QMainWindow window;
        window.setWindowTitle("Web View");

        QWebEngineView webview(&window);
        webview.setUrl(QUrl("http://127.0.0.1:18181/"));

        window.setCentralWidget(&webview);
        window.resize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset);

        if (darkMode)
            setupDarkModeWindow(window);

        window.show();
        return app.exec();
    }
}

// -----------------------------------------------------------------------------------------------------------
