/*
*/

#include "JackAudioDriver.h"
#include "JackDriverLoader.h"
#include "JackEngineControl.h"
#include "JackTools.h"

#include <cerrno>
#include <ctime>

#include <fcntl.h>
#include <signal.h>
#include <syscall.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <linux/futex.h>

namespace Jack
{

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

// -----------------------------------------------------------------------------------------------------------

static inline
void shm_sem_rt_post(SharedMemoryData* const data)
{
    const bool unlocked = __sync_bool_compare_and_swap(&data->sem2, 0, 1);
    if (! unlocked)
        return;
    ::syscall(__NR_futex, &data->sem2, FUTEX_WAKE, 1, nullptr, nullptr, 0);
}

static inline
bool shm_sem_rt_wait(SharedMemoryData* const data)
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

// -----------------------------------------------------------------------------------------------------------

class DesktopAudioDriver : public JackAudioDriver
{
    private:
        SharedMemoryData* fShmData;
        int fShmFd;
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
                if (! shm_sem_rt_wait(fShmData))
                    continue;

                CycleTakeBeginTime();

                if (Process() != 0)
                    fIsProcessing = false;

                shm_sem_rt_post(fShmData);
            }
        }

    public:

        DesktopAudioDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
            : JackAudioDriver(name, alias, engine, table),
              fShmData(nullptr),
              fShmFd(-1),
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

            fShmFd = shm_open("/mod-desktop-test1", O_RDWR, 0);
            if (fShmFd < 0)
            {
                Close();
                jack_error("Can't open default MOD Desktop driver 1");
                return -1;
            }

            void* const ptr = ::mmap(nullptr, kSharedMemoryDataSize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fShmFd, 0);
            if (ptr == nullptr || ptr == MAP_FAILED)
            {
                Close();
                jack_error("Can't open default MOD Desktop driver 2");
                return -1;
            }

            fShmData = static_cast<SharedMemoryData*>(ptr);
            if (fShmData->magic != 1337)
            {
                Close();
                jack_error("Can't open default MOD Desktop driver 3");
                return -1;
            }

            return 0;
        }

        int Close() override
        {
            printf("%03d:%s\n", __LINE__, __FUNCTION__);
            JackAudioDriver::Close();

            if (fShmData != nullptr)
            {
                ::munmap(fShmData, kSharedMemoryDataSize);
                fShmData = nullptr;
            }

            if (fShmFd >= 0)
            {
                close(fShmFd);
                fShmFd = -1;
            }

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

            fEngineControl->fPeriod = fEngineControl->fPeriodUsecs * 1000;
            fEngineControl->fComputation = JackTools::ComputationMicroSec(fEngineControl->fBufferSize) * 1000;
            fEngineControl->fConstraint = fEngineControl->fPeriodUsecs * 1000;

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
    return jack_driver_descriptor_construct("desktop", JackDriverMaster, "MOD Desktop plugin audio backend", &filler);
}

SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
{
    printf("zzz %03d:%s\n", __LINE__, __FUNCTION__);
    Jack::JackDriverClientInterface* driver = new Jack::DesktopAudioDriver("system", "mod-desktop", engine, table);
    printf("zzz %03d:%s\n", __LINE__, __FUNCTION__);

    if (driver->Open(128, 48000, true, true, 2, 2, false, "", "", 0, 0) == 0)
    {
        printf("%03d:%s OK\n", __LINE__, __FUNCTION__);
        return driver;
    }

    printf("%03d:%s FAIL\n", __LINE__, __FUNCTION__);
    delete driver;
    return nullptr;
}

} // extern "C"
