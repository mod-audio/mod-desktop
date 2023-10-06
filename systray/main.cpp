// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mod-app.hpp"
#include "qrc_mod-app.hpp"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#ifdef _WIN32
#include <shlobj.h>
static const WCHAR* user_files_dir = nullptr;
#else
#include <dlfcn.h>
#endif

QString getUserFilesDir()
{
   #ifdef _WIN32
    if (user_files_dir != nullptr)
        return QString::fromWCharArray(user_files_dir);
   #else
    // TODO
   #endif

    return {};
}

void writeMidiChannelsToProfile(int pedalboard, int snapshot)
{
   #ifdef _WIN32
    WCHAR path[MAX_PATH + 256] = {};
    GetEnvironmentVariableW(L"MOD_DATA_DIR", path, sizeof(path)/sizeof(path[0]));
    std::wcscat(path, L"\\profile5.json");
    QFile jsonFile(QString::fromWCharArray(path));
   #else
    char path[MAX_PATH + 256] = {};
    std::strcpy(path, getenv("MOD_DATA_DIR"));
    std::strcat(path, "/profile5.json");
    QFile jsonFile(QString::fromUtf8(path));
   #endif

    const bool exists = jsonFile.exists();

    if (! jsonFile.open(QIODevice::ReadWrite|QIODevice::Truncate|QIODevice::Text))
        return;

    QJsonObject jsonObj;

    if (exists)
        jsonObj = QJsonDocument::fromJson(jsonFile.readAll()).object();

    jsonObj["midiChannelForPedalboardsNavigation"] = pedalboard;
    jsonObj["midiChannelForSnapshotsNavigation"] = snapshot;

    jsonFile.seek(0);
    jsonFile.write(QJsonDocument(jsonObj).toJson());
}

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
