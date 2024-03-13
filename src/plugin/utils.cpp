// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "utils.hpp"

#include <pthread.h>

#ifdef _WIN32
#else
#include <sys/stat.h>
// #include <unistd.h>
#endif

#if defined(__APPLE__)
#include <crt_externs.h>
#include <sys/syslimits.h>
#include <IOKit/IOKitLib.h>
#elif defined(__linux__)
#include <linux/limits.h>
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------------------------------------

#ifdef _WIN32
static const WCHAR* getAppDirW()
{
    static WCHAR appDir[MAX_PATH] = {};

    if (appDir[0] == 0)
    {
        // TODO
        SHGetSpecialFolderPathW(nullptr, dataDir, CSIDL_MYDOCUMENTS, false);
        std::wcsncat(dataDir, L"\\MOD Desktop", MAX_PATH - 1);
    }

    return appDir;
}
#endif

const char* getAppDir()
{
   #ifdef DISTRHO_OS_MAC
    return "/Applications/MOD Desktop.app/Contents/MacOS";
   #else
    static char appDir[PATH_MAX] = {};

    if (appDir[0] == 0)
    {
       #if defined(_WIN32)
       #else
        char appDir[PATH_MAX] = {};

        // TODO
        std::strncpy(appDir, filename, PATH_MAX - 1);

        if (char* const c = std::strrchr(appDir, '/'))
            *c = 0;
       #endif
    }

    return appDir;
   #endif
}

// -----------------------------------------------------------------------------------------------------------

static const char* gMOD_USER_FILES_DIR = nullptr;

static void set_envp_value(char** envp, const char* const fullvalue)
{
    const size_t keylen = std::strchr(fullvalue, '=') - fullvalue;

    for (; *envp != nullptr; ++envp)
    {
        if (std::strncmp(*envp, fullvalue, keylen + 1) == 0)
        {
            const size_t fulllen = std::strlen(fullvalue);
            *envp = static_cast<char*>(std::realloc(*envp, fulllen + 1));
            std::memcpy(*envp + (keylen + 1), fullvalue + (keylen + 1), fulllen - keylen);
            return;
        }
    }

    *envp = strdup(fullvalue);
}

static const char* set_envp_value(char** envp, const char* const key, const char* const value)
{
    const size_t keylen = std::strlen(key);
    const size_t valuelen = std::strlen(value);

    for (; *envp != nullptr; ++envp)
    {
        if (std::strncmp(*envp, key, keylen) == 0 && (*envp)[keylen] == '=')
        {
            *envp = static_cast<char*>(std::realloc(*envp, keylen + valuelen + 2));
            std::memcpy(*envp + (keylen + 1), value, valuelen + 1);
            return *envp + (keylen + 1);
        }
    }

    *envp = static_cast<char*>(std::malloc(keylen + valuelen + 2));
    std::memcpy(*envp, key, keylen);
    std::memcpy(*envp + (keylen + 1), value, valuelen + 1);
    (*envp)[keylen] = '=';
    return *envp + (keylen + 1);
}

// -----------------------------------------------------------------------------------------------------------

// NOTE this needs to match initEvironment from systray side
char* const* getEvironment()
{
   #ifdef DISTRHO_OS_MAC
    const char* const* const* const environptr = _NSGetEnviron();
    DISTRHO_SAFE_ASSERT_RETURN(environptr != nullptr, nullptr);

    const char* const* const environ = *environptr;
    DISTRHO_SAFE_ASSERT_RETURN(environ != nullptr, nullptr);
   #endif

    uint envsize = 0;
   #ifdef _WIN32
    envp = (
        "MOD_DATA_DIR=" P "\\data\0"
        "MOD_FACTORY_PEDALBOARDS_DIR=" P "\\pedalboards\0"
        "LV2_PATH=" P "\\plugins\0"
        "\0"
    );
   #else
    while (environ[envsize] != nullptr)
        ++envsize;

    char** const envp = new char*[envsize + 13];

    for (uint i = 0; i < envsize; ++i)
        envp[i] = strdup(environ[i]);

    for (uint i = 0; i < 13; ++i)
        envp[envsize + i] = nullptr;

    // base environment details
    set_envp_value(envp, "LANG=en_US.UTF-8");
    set_envp_value(envp, "MOD_DESKTOP=1");
    set_envp_value(envp, "MOD_DESKTOP_PLUGIN=1");
    set_envp_value(envp, "PYTHONUNBUFFERED=1");
   #endif

    // get directory of the mod-desktop application
   #ifdef _WIN32
    const WCHAR* const appDir = getAppDirW();
   #else
    const char* const appDir = getAppDir();
   #endif

    // get and set directory to our documents and settings, under "user documents"; also make sure it exists
   #ifdef _WIN32
    WCHAR dataDir[MAX_PATH] = {};
    SHGetSpecialFolderPathW(nullptr, dataDir, CSIDL_MYDOCUMENTS, false);

    _wmkdir(dataDir);
    std::wcsncat(dataDir, L"\\MOD Desktop", MAX_PATH - 1);
    _wmkdir(dataDir);

    set_envp_value(envp, L"MOD_DATA_DIR", dataDir);
   #else
    char dataDir[PATH_MAX] = {};

    // NOTE a generic implementation is a bit complex, let Qt do it for us this time
    // const QByteArray docsDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation).toUtf8());
    // std::strncpy(dataDir, docsDir.constData(), PATH_MAX - 1);
    // TODO
    std::strncpy(dataDir, getenv("HOME"), PATH_MAX - 1);
    std::strncat(dataDir, "/Documents", PATH_MAX - 1);

    mkdir(dataDir, 0777);
    std::strncat(dataDir, "/MOD Desktop", PATH_MAX - 1);
    mkdir(dataDir, 0777);

    set_envp_value(envp, "MOD_DATA_DIR", dataDir);
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

    /*
    {
        QMessageAuthenticationCode qhash(QCryptographicHash::Sha256);
        qhash.setKey(QByteArray::fromRawData(reinterpret_cast<const char*>(key), sizeof(key)));
        qhash.addData(reinterpret_cast<const char*>(mod_desktop_hash), sizeof(mod_desktop_hash));

        QByteArray qresult(qhash.result().toHex(':').toUpper());
        qresult.truncate(32 + 15);

       #ifdef _WIN32
        set_envp_value(envp, L"MOD_DEVICE_UID", qresult.constData());
       #else
        set_envp_value(envp, "MOD_DEVICE_UID", qresult.constData(), 1);
       #endif
    }
    */

    // set path to factory pedalboards
  #ifdef _WIN32
    std::memcpy(path, appDir, appDirLen * sizeof(WCHAR));
    std::wcsncpy(path + appDirLen, L"\\pedalboards", MAX_PATH - appDirLen - 1);
    set_envp_value(envp, L"MOD_FACTORY_PEDALBOARDS_DIR", path);
  #else
    std::memcpy(path, appDir, appDirLen);
   #ifdef __APPLE__
    std::strncpy(path + appDirLen - 5, "Resources/pedalboards", PATH_MAX - appDirLen - 1);
   #else
    std::strncpy(path + appDirLen, "/pedalboards", PATH_MAX - appDirLen - 1);
   #endif
    set_envp_value(envp, "MOD_FACTORY_PEDALBOARDS_DIR", path);
  #endif

    // set path to plugin loadable "user-files"; also make sure it exists
   #ifdef _WIN32
    std::memcpy(path, dataDir, dataDirLen * sizeof(WCHAR));
    std::wcsncpy(path + dataDirLen, L"\\user-files", MAX_PATH - dataDirLen - 1);
    _wmkdir(path);
    gMOD_USER_FILES_DIR = set_envp_value(envp, L"MOD_USER_FILES_DIR", path);
   #else
    std::memcpy(path, dataDir, dataDirLen);
    std::strncpy(path + dataDirLen, "/user-files", PATH_MAX - dataDirLen - 1);
    mkdir(path, 0777);
    gMOD_USER_FILES_DIR = set_envp_value(envp, "MOD_USER_FILES_DIR", path);
   #endif

    // set path to MOD keys (plugin licenses)
    // NOTE must terminate with a path separator
   #ifdef _WIN32
    std::memcpy(path, dataDir, dataDirLen * sizeof(WCHAR));
    std::wcsncpy(path + dataDirLen, L"\\keys\\", MAX_PATH - dataDirLen - 1);
    set_envp_value(envp, L"MOD_KEYS_PATH", path);
   #else
    std::memcpy(path, dataDir, dataDirLen);
    std::strncpy(path + dataDirLen, "/keys/", PATH_MAX - dataDirLen - 1);
    set_envp_value(envp, "MOD_KEYS_PATH", path);
   #endif

    // set path to MOD LV2 plugins (first local/user, then app/system)
  #ifdef _WIN32
    std::memcpy(path, dataDir, dataDirLen * sizeof(WCHAR));
    std::wcsncpy(path + dataDirLen, L"\\lv2;", MAX_PATH - dataDirLen - 1);
    std::wcsncat(path, appDir, MAX_PATH - 1);
    std::wcsncat(path, L"\\plugins", MAX_PATH - 1);
    set_envp_value(envp, L"LV2_PATH", path);
  #else
    std::memcpy(path, dataDir, dataDirLen);
    std::strncpy(path + dataDirLen, "/lv2:", PATH_MAX - dataDirLen - 1);
    std::strncat(path, appDir, PATH_MAX - 1);
   #ifdef __APPLE__
    path[dataDirLen + 5 + appDirLen - 5] = '\0'; // remove "MacOS"
    std::strncat(path, "LV2", PATH_MAX - 1);
   #else
    std::strncat(path, "/plugins", PATH_MAX - 1);
   #endif
    set_envp_value(envp, "LV2_PATH", path);
  #endif

   #if !(defined(__APPLE__) || defined(_WIN32))
    // add our custom lib path on top of LD_LIBRARY_PATH to make sure jackd can run
    if (const char* const ldpath = getenv("LD_LIBRARY_PATH"))
    {
        std::memcpy(path, appDir, appDirLen);
        path[appDirLen] = ':';
        std::strncpy(path + appDirLen + 1, ldpath, PATH_MAX - appDirLen - 2);
        set_envp_value(envp, "LD_LIBRARY_PATH", path);
    }
    else
    {
        set_envp_value(envp, "LD_LIBRARY_PATH", appDir);
    }
   #endif

    // other
   #ifndef _WIN32
    std::memcpy(path, appDir, appDirLen);
    std::strncpy(path + appDirLen, "/jack", PATH_MAX - appDirLen - 1);
    set_envp_value(envp, "JACK_DRIVER_DIR", path);
   #endif

    return envp;
}

// -----------------------------------------------------------------------------------------------------------

static void* _openWebGui(void*)
{
   #if defined(_WIN32)
    ShellExecuteW(nullptr, L"open", L"http://127.0.0.1:18181", nullptr, nullptr, SW_SHOWDEFAULT);
   #elif defined(__APPLE__)
    std::system("open \"http://127.0.0.1:18181\"");
   #else
    std::system("xdg-open \"http://127.0.0.1:18181\"");
   #endif
    return nullptr;
}

void openWebGui()
{
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, 1);
    pthread_create(&thread, &attr, _openWebGui, nullptr);
    pthread_attr_destroy(&attr);
}

// -----------------------------------------------------------------------------------------------------------

static void* _openUserFilesDir(void*)
{
   #if defined(_WIN32)
    ShellExecuteW(NULL, L"explore", gMOD_USER_FILES_DIR, nullptr, nullptr, SW_SHOWDEFAULT);
   #elif defined(__APPLE__)
    String cmd("open \"");
    cmd += gMOD_USER_FILES_DIR;
    cmd += "\"";
    std::system(cmd);
   #else
    String cmd("xdg-open \"");
    cmd += gMOD_USER_FILES_DIR;
    cmd += "\"";
   #endif
    return nullptr;
}

void openUserFilesDir()
{
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, 1);
    pthread_create(&thread, &attr, _openUserFilesDir, nullptr);
    pthread_attr_destroy(&attr);
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
