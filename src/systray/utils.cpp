// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "utils.hpp"

#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSettings>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleFactory>

#ifdef _WIN32
#include <dwmapi.h>
#include <shlobj.h>
#define PRE_20H1_DWMWA_USE_IMMERSIVE_DARK_MODE 19
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#else
#include <QtCore/QMessageAuthenticationCode>
#include <QtCore/QStandardPaths>
#include <dlfcn.h>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <cstring>

#if defined(__APPLE__)
#include <IOKit/IOKitLib.h>
#include <sys/syslimits.h>
#elif defined(__linux__)
#include <linux/limits.h>
#endif

constexpr const uint8_t mod_desktop_hash[] = {
    0xe4, 0xf7, 0x0d, 0xe9, 0x77, 0xb8, 0x47, 0xe0, 0xba, 0x2e, 0x70, 0x14, 0x93, 0x7a, 0xce, 0xa7
};

constexpr uint8_t char2u8(const uint8_t c)
{
    return c >= '0' && c <= '9' ? c - '0'
        : c >= 'a' && c <= 'f' ? 0xa + c - 'a'
        : c >= 'A' && c <= 'F' ? 0xa + c - 'A'
        : 0;
}

void initEvironment()
{
    // base environment details
   #ifdef _WIN32
    SetEnvironmentVariableW(L"JACK_NO_START_SERVER", L"1");
    SetEnvironmentVariableW(L"LANG", L"en_US.UTF-8");
    SetEnvironmentVariableW(L"MOD_DESKTOP", L"1");
    SetEnvironmentVariableW(L"PYTHONUNBUFFERED", L"1");
   #else
    setenv("JACK_NO_START_SERVER", "1", 1);
    setenv("LANG", "en_US.UTF-8", 1);
    setenv("MOD_DESKTOP", "1", 1);
    setenv("PYTHONUNBUFFERED", "1", 1);
   #endif

    // get directory of this application
   #ifdef _WIN32
    WCHAR appDir[MAX_PATH] = {};
    GetModuleFileNameW(GetModuleHandleW(nullptr), appDir, MAX_PATH);

    if (wchar_t* const wcc = std::wcsrchr(appDir, '\\'))
        *wcc = 0;
   #else
    char appDir[PATH_MAX] = {};

    {
        Dl_info info = {};
        dladdr((void*)initEvironment, &info);

        if (info.dli_fname[0] == '.')
        {
            getcwd(appDir, PATH_MAX - 1);
            std::strncat(appDir, info.dli_fname + 1, PATH_MAX - 1);
        }
        else if (info.dli_fname[0] != '/')
        {
            getcwd(appDir, PATH_MAX - 1);
            std::strncat(appDir, "/", PATH_MAX - 1);
            std::strncat(appDir, info.dli_fname, PATH_MAX - 1);
        }
        else
        {
            std::strncpy(appDir, info.dli_fname, PATH_MAX - 1);
        }
    }

    if (char* const c = strrchr(appDir, '/'))
        *c = 0;
   #endif

    // set current working directory to this application directory
   #ifdef _WIN32
    SetCurrentDirectoryW(appDir);
    QDir::setCurrent(QString::fromWCharArray(appDir));
   #else
    chdir(appDir);
    QDir::setCurrent(QString::fromUtf8(appDir));
   #endif

    // get and set directory to our documents and settings, under "user documents"; also make sure it exists
   #ifdef _WIN32
    WCHAR dataDir[MAX_PATH] = {};
    SHGetSpecialFolderPathW(nullptr, dataDir, CSIDL_MYDOCUMENTS, false);

    _wmkdir(dataDir);
    std::wcsncat(dataDir, L"\\MOD Desktop", MAX_PATH - 1);
    _wmkdir(dataDir);

    SetEnvironmentVariableW(L"MOD_DATA_DIR", dataDir);
   #else
    char dataDir[PATH_MAX] = {};

    // NOTE a generic implementation is a bit complex, let Qt do it for us this time
    const QByteArray docsDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation).toUtf8());
    std::strncpy(dataDir, docsDir.constData(), PATH_MAX - 1);

    mkdir(dataDir, 0777);
    std::strncat(dataDir, "/MOD Desktop", PATH_MAX - 1);
    mkdir(dataDir, 0777);

    setenv("MOD_DATA_DIR", dataDir, 1);
   #endif

    // reusable
   #ifdef _WIN32
    WCHAR path[MAX_PATH] = {};
    const size_t appDirLen = std::wcslen(appDir);
    const size_t dataDirLen = std::wcslen(dataDir);
   #else
    char path[PATH_MAX] = {};
    const size_t appDirLen = std::strlen(appDir);
    const size_t dataDirLen = std::strlen(dataDir);
   #endif

    // generate UID
    uint8_t key[16] = {};
   #if defined(__APPLE__)
    if (const CFMutableDictionaryRef deviceRef = IOServiceMatching("IOPlatformExpertDevice"))
    {
        const io_service_t service = IOServiceGetMatchingService(MACH_PORT_NULL, deviceRef);

        if (service != IO_OBJECT_NULL)
        {
            const CFTypeRef uuidRef = IORegistryEntryCreateCFProperty(service, CFSTR("IOPlatformUUID"), kCFAllocatorDefault, 0);
            const CFStringRef uuidStr = reinterpret_cast<const CFStringRef>(uuidRef);

            if (CFGetTypeID(uuidRef) == CFStringGetTypeID() && CFStringGetLength(uuidStr) >= 36)
            {
                CFStringGetCString(uuidStr, path, PATH_MAX - 1, kCFStringEncodingUTF8);

                for (int i=0, j=0; i+j<36; ++i)
                {
                    if (path[i*2+j] == '-')
                        ++j;
                    key[i] = char2u8(path[i*2+j]) << 4;
                    key[i] |= char2u8(path[i*2+j+1]) << 0;
                }
            }

            CFRelease(uuidRef);
            IOObjectRelease(service);
        }
    }
   #elif defined(_WIN32)
    // TODO
   #else
    if (FILE* const f = fopen("/etc/machine-id", "r"))
    {
        if (fread(path, PATH_MAX - 1, 1, f) == 0 && strlen(path) >= 33)
        {
            for (int i=0; i<16; ++i)
            {
                key[i] = char2u8(path[i*2]) << 4;
                key[i] |= char2u8(path[i*2+1]) << 0;
            }
        }
        fclose(f);
    }
   #endif

    {
        QMessageAuthenticationCode qhash(QCryptographicHash::Sha256);
        qhash.setKey(QByteArray::fromRawData(reinterpret_cast<const char*>(key), sizeof(key)));
        qhash.addData(reinterpret_cast<const char*>(mod_desktop_hash), sizeof(mod_desktop_hash));

        QByteArray qresult(qhash.result().toHex(':').toUpper());
        qresult.truncate(32 + 15);

       #ifdef _WIN32
        SetEnvironmentVariableA("MOD_DEVICE_UID", qresult.constData());
       #else
        setenv("MOD_DEVICE_UID", qresult.constData(), 1);
       #endif
    }

    // set path to factory pedalboards
  #ifdef _WIN32
    std::memcpy(path, appDir, appDirLen * sizeof(WCHAR));
    std::wcsncpy(path + appDirLen, L"\\pedalboards", MAX_PATH - appDirLen - 1);
    SetEnvironmentVariableW(L"MOD_FACTORY_PEDALBOARDS_DIR", path);
  #else
    std::memcpy(path, appDir, appDirLen);
   #ifdef __APPLE__
    std::strncpy(path + appDirLen - 5, "Resources/pedalboards", PATH_MAX - appDirLen - 1);
   #else
    std::strncpy(path + appDirLen, "/pedalboards", PATH_MAX - appDirLen - 1);
   #endif
    setenv("MOD_FACTORY_PEDALBOARDS_DIR", path, 1);
  #endif

    // set path to plugin loadable "user-files"; also make sure it exists
   #ifdef _WIN32
    std::memcpy(path, dataDir, dataDirLen * sizeof(WCHAR));
    std::wcsncat(path + dataDirLen, L"\\user-files", MAX_PATH - dataDirLen - 1);
    _wmkdir(path);
    SetEnvironmentVariableW(L"MOD_USER_FILES_DIR", path);
   #else
    std::memcpy(path, dataDir, dataDirLen);
    std::strncat(path + dataDirLen, "/user-files", PATH_MAX - dataDirLen - 1);
    mkdir(path, 0777);
    setenv("MOD_USER_FILES_DIR", path, 1);
   #endif

    // set path to MOD keys (plugin licenses)
    // NOTE must terminate with a path separator
   #ifdef _WIN32
    std::memcpy(path, dataDir, dataDirLen * sizeof(WCHAR));
    std::wcsncpy(path + dataDirLen, L"\\keys\\", MAX_PATH - dataDirLen - 1);
    SetEnvironmentVariableW(L"MOD_KEYS_PATH", path);
   #else
    std::memcpy(path, dataDir, dataDirLen);
    std::strncpy(path + dataDirLen, "/keys/", PATH_MAX - dataDirLen - 1);
    setenv("MOD_KEYS_PATH", path, 1);
   #endif

    // set path to MOD LV2 plugins (first local/user, then app/system)
  #ifdef _WIN32
    std::memcpy(path, dataDir, dataDirLen * sizeof(WCHAR));
    std::wcsncpy(path + dataDirLen, L"\\lv2;", MAX_PATH - dataDirLen - 1);
    std::wcsncat(path, appDir, MAX_PATH - 1);
    std::wcsncat(path, L"\\plugins", MAX_PATH - 1);
    SetEnvironmentVariableW(L"MOD_LV2_PATH", path);
  #else
    std::memcpy(path, dataDir, dataDirLen);
    std::strncpy(path + dataDirLen, "/lv2:", PATH_MAX - dataDirLen - 1);
    std::strncat(path, appDir, PATH_MAX - 1);
   #ifdef __APPLE__
    path[dataDirLen + 5 + appDirLen - 5] = '\0'; // remove "MacOS"
    std::strncat(path, "PlugIns/LV2", PATH_MAX - 1);
   #else
    std::strncat(path, "/plugins", PATH_MAX - 1);
   #endif
    setenv("MOD_LV2_PATH", path, 1);
  #endif

   #if !(defined(__APPLE__) || defined(_WIN32))
    // special handling for PipeWire JACK, need to find full path to shared lib
    bool usingPipeWire = false;
    if (void* const pwlib = dlmopen(LM_ID_NEWLM, "libjack.so.0", RTLD_NOW|RTLD_LOCAL))
    {
        typedef int (*jacksym)(void);
        const jacksym sym1 = reinterpret_cast<jacksym>(dlsym(pwlib, "jack_client_name_size"));
        void* const sym2 = dlsym(pwlib, "jack_set_sample_rate");

        if (sym1 != nullptr && sym2 != nullptr && sym1() != 0)
        {
            Dl_info info = {};
            dladdr(sym2, &info);
            setenv("JACKBRIDGE_FILENAME", info.dli_fname, 1);
            usingPipeWire = true;
            fprintf(stdout, "MOD Desktop DEBUG: jacklib syms %p %p | %d | ok with filename '%s'\n", sym1, sym2, sym1(), info.dli_fname);
        }
        else
        {
            fprintf(stdout, "MOD Desktop DEBUG: jacklib syms %p %p | %d | failed\n", sym1, sym2, sym1 != nullptr ? sym1() : -1337);
        }
        fflush(stdout);

        dlclose(pwlib);
    }

    // if LD_LIBRARY_PATH is set, add our custom lib path on top to make sure jackd can run
    if (const char* const ldpath = getenv("LD_LIBRARY_PATH"))
    {
        std::memcpy(path, appDir, appDirLen);
        path[appDirLen] = ':';
        std::strncpy(path + appDirLen + 1, ldpath, PATH_MAX - appDirLen - 2);
        setenv("LD_LIBRARY_PATH", path, 1);
    }
    // always set path in case of PipeWire
    else if (usingPipeWire)
    {
        setenv("LD_LIBRARY_PATH", appDir, 1);
    }
   #endif

    // other
   #ifndef _WIN32
    std::memcpy(path, appDir, appDirLen);
    std::strncpy(path + appDirLen, "/jack", PATH_MAX - appDirLen - 1);
    setenv("JACK_DRIVER_DIR", path, 1);
   #endif
}

QString getLV2Path(const bool includeSystemPlugins)
{
   #ifdef _WIN32
    WCHAR path[MAX_PATH] = {};
    DWORD r, pathlen = GetEnvironmentVariableW(L"MOD_LV2_PATH", path, MAX_PATH - 1);
   #else
    char path[PATH_MAX] = {};
    std::strncpy(path, std::getenv("MOD_LV2_PATH"), PATH_MAX - 1);
   #endif

    if (includeSystemPlugins)
    {
       #if defined(__APPLE__)
        std::strncat(path, ":~/Library/Audio/Plug-Ins/LV2:/Library/Audio/Plug-Ins/LV2", PATH_MAX - 1);
       #elif defined(_WIN32)
        r = GetEnvironmentVariableW(L"LOCALAPPDATA", path + pathlen + 1, MAX_PATH - pathlen - 1);
        if (r == pathlen)
            r = GetEnvironmentVariableW(L"APPDATA", path + pathlen + 1, MAX_PATH - pathlen - 1);
        if (r != pathlen)
        {
            path[pathlen] = L';';
            pathlen += r;
            path[pathlen++] = L'\\';
            path[pathlen++] = L'L';
            path[pathlen++] = L'V';
            path[pathlen++] = L'2';
            path[pathlen] = 0;
        }

        r = GetEnvironmentVariableW(L"COMMONPROGRAMFILES", path + pathlen + 1, MAX_PATH - pathlen - 1);
        if (r != pathlen)
        {
            path[pathlen] = L';';
            pathlen += r;
            path[pathlen++] = L'\\';
            path[pathlen++] = L'L';
            path[pathlen++] = L'V';
            path[pathlen++] = L'2';
            path[pathlen] = 0;
        }
       #else
        if (const char* const env = std::getenv("LV2_PATH"))
        {
            std::strncat(path, ":", PATH_MAX - 1);
            std::strncat(path, env, PATH_MAX - 1);
        }
        else
        {
            std::strncat(path, ":~/.lv2:/usr/local/lib/lv2:/usr/lib/lv2", PATH_MAX - 1);
        }
       #endif
    }

   #ifdef _WIN32
    return QString::fromWCharArray(path);
   #else
    return QString::fromUtf8(path);
   #endif
}

void openWebGui()
{
    QDesktopServices::openUrl(QUrl("http://127.0.0.1:18181"));
}

void openUserFilesDir()
{
   #ifdef _WIN32
    WCHAR path[MAX_PATH] = {};
    GetEnvironmentVariableW(L"MOD_USER_FILES_DIR", path, MAX_PATH);
    const QString userFilesDir(QString::fromWCharArray(path));
   #else
    const QString userFilesDir(QString::fromUtf8(std::getenv("MOD_USER_FILES_DIR")));
   #endif
    QDesktopServices::openUrl(QUrl::fromLocalFile(userFilesDir));
}

bool shouldUseDarkMode()
{
   #ifdef _WIN32
    return QSettings(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat).value("AppsUseLightTheme", 1).toInt() == 0;
   #else
    return false;
   #endif
}

static void closeApp()
{
    if (QApplication* const app = qApp)
    {
        app->setQuitOnLastWindowClosed(true);
        app->quit();
    }
}

#ifdef _WIN32
static BOOL signalHandler(const DWORD ctrl)
{
    if (ctrl == CTRL_C_EVENT)
    {
        closeApp();
        return TRUE;
    }
    return FALSE;
}
#else
static void signalHandler(const int sig)
{
    switch (sig)
    {
    case SIGINT:
    case SIGTERM:
        closeApp();
        break;
    }
}
#endif

void setupControlCloseSignal()
{
   #ifdef _WIN32
    SetConsoleCtrlHandler(signalHandler, TRUE);
   #else
    struct sigaction sig = {};
    sig.sa_handler = signalHandler;
    sig.sa_flags = SA_RESTART;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGINT, &sig, nullptr);
    sigaction(SIGTERM, &sig, nullptr);
   #endif
}

void setupDarkModePalette(QApplication& app)
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
    app.setStyleSheet(R"(
        #frame_audio, #frame_gui, #frame_lv2, #frame_midi { border: 1px solid #3b3b3b; }
        QToolTip { color: #e6e6e6; background-color: #252525; border: 1px solid #424242; }
    R)");
}

void setupDarkModeWindow(QMainWindow& window)
{
   #ifdef _WIN32
    BOOL darkMode = TRUE;
    const HWND hwnd = (HWND)window.winId();

    if ((DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode)) != S_OK))
        DwmSetWindowAttribute(hwnd, PRE_20H1_DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
   #endif
}

void writeMidiChannelsToProfile(int pedalboard, int snapshot)
{
   #ifdef _WIN32
    WCHAR path[MAX_PATH] = {};
    GetEnvironmentVariableW(L"MOD_DATA_DIR", path, MAX_PATH);
    std::wcsncat(path, L"\\profile5.json", MAX_PATH - 1);
    const QString jsonPath(QString::fromWCharArray(path));
   #else
    char path[PATH_MAX] = {};
    std::strncpy(path, std::getenv("MOD_DATA_DIR"), PATH_MAX - 1);
    std::strncat(path, "/profile5.json", PATH_MAX - 1);
    const QString jsonPath(QString::fromUtf8(path));
   #endif

    QFile jsonFile(jsonPath);

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
