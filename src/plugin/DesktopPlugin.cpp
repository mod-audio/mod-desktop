// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "DistrhoPlugin.hpp"

#include "extra/Sleep.hpp"

#include <cerrno>
#include <ctime>

#include <fcntl.h>
#include <signal.h>
#include <syscall.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <linux/futex.h>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

/*
 * Get a monotonically-increasing time in milliseconds.
 */
static inline
uint32_t d_gettime_ms() noexcept
{
   #if defined(DISTRHO_OS_MAC)
    static const time_t s = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000000;
    return (clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000000) - s;
   #elif defined(DISTRHO_OS_WINDOWS)
    return static_cast<uint32_t>(timeGetTime());
   #else
    static struct {
        timespec ts;
        int r;
        uint32_t ms;
    } s = { {}, clock_gettime(CLOCK_MONOTONIC, &s.ts), static_cast<uint32_t>(s.ts.tv_sec * 1000 +
                                                                             s.ts.tv_nsec / 1000000) };
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000) - s.ms;
   #endif
}

// -----------------------------------------------------------------------------------------------------------

struct SharedMemoryData {
    uint32_t magic;
    int32_t sem1;
    int32_t sem2;
    int32_t padding1;
    // uint8_t sem[32];
    uint16_t midiEventCount;
    uint16_t midiFrames[511];
    uint8_t midiData[511 * 4];
    uint8_t padding2[4];
    float audio[];
};

const size_t kSharedMemoryDataSize = sizeof(SharedMemoryData) + sizeof(float) * 128 * 2;

static inline
void shm_sem_rt_post(SharedMemoryData* const data)
{
    const bool unlocked = __sync_bool_compare_and_swap(&data->sem1, 0, 1);
    DISTRHO_SAFE_ASSERT_RETURN(unlocked,);
    ::syscall(__NR_futex, &data->sem1, FUTEX_WAKE, 1, nullptr, nullptr, 0);
}

static inline
bool shm_sem_rt_wait(SharedMemoryData* const data)
{
    const timespec timeout = { 1, 0 };

    for (;;)
    {
        if (__sync_bool_compare_and_swap(&data->sem2, 1, 0))
            return true;

        if (::syscall(__NR_futex, &data->sem2, FUTEX_WAIT, 0, &timeout, nullptr, 0) != 0)
            if (errno != EAGAIN && errno != EINTR)
                return false;
    }
}

class SharedMemory {
    SharedMemoryData* data = nullptr;
    int fd = -1;

public:
    SharedMemory()
    {
    }

    ~SharedMemory()
    {
        deinit();
    }

    bool init()
    {
        fd = shm_open("/mod-desktop-test1", O_CREAT|O_EXCL|O_RDWR, 0600);
        DISTRHO_SAFE_ASSERT_RETURN(fd >= 0, false);
        
        {
            const int ret = ::ftruncate(fd, static_cast<off_t>(kSharedMemoryDataSize));
            DISTRHO_SAFE_ASSERT_RETURN(ret == 0, false);
        }

        {
            void* const ptr = ::mmap(nullptr, kSharedMemoryDataSize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fd, 0);
            DISTRHO_SAFE_ASSERT_RETURN(ptr != nullptr, false);
            DISTRHO_SAFE_ASSERT_RETURN(ptr != MAP_FAILED, false);

            data = static_cast<SharedMemoryData*>(ptr);
        }

        std::memset(data, 0, kSharedMemoryDataSize);
        data->magic = 1337;

        return true;
    }

    void deinit()
    {
        if (data != nullptr)
        {
            ::munmap(data, kSharedMemoryDataSize);
            data = nullptr;
        }

        if (fd >= 0)
        {
            close(fd);
            shm_unlink("/mod-desktop-test1");
            fd = -1;
        }
    }

    bool process(const float** const input, float** output)
    {
        DISTRHO_SAFE_ASSERT_RETURN(data != nullptr, false);

        // place audio buffer
        std::memcpy(data->audio, input[0], sizeof(float) * 128);
        std::memcpy(data->audio + 128, input[1], sizeof(float) * 128);

        // unlock RT waiter
        shm_sem_rt_post(data);

        // wait for processing
        if (! shm_sem_rt_wait(data))
            return false;

        // copy processed buffers
        std::memcpy(output[0], data->audio, sizeof(float) * 128);
        std::memcpy(output[1], data->audio + 128, sizeof(float) * 128);

        return true;
    }
};

// -----------------------------------------------------------------------------------------------------------

class ChildProcess
{
    pid_t pid = 0;

public:
    ChildProcess()
    {
    }

    ~ChildProcess()
    {
        if (pid != 0)
          stop();
    }

    bool start()
    {
        #define P "/home/falktx/Source/MOD/mod-app/build"

        static constexpr const char* const args[] = {
            "jackd", "-R", "-S", "-n", "mod-desktop", "-C", P "/jack/jack-session.conf", "-d", "desktop", nullptr
        };

        // FIXME
        setenv("LD_LIBRARY_PATH", P, 1);
        setenv("JACK_DRIVER_DIR", P "/jack", 1);
        setenv("MOD_DATA_DIR", P "/data", 1);
        setenv("MOD_FACTORY_PEDALBOARDS_DIR", P "/pedalboards", 1);
        setenv("MOD_DESKTOP", "1", 1);
        setenv("LANG", "en_US.UTF-8", 1);
        setenv("LV2_PATH", P "/plugins", 1);

        #undef P

        const pid_t ret = pid = vfork();

        switch (ret)
        {
        // child process
        case 0:
            execvp("/usr/bin/jackd", const_cast<char* const*>(args));

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

    void stop()
    {
        DISTRHO_SAFE_ASSERT_RETURN(pid != 0,);

        const uint32_t timeout = d_gettime_ms() + 2000;
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
};

class DesktopPlugin : public Plugin
{
    ChildProcess jackd;
    SharedMemory shm;
    bool processing = false;
    float fParameters[24] = {};

public:
    DesktopPlugin()
        : Plugin(0, 0, 0)
    {
        if (isDummyInstance())
            return;

        if (shm.init() && jackd.start())
            processing = true;
    }

    ~DesktopPlugin()
    {
        jackd.stop();
        shm.deinit();
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      A plugin label follows the same rules as Parameter::symbol, with the exception that it can start with numbers.
    */
    const char* getLabel() const override
    {
        return "mod_desktop";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const override
    {
        return "";
    }

   /**
      Get the plugin author/maker.
    */
    const char* getMaker() const override
    {
        return "MOD Audio";
    }

   /**
      Get the plugin homepage.
    */
    const char* getHomePage() const override
    {
        return "https://github.com/moddevices/mod-desktop";
    }

   /**
      Get the plugin license name (a single line of text).
      For commercial plugins this should return some short copyright information.
    */
    const char* getLicense() const override
    {
        return "AGPL-3.0-or-later";
    }

   /**
      Get the plugin version, in hexadecimal.
    */
    uint32_t getVersion() const override
    {
        return d_version(1, 0, 0);
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the audio port @a index.@n
      This function will be called once, shortly after the plugin is created.
    */
    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
        // treat meter audio ports as stereo
        port.groupId = kPortGroupStereo;

        // everything else is as default
        Plugin::initAudioPort(input, index, port);
    }

   /**
      Initialize the parameter @a index.@n
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) override
    {
        parameter.hints = kParameterIsAutomatable;

        switch (index)
        {
        }
    }

   /**
      Set a state key and default value.
      This function will be called once, shortly after the plugin is created.
    */
    void initState(uint32_t, String&, String&) override
    {
        // we are using states but don't want them saved in the host
    }

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
    */
    float getParameterValue(const uint32_t index) const override
    {
        return fParameters[index];
    }

   /**
      Change a parameter value.
    */
    void setParameterValue(const uint32_t index, const float value) override
    {
        fParameters[index] = value;
    }

   /**
      Change an internal state.
    */
    void setState(const char*, const char*) override
    {
    }

   /* --------------------------------------------------------------------------------------------------------
    * Process */

   /**
      Run/process function for plugins without MIDI input.
    */
    void run(const float** const inputs, float** const outputs, const uint32_t frames) override
    {
        if (processing)
        {
            if (frames != 128)
                d_stdout("frames != 128");
            else if (shm.process(inputs, outputs))
                return;

            d_stdout("shm processing failed");
            processing = false;
        }

        std::memset(outputs[0], 0, sizeof(float) * frames);
        std::memset(outputs[1], 0, sizeof(float) * frames);
    }

    // -------------------------------------------------------------------------------------------------------

   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesktopPlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new DesktopPlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
