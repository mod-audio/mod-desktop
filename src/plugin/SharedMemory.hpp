// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "DistrhoUtils.hpp"

#if defined(DISTRHO_OS_MAC)
#elif defined(DISTRHO_OS_WINDOWS)
#else
# include <cerrno>
# include <fcntl.h>
# include <syscall.h>
# include <linux/futex.h>
# include <sys/mman.h>
# include <sys/time.h>
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class SharedMemory
{
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
       #if defined(DISTRHO_OS_MAC)
       #elif defined(DISTRHO_OS_WINDOWS)
       #else
        fd = shm_open("/mod-desktop-test1", O_CREAT|O_EXCL|O_RDWR, 0600);
        DISTRHO_SAFE_ASSERT_RETURN(fd >= 0, false);

        {
            const int ret = ::ftruncate(fd, static_cast<off_t>(kDataSize));
            DISTRHO_SAFE_ASSERT_RETURN(ret == 0, fail_deinit());
        }

        {
            void* const ptr = ::mmap(nullptr, kDataSize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fd, 0);
            DISTRHO_SAFE_ASSERT_RETURN(ptr != nullptr, fail_deinit());
            DISTRHO_SAFE_ASSERT_RETURN(ptr != MAP_FAILED, fail_deinit());

            data = static_cast<Data*>(ptr);
        }
       #endif

        std::memset(data, 0, kDataSize);
        data->magic = 1337;

        return true;
    }

    void deinit()
    {
       #if defined(DISTRHO_OS_MAC)
       #elif defined(DISTRHO_OS_WINDOWS)
       #else
        if (data != nullptr)
        {
            ::munmap(data, kDataSize);
            data = nullptr;
        }

        if (fd >= 0)
        {
            close(fd);
            shm_unlink("/mod-desktop-test1");
            fd = -1;
        }
       #endif
    }

    void getAudioData(float* audio[2])
    {
        DISTRHO_SAFE_ASSERT_RETURN(data != nullptr,);

        audio[0] = data->audio;
        audio[1] = data->audio + 128;
    }

    void reset()
    {
        if (data == nullptr)
            return;

        data->midiEventCount = 0;
        std::memset(data->audio, 0, sizeof(float) * 128 * 2);
    }

    bool process(float** output, const uint32_t offset)
    {
        // unlock RT waiter
        post();

        // wait for processing
        if (! wait())
            return false;

        // copy processed buffer
        std::memcpy(output[0] + offset, data->audio, sizeof(float) * 128);
        std::memcpy(output[1] + offset, data->audio + 128, sizeof(float) * 128);

        return true;
    }

private:
    struct Data {
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
    }* data = nullptr;

    static constexpr const size_t kDataSize = sizeof(Data) + sizeof(float) * 128 * 2;

   #if defined(DISTRHO_OS_MAC)
    void post()
    {
    }

    bool wait()
    {
        return false;
    }
   #elif defined(DISTRHO_OS_WINDOWS)
    void post()
    {
    }

    bool wait()
    {
        return false;
    }
   #else
    int fd = -1;

    void post()
    {
        const bool unlocked = __sync_bool_compare_and_swap(&data->sem1, 0, 1);
        DISTRHO_SAFE_ASSERT_RETURN(unlocked,);
        ::syscall(__NR_futex, &data->sem1, FUTEX_WAKE, 1, nullptr, nullptr, 0);
    }

    bool wait()
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
   #endif

   bool fail_deinit()
   {
       deinit();
       return false;
   }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedMemory)
};

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
