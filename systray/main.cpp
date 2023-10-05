// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mod-app.hpp"
#include "qrc_mod-app.hpp"

#ifdef _WIN32
#include <shlobj.h>
const WCHAR* user_files_dir = nullptr;
#else
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

    WCHAR path[(MAX_PATH + 256) * 2] = {};
    std::wcscat(path, wc);
    std::wcscat(path, L"\\data\\lv2;");
    std::wcscat(path, wc);
    std::wcscat(path, L"\\plugins");
    SetEnvironmentVariableW(L"LV2_PATH", path);

    if (SHGetSpecialFolderPathW(nullptr, path, CSIDL_MYDOCUMENTS, FALSE))
    {
        std::wcscat(path, L"\\mod-app");
        _wmkdir(path);
        SetEnvironmentVariableW(L"MOD_DATA_DIR", path);

        std::wcscat(path, L"\\user-files");
        _wmkdir(path);
        user_files_dir = path;
    }
   #else
    // TODO
    const QString cwd;
   #endif

    app.setQuitOnLastWindowClosed(false);

    AppWindow window(cwd);
    return app.exec();
}
