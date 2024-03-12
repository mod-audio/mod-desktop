// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "extra/Sleep.hpp"
#include "Time.hpp"

#if defined(DISTRHO_OS_MAC)
#elif defined(DISTRHO_OS_WINDOWS)
#else
#endif

#ifdef DISTRHO_OS_WINDOWS
# include <string>
# include <winsock2.h>
# include <windows.h>
#else
# include <cerrno>
# include <ctime>
# include <signal.h>
# include <sys/wait.h>
#endif

// #include <sys/time.h>

START_NAMESPACE_DISTRHO

// FIXME
#if defined(DISTRHO_OS_MAC)
# define P "/Users/falktx/Source/MOD/mod-app/build/mod-desktop.app/Contents"
#elif defined(DISTRHO_OS_WINDOWS)
# define P "Z:\\home\\falktx\\Source\\MOD\\mod-app\\build"
#else
# define P "/home/falktx/Source/MOD/mod-app/build"
#endif

// -----------------------------------------------------------------------------------------------------------

class ChildProcess
{
   #ifdef _WIN32
    PROCESS_INFORMATION process = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };
   #else
    pid_t pid = -1;
   #endif

public:
    ChildProcess()
    {
    }

    ~ChildProcess()
    {
        stop();
    }

    bool start(const char* const args[])
    {
        // FIXME
       #ifdef DISTRHO_OS_WINDOWS
        const CHAR* const envp = (
            "MOD_DATA_DIR=" P "\\data\0"
            "MOD_FACTORY_PEDALBOARDS_DIR=" P "\\pedalboards\0"
            "LV2_PATH=" P "\\plugins\0"
            "\0");
       #else
        const char* const envp[] = {
            "MOD_DESKTOP=1",
            "LANG=en_US.UTF-8",
           #if defined(DISTRHO_OS_MAC)
            "DYLD_LIBRARY_PATH=" P "/MacOS",
            "JACK_DRIVER_DIR=" P "/MacOS/jack",
            "MOD_DATA_DIR=/Users/falktx/Documents/MOD Desktop",
            "MOD_FACTORY_PEDALBOARDS_DIR=" P "/Resources/pedalboards",
            "LV2_PATH=" P "/LV2",
           #else
            "LD_LIBRARY_PATH=" P,
            "JACK_DRIVER_DIR=" P "/jack",
            "MOD_DATA_DIR=" P "/data",
            "MOD_FACTORY_PEDALBOARDS_DIR=" P "/pedalboards",
            "LV2_PATH=" P "/plugins",
           #endif
            nullptr
        };
       #endif

       #ifdef _WIN32
        std::string cmd;

        for (uint i = 0; args[i] != nullptr; ++i)
        {
            if (i != 0)
                cmd += " ";

            cmd += args[i];
        }

        STARTUPINFOA si = {};
        si.cb = sizeof(si);

        d_stdout("will start process with args '%s'", cmd.data());

        return CreateProcessA(nullptr,    // lpApplicationName
                              &cmd[0],    // lpCommandLine
                              nullptr,    // lpProcessAttributes
                              nullptr,    // lpThreadAttributes
                              TRUE,       // bInheritHandles
                              0, // CREATE_NO_WINDOW /*| CREATE_UNICODE_ENVIRONMENT*/, // dwCreationFlags
                              const_cast<LPSTR>(envp), // lpEnvironment
                              nullptr,    // lpCurrentDirectory
                              &si,        // lpStartupInfo
                              &process) != FALSE;
       #else
        const pid_t ret = pid = vfork();

        switch (ret)
        {
        // child process
        case 0:
            execve(args[0], const_cast<char* const*>(args), const_cast<char* const*>(envp));

            d_stderr2("exec failed: %d:%s", errno, std::strerror(errno));
            _exit(1);
            break;

        // error
        case -1:
            d_stderr2("vfork() failed: %d:%s", errno, std::strerror(errno));
            break;
        }

        return ret > 0;
       #endif
    }

    void stop(const uint32_t timeoutInMilliseconds = 2000)
    {
        const uint32_t timeout = d_gettime_ms() + timeoutInMilliseconds;
        bool sendTerminate = true;

       #ifdef _WIN32
        if (process.hProcess == INVALID_HANDLE_VALUE)
            return;

        const PROCESS_INFORMATION oprocess = process;
        process = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0 };

        for (;;)
        {
            switch (WaitForSingleObject(oprocess.hProcess, 0))
            {
            case WAIT_OBJECT_0:
            case WAIT_FAILED:
                CloseHandle(oprocess.hThread);
                CloseHandle(oprocess.hProcess);
                return;
            }

            if (sendTerminate)
            {
                sendTerminate = false;
                TerminateProcess(oprocess.hProcess, 15);
            }
            if (d_gettime_ms() < timeout)
            {
                d_msleep(5);
                continue;
            }
            d_stderr("ChildProcess::stop() - timed out");
            TerminateProcess(oprocess.hProcess, 9);
            d_msleep(5);
            CloseHandle(oprocess.hThread);
            CloseHandle(oprocess.hProcess);
            break;
        }
       #else
        if (pid <= 0)
            return;

        const pid_t opid = pid;
        pid = -1;

        for (pid_t ret;;)
        {
            try {
                ret = ::waitpid(opid, nullptr, WNOHANG);
            } DISTRHO_SAFE_EXCEPTION_BREAK("waitpid");

            switch (ret)
            {
            case -1:
                if (errno == ECHILD)
                {
                    // success, child doesn't exist
                    return;
                }
                else
                {
                    d_stderr("ChildProcess::stop() - waitpid failed: %d:%s", errno, std::strerror(errno));
                    return;
                }
                break;

            case 0:
                if (sendTerminate)
                {
                    sendTerminate = false;
                    kill(opid, SIGTERM);
                }
                if (d_gettime_ms() < timeout)
                {
                    d_msleep(5);
                    continue;
                }

                d_stderr("ChildProcess::stop() - timed out");
                kill(opid, SIGKILL);
                waitpid(opid, nullptr, WNOHANG);
                break;

            default:
                if (ret == opid)
                {
                    // success
                    return;
                }
                else
                {
                    d_stderr("ChildProcess::stop() - got wrong pid %i (requested was %i)", int(ret), int(opid));
                    return;
                }
            }

            break;
        }
       #endif
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChildProcess)
};

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
