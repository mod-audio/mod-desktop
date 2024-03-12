/*
*/

#include "JackAudioDriver.h"
#include "JackDriverLoader.h"
#include "JackEngineControl.h"
#include "JackTools.h"

#ifdef _WIN32
#else
# include <fcntl.h>
# include <sys/mman.h>
# ifdef __APPLE__
#  include <mach/mach.h>
#  include <mach/semaphore.h>
#  include <servers/bootstrap.h>
# else
#  include <cerrno>
#  include <syscall.h>
#  include <sys/time.h>
#  include <linux/futex.h>
# endif
#endif

namespace Jack
{

// -----------------------------------------------------------------------------------------------------------

class DesktopAudioDriver : public JackAudioDriver
{
    struct Data {
        uint32_t magic;
        int32_t padding1;
       #if defined(__APPLE__)
        char bootname1[32];
        char bootname2[32];
       #elif defined(_WIN32)
        HANDLE sem1;
        HANDLE sem2;
       #else
        int32_t sem1;
        int32_t sem2;
       #endif
        uint16_t midiEventCount;
        uint16_t midiFrames[511];
        uint8_t midiData[511 * 4];
        uint8_t padding2[4];
        float audio[];
    };

    static constexpr const size_t kDataSize = sizeof(Data) + sizeof(float) * 128 * 2;

    Data* fShmData;

   #ifndef _WIN32
    int fShmFd;
   #endif

   #if defined(__APPLE__)
    mach_port_t task = MACH_PORT_NULL;
    semaphore_t sem1 = MACH_PORT_NULL;
    semaphore_t sem2 = MACH_PORT_NULL;

    void post()
    {
        semaphore_signal(sem2);
    }

    bool wait()
    {
        const mach_timespec timeout = { 1, 0 };
        return semaphore_timedwait(sem1, timeout) == KERN_SUCCESS;
    }
   #elif defined(_WIN32)
    void post()
    {
    }

    bool wait()
    {
        return false;
    }
   #else
    void post()
    {
        const bool unlocked = __sync_bool_compare_and_swap(&data->sem2, 0, 1);
        if (! unlocked)
            return;
        ::syscall(__NR_futex, &data->sem2, FUTEX_WAKE, 1, nullptr, nullptr, 0);
    }

    bool wait()
    {
        const timespec timeout = { 1, 0 };

        for (;;)
        {
            if (__sync_bool_compare_and_swap(&data->sem1, 1, 0))
                return true;

            if (::syscall(__NR_futex, &data->sem1, FUTEX_WAIT, 0, &timeout, nullptr, 0) != 0)
                if (errno != EAGAIN && errno != EINTR)
                    return false;
        }
    }
   #endif

    jack_native_thread_t fProcessThread;
    bool fIsProcessing, fIsRunning;

    static void* on_process(void* const arg)
    {
        static_cast<DesktopAudioDriver*>(arg)->_process();
        return nullptr;
    }

    void _process()
    {
        if (set_threaded_log_function())
        {
        }

        while (fIsProcessing && fIsRunning)
        {
            if (! wait())
                continue;

            CycleTakeBeginTime();

            if (Process() != 0)
                fIsProcessing = false;

            post();
        }
    }

public:

    DesktopAudioDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
        : JackAudioDriver(name, alias, engine, table),
          fShmData(nullptr),
         #ifndef _WIN32
          fShmFd(-1),
         #endif
          fIsProcessing(false),
          fIsRunning(false)
    {
        printf("%03d:%s\n", __LINE__, __FUNCTION__);
    }

    ~DesktopAudioDriver() override
    {
        printf("%03d:%s\n", __LINE__, __FUNCTION__);
    }

    int Open(jack_nframes_t buffer_size,
             jack_nframes_t samplerate,
             bool capturing,
             bool playing,
             int chan_in,
             int chan_out,
             bool monitor,
             const char* capture_driver_name,
             const char* playback_driver_name,
             jack_nframes_t capture_latency,
             jack_nframes_t playback_latency) override
    {
        printf("%03d:%s\n", __LINE__, __FUNCTION__);
        if (JackAudioDriver::Open(buffer_size, samplerate, capturing, playing, chan_in, chan_out, monitor,
            capture_driver_name, playback_driver_name, capture_latency, playback_latency) != 0) {
            return -1;
        }

       #ifdef _WIN32
       #else
        fShmFd = shm_open("/mod-desktop-test1", O_RDWR, 0);
        if (fShmFd < 0)
        {
            Close();
            jack_error("Can't open default MOD Desktop driver 1");
            return -1;
        }

        void* ptr;

       #ifdef MAP_LOCKED
        ptr = mmap(nullptr, kDataSize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fShmFd, 0);
        if (ptr == nullptr || ptr == MAP_FAILED)
       #endif
        {
            ptr = mmap(nullptr, kDataSize, PROT_READ|PROT_WRITE, MAP_SHARED, fShmFd, 0);
        }

        if (ptr == nullptr || ptr == MAP_FAILED)
        {
            Close();
            jack_error("Can't open default MOD Desktop driver 2");
            return -1;
        }

       #ifndef MAP_LOCKED
        mlock(ptr, kDataSize);
       #endif

        fShmData = static_cast<Data*>(ptr);
       #endif

        if (fShmData->magic != 1337)
        {
            Close();
            jack_error("Can't open default MOD Desktop driver 3");
            return -1;
        }

       #ifdef __APPLE__
        task = mach_task_self();

        mach_port_t bootport1;
        if (task_get_bootstrap_port(task, &bootport1) != KERN_SUCCESS ||
            bootstrap_look_up(bootport1, fShmData->bootname1, &sem1) != KERN_SUCCESS)
        {
            Close();
            jack_error("Can't open default MOD Desktop driver 4");
            return -1;
        }

        mach_port_t bootport2;
        if (task_get_bootstrap_port(task, &bootport2) != KERN_SUCCESS ||
            bootstrap_look_up(bootport2, fShmData->bootname2, &sem2) != KERN_SUCCESS)
        {
            Close();
            jack_error("Can't open default MOD Desktop driver 5");
            return -1;
        }
       #endif

        return 0;
    }

    int Close() override
    {
        printf("%03d:%s\n", __LINE__, __FUNCTION__);
        JackAudioDriver::Close();

       #ifdef __APPLE__
        if (sem1 != MACH_PORT_NULL)
        {
            semaphore_destroy(task, sem1);
            sem1 = MACH_PORT_NULL;
        }

        if (sem2 != MACH_PORT_NULL)
        {
            semaphore_destroy(task, sem2);
            sem2 = MACH_PORT_NULL;
        }
       #endif

       #ifdef _WIN32
       #else
        if (fShmData != nullptr)
        {
            ::munmap(fShmData, kDataSize);
            fShmData = nullptr;
        }

        if (fShmFd >= 0)
        {
            close(fShmFd);
            fShmFd = -1;
        }
       #endif

        return 0;
    }

//         int Attach() override
//         {
//         }

    int Start() override
    {
        printf("%03d:%s\n", __LINE__, __FUNCTION__);
        if (JackAudioDriver::Start() != 0)
            return -1;

       #if 0 // def __APPLE__
        fEngineControl->fPeriod = fEngineControl->fPeriodUsecs * 1000;
        fEngineControl->fComputation = JackTools::ComputationMicroSec(fEngineControl->fBufferSize) * 1000;
        fEngineControl->fConstraint = fEngineControl->fPeriodUsecs * 1000;
       #endif

        fIsProcessing = fIsRunning = true;
        return JackPosixThread::StartImp(&fProcessThread, 0, 0, on_process, this);
    }

    int Stop() override
    {
        printf("%03d:%s\n", __LINE__, __FUNCTION__);
        fIsProcessing = false;

        if (fIsRunning)
        {
            fIsRunning = false;
            JackPosixThread::StopImp(fProcessThread);
        }

        return JackAudioDriver::Stop();
    }

    int Read() override
    {
        memcpy(GetInputBuffer(0), fShmData->audio, sizeof(float) * 128);
        memcpy(GetInputBuffer(1), fShmData->audio + 128, sizeof(float) * 128);
        return 0;
    }

    int Write() override
    {
        memcpy(fShmData->audio, GetOutputBuffer(0), sizeof(float) * 128);
        memcpy(fShmData->audio + 128, GetOutputBuffer(1), sizeof(float) * 128);
        return 0;
    }

    bool IsFixedBufferSize() override
    {
        printf("%03d:%s\n", __LINE__, __FUNCTION__);
        return true;
    }
};

} // end of namespace

extern "C" {

#include "JackCompilerDeps.h"

SERVER_EXPORT jack_driver_desc_t* driver_get_descriptor()
{
    printf("%03d:%s\n", __LINE__, __FUNCTION__);
    jack_driver_desc_filler_t filler;
    jack_driver_param_value_t value;

    jack_driver_desc_t* const desc = jack_driver_descriptor_construct("desktop", JackDriverMaster, "MOD Desktop plugin audio backend", &filler);

    value.ui = 48000U;
    jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

    return desc;
}

SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
{
    jack_nframes_t srate = 48000;

    for (const JSList* node = params; node; node = jack_slist_next(node))
    {
        const jack_driver_param_t* const param = (const jack_driver_param_t *) node->data;

        switch (param->character)
        {
        case 'r':
            srate = param->value.ui;
            break;
        }
    }

    Jack::JackDriverClientInterface* driver = new Jack::DesktopAudioDriver("system", "mod-desktop", engine, table);

    if (driver->Open(128, srate, true, true, 2, 2, false, "", "", 0, 0) == 0)
    {
        printf("%03d:%s OK\n", __LINE__, __FUNCTION__);
        return driver;
    }

    printf("%03d:%s FAIL\n", __LINE__, __FUNCTION__);
    delete driver;
    return nullptr;
}

} // extern "C"
