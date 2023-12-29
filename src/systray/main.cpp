// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mod-app.hpp"
#include "qrc_mod-app.hpp"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QStyleFactory>

#ifdef _WIN32
#include <dwmapi.h>
#include <shlobj.h>
#define PRE_20H1_DWMWA_USE_IMMERSIVE_DARK_MODE 19
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
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
    app.setApplicationName("MOD App");
    app.setOrganizationName("MOD Audio");
    app.setWindowIcon(QIcon(":/res/mod-logo.svg"));

  #ifdef _WIN32
    SetEnvironmentVariableW(L"LANG", L"en_US.UTF-8");

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
    SetEnvironmentVariableW(L"MOD_USER_FILES_DIR", path);
    user_files_dir = path;

    const BOOL darkMode = QSettings(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat).value("AppsUseLightTheme", 1).toInt() == 0;
   #else
    const bool darkMode = false;
   #endif

    if (darkMode)
    {
        QPalette palette;
        palette.setColor(QPalette::Disabled, QPalette::Window, QColor(40,40,40));
        palette.setColor(QPalette::Active,   QPalette::Window, QColor(45,45,45));
        palette.setColor(QPalette::Inactive, QPalette::Window, QColor(45,45,45));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(83, 83, 83));
        palette.setColor(QPalette::Active,   QPalette::WindowText, QColor(240, 240, 240));
        palette.setColor(QPalette::Inactive, QPalette::WindowText, QColor(240, 240, 240));
        palette.setColor(QPalette::Disabled, QPalette::Base, QColor(6, 6, 6));
        palette.setColor(QPalette::Active,   QPalette::Base, QColor(7, 7, 7));
        palette.setColor(QPalette::Inactive, QPalette::Base, QColor(7, 7, 7));
        palette.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(12, 12, 12));
        palette.setColor(QPalette::Active,   QPalette::AlternateBase, QColor(14, 14, 14));
        palette.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(14, 14, 14));
        palette.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(4, 4, 4));
        palette.setColor(QPalette::Active,   QPalette::ToolTipBase, QColor(4, 4, 4));
        palette.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(4, 4, 4));
        palette.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(230, 230, 230));
        palette.setColor(QPalette::Active,   QPalette::ToolTipText, QColor(230, 230, 230));
        palette.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(230, 230, 230));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(74, 74, 74));
        palette.setColor(QPalette::Active,   QPalette::Text, QColor(230, 230, 230));
        palette.setColor(QPalette::Inactive, QPalette::Text, QColor(230, 230, 230));
        palette.setColor(QPalette::Disabled, QPalette::Button, QColor(24, 24, 24));
        palette.setColor(QPalette::Active,   QPalette::Button, QColor(28, 28, 28));
        palette.setColor(QPalette::Inactive, QPalette::Button, QColor(28, 28, 28));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(90, 90, 90));
        palette.setColor(QPalette::Active,   QPalette::ButtonText, QColor(240, 240, 240));
        palette.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(240, 240, 240));
        palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
        palette.setColor(QPalette::Active,   QPalette::BrightText, QColor(255, 255, 255));
        palette.setColor(QPalette::Inactive, QPalette::BrightText, QColor(255, 255, 255));
        palette.setColor(QPalette::Disabled, QPalette::Light, QColor(191, 191, 191));
        palette.setColor(QPalette::Active,   QPalette::Light, QColor(191, 191, 191));
        palette.setColor(QPalette::Inactive, QPalette::Light, QColor(191, 191, 191));
        palette.setColor(QPalette::Disabled, QPalette::Midlight, QColor(155, 155, 155));
        palette.setColor(QPalette::Active,   QPalette::Midlight, QColor(155, 155, 155));
        palette.setColor(QPalette::Inactive, QPalette::Midlight, QColor(155, 155, 155));
        palette.setColor(QPalette::Disabled, QPalette::Dark, QColor(129, 129, 129));
        palette.setColor(QPalette::Active,   QPalette::Dark, QColor(129, 129, 129));
        palette.setColor(QPalette::Inactive, QPalette::Dark, QColor(129, 129, 129));
        palette.setColor(QPalette::Disabled, QPalette::Mid, QColor(94, 94, 94));
        palette.setColor(QPalette::Active,   QPalette::Mid, QColor(94, 94, 94));
        palette.setColor(QPalette::Inactive, QPalette::Mid, QColor(94, 94, 94));
        palette.setColor(QPalette::Disabled, QPalette::Shadow, QColor(155, 155, 155));
        palette.setColor(QPalette::Active,   QPalette::Shadow, QColor(155, 155, 155));
        palette.setColor(QPalette::Inactive, QPalette::Shadow, QColor(155, 155, 155));
        palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(14, 14, 14));
        palette.setColor(QPalette::Active,   QPalette::Highlight, QColor(60, 60, 60));
        palette.setColor(QPalette::Inactive, QPalette::Highlight, QColor(34, 34, 34));
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(83, 83, 83));
        palette.setColor(QPalette::Active,   QPalette::HighlightedText, QColor(255, 255, 255));
        palette.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(240, 240, 240));
        palette.setColor(QPalette::Disabled, QPalette::Link, QColor(34, 34, 74));
        palette.setColor(QPalette::Active,   QPalette::Link, QColor(100, 100, 230));
        palette.setColor(QPalette::Inactive, QPalette::Link, QColor(100, 100, 230));
        palette.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(74, 34, 74));
        palette.setColor(QPalette::Active,   QPalette::LinkVisited, QColor(230, 100, 230));
        palette.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(230, 100, 230));

        app.setStyle(QStyleFactory::create("Fusion"));
        app.setPalette(palette);
        // TODO check if this is ok
        app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
    }

  #ifndef _WIN32
    setenv("LANG", "en_US.UTF-8", 1);

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
   #ifdef __APPLE__
    std::strncpy(lv2path, path, pathlen - 5);
    std::strcat(lv2path, "PlugIns/LV2:");
   #else
    std::strncpy(lv2path, path, pathlen);
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

    // TODO MOD_KEYS_PATH

    std::strcat(lv2path, path);
    std::strcat(lv2path, "/lv2");
    setenv("MOD_LV2_PATH", lv2path, 1);

    std::strcat(path, "/user-files");
    mkdir(path, 0777);
    setenv("MOD_USER_FILES_DIR", path, 1);
    user_files_dir = path;

    struct sigaction sig = {};
    sig.sa_handler = signal;
    sig.sa_flags   = SA_RESTART;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGTERM, &sig, nullptr);
    sigaction(SIGINT, &sig, nullptr);
  #endif

    AppWindow window(cwd);

   #ifdef _WIN32
    if (darkMode)
    {
        const HWND hwnd = (HWND)window.winId();
        if ((DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode)) != S_OK))
            DwmSetWindowAttribute(hwnd, PRE_20H1_DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    }
   #endif

    return app.exec();
}
