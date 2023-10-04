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

    const QString cwd(QString::fromWCharArray(wc));
    SetCurrentDirectoryW(wc);

    WCHAR lv2path[(MAX_PATH + 256) * 2] = {};
    std::wcscat(lv2path, wc);
    std::wcscat(lv2path, L"\\data\\lv2;");
    std::wcscat(lv2path, wc);
    std::wcscat(lv2path, L"\\plugins");
    SetEnvironmentVariableW(L"LV2_PATH", lv2path);
   #else
    // TODO
    const QString cwd;
   #endif

    app.setQuitOnLastWindowClosed(false);

    AppWindow window(cwd);
    return app.exec();
}
