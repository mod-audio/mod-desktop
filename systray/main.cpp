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
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
static const char* user_files_dir = nullptr;
#endif

#if defined(__APPLE__)
#include <sys/syslimits.h>
#elif defined(__linux__)
#include <linux/limits.h>
#endif

#include <cstring>

QString getUserFilesDir()
{
    if (user_files_dir != nullptr)
    {
       #ifdef _WIN32
        return QString::fromWCharArray(user_files_dir);
       #else
        return QString::fromUtf8(user_files_dir);
       #endif
    }

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
    char path[PATH_MAX + 256] = {};
    std::strcpy(path, std::getenv("MOD_DATA_DIR"));
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

#ifndef _WIN32
static void signal(int)
{
    qApp->setQuitOnLastWindowClosed(true);
    qApp->quit();
}
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
    WCHAR path[MAX_PATH + 256] = {};

    GetModuleFileNameW(GetModuleHandleW(nullptr), path, sizeof(path)/sizeof(path[0]));

    if (wchar_t* const wcc = wcsrchr(path, '\\'))
        *wcc = 0;

    const QString cwd(QString::fromWCharArray(path));
    SetCurrentDirectoryW(path);

    WCHAR lv2path[(MAX_PATH + 256) * 2] = {};
    std::wcscat(lv2path, path);
    std::wcscat(lv2path, L"\\plugins;");

    SHGetSpecialFolderPathW(nullptr, path, CSIDL_MYDOCUMENTS, FALSE);

    std::wcscat(path, L"\\MOD App");
    _wmkdir(path);
    SetEnvironmentVariableW(L"MOD_DATA_DIR", path);

    std::wcscat(lv2path, path);
    std::wcscat(lv2path, L"\\data\\lv2;");
    SetEnvironmentVariableW(L"MOD_LV2_PATH", lv2path);

    std::wcscat(path, L"\\user-files");
    _wmkdir(path);
    user_files_dir = path;
  #else
    char path[PATH_MAX + 256] = {};

    Dl_info info = {};
    dladdr((void*)main, &info);

    if (info.dli_fname[0] == '.')
    {
        getcwd(path, sizeof(path));
        std::strcat(path, info.dli_fname + 1);
    }
    else
    {
        std::strncpy(path, info.dli_fname, sizeof(path));
    }

    if (char* const c = strrchr(path, '/'))
        *c = 0;

    const QString cwd(QString::fromUtf8(path));
    chdir(path);

    const size_t pathlen = std::strlen(path);
    std::memcpy(path + pathlen, "/jack", 6);
    setenv("JACK_DRIVER_DIR", path, 1);

    char lv2path[(PATH_MAX + 256) * 2] = {};
    std::strncat(lv2path, path, pathlen);
   #ifdef __APPLE__
    std::strcat(lv2path, "/../PlugIns/LV2:");
   #else
    std::strcat(lv2path, "/plugins:");
   #endif

    if (const char* const home = getenv("HOME"))
        std::strncpy(path, home, sizeof(path));
    else if (struct passwd* const pwd = getpwuid(getuid()))
        std::strncpy(path, pwd->pw_dir, sizeof(path));
    else
        return 1;

   #ifdef __APPLE__
    std::strcat(path, "/Documents/MOD App");
   #else
    // TODO fetch user docs dir
    std::strcat(path, "/Documents/MOD App");
   #endif

    mkdir(path, 0777);
    setenv("MOD_DATA_DIR", path, 1);

    std::strcat(lv2path, path);
    std::strcat(lv2path, "/lv2");
    setenv("MOD_LV2_PATH", lv2path, 1);

    std::strcat(path, "/user-files");
    mkdir(path, 0777);
    user_files_dir = path;

    struct sigaction sig = {};
    sig.sa_handler = signal;
    sig.sa_flags   = SA_RESTART;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGTERM, &sig, nullptr);
    sigaction(SIGINT, &sig, nullptr);
  #endif

    AppWindow window(cwd);
    return app.exec();
}
