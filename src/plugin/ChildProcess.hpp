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
#else
# include <cerrno>
# include <ctime>
# include <signal.h>
# include <sys/wait.h>
#endif

// #include <sys/time.h>

START_NAMESPACE_DISTRHO

// FIXME
#define P "/home/falktx/Source/MOD/mod-app/build"

// -----------------------------------------------------------------------------------------------------------

class ChildProcess
{
   #ifdef _WIN32
   #else
    pid_t pid = 0;
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
        const char* const envp[] = {
            "LD_LIBRARY_PATH=" P,
            "JACK_DRIVER_DIR=" P "/jack",
            "MOD_DATA_DIR=" P "/data",
            "MOD_FACTORY_PEDALBOARDS_DIR=" P "/pedalboards",
            "MOD_DESKTOP=1",
            "LANG=en_US.UTF-8",
            "LV2_PATH=" P "/plugins",
            nullptr
        };

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
    }

    void stop(const uint32_t timeoutInMilliseconds = 2000)
    {
        if (pid == 0)
            return;

        const uint32_t timeout = d_gettime_ms() + timeoutInMilliseconds;
        const pid_t opid = pid;
        pid_t ret;
        bool sendTerminate = true;
        pid = 0;

        for (;;)
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
                    ::kill(opid, SIGTERM);
                }
                if (d_gettime_ms() < timeout)
                {
                    d_msleep(5);
                    continue;
                }
                d_stderr("ChildProcess::stop() - timed out");
                ::kill(opid, SIGKILL);
                ::waitpid(opid, nullptr, WNOHANG);
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
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChildProcess)
};

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
