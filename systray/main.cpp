// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mod-app.hpp"
#include "qrc_mod-app.hpp"

#ifndef _WIN32
#include <dlfcn.h>
#endif

int main(int argc, char* argv[])
{
    // TODO set branding here

    QApplication app(argc, argv);
    app.setApplicationName("MOD-App");
    app.setOrganizationName("MOD Audio");
    app.setWindowIcon(QIcon(":/mod-logo.svg"));

    if (! QSystemTrayIcon::isSystemTrayAvailable())
    {
        return 1;
    }

   #ifdef _WIN32
    WCHAR wc[MAX_PATH + 256] = {};
    GetModuleFileNameW(GetModuleHandleW(nullptr), wc, sizeof(wc)/sizeof(wc[0]));

    if (wchar_t* const wcc = wcsrchr(wc, '\\'))
        *wcc = 0;

    SetCurrentDirectoryW(wc);

    const QString cwd(QString::fromWCharArray(wc));

    std::wcscat(wc, L"\\plugins");
    SetEnvironmentVariableW(L"LV2_PATH", wc);
   #else
    // TODO
    const QString cwd;
   #endif

    app.setQuitOnLastWindowClosed(false);

    AppWindow window(cwd);
    return app.exec();
}
