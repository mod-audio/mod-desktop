// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "DistrhoUtils.hpp"

#ifndef DISTRHO_OS_WINDOWS
# include <cerrno>
# include <fcntl.h>
# include <sys/mman.h>
# ifdef DISTRHO_OS_MAC
extern "C" {
int __ulock_wait(uint32_t operation, void* addr, uint64_t value, uint32_t timeout_us);
int __ulock_wake(uint32_t operation, void* addr, uint64_t value);
}
# else
#  include <syscall.h>
#  include <sys/time.h>
#  include <linux/futex.h>
# endif
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class SharedMemory
{
public:
    struct Data {
        uint32_t magic;
        int32_t padding1;
       #ifdef DISTRHO_OS_WINDOWS
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
    }* data = nullptr;

   #ifndef DISTRHO_OS_WINDOWS
    uint port = 0;
   #endif

    SharedMemory()
    {
    }

    ~SharedMemory()
    {
        deinit();
    }

    static bool canInit(const uint portBaseNum)
    {
        char shmName[32] = {};

      #ifdef DISTRHO_OS_WINDOWS
        std::snprintf(shmName, 31, "Local\\mod-desktop-shm-%d", portBaseNum);
        const HANDLE testshm = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shmName);

        if (testshm == nullptr)
        {
            d_stderr("SharedMemory::canInit(%u) - true", portBaseNum);
            return true;
        }

        CloseHandle(testshm);
       #else
        std::snprintf(shmName, 31, "/mod-desktop-shm-%d", portBaseNum);
        const int testshmfd = shm_open(shmName, O_RDONLY, 0);

        if (testshmfd < 0)
        {
            d_stderr("SharedMemory::canInit(%u) - true", portBaseNum);
            return true;
        }

        close(testshmfd);
       #endif

        d_stderr("SharedMemory::canInit(%u) - false", portBaseNum);
        return false;
    }

    bool init(const uint portBaseNum)
    {
        void* ptr;
        char shmName[32] = {};

      #ifdef DISTRHO_OS_WINDOWS
        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        std::snprintf(shmName, 31, "Local\\mod-desktop-shm-%d", portBaseNum);

        shm = CreateFileMappingA(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE|SEC_COMMIT, 0, static_cast<DWORD>(kDataSize), shmName);
        DISTRHO_SAFE_ASSERT_RETURN(shm != nullptr, false);

        ptr = MapViewOfFile(shm, FILE_MAP_ALL_ACCESS, 0, 0, kDataSize);
        DISTRHO_SAFE_ASSERT_RETURN(ptr != nullptr, fail_deinit());

        VirtualLock(ptr, kDataSize);
      #else
        std::snprintf(shmName, 31, "/mod-desktop-shm-%d", portBaseNum);

        shmfd = shm_open(shmName, O_CREAT|O_EXCL|O_RDWR, 0666);
        DISTRHO_CUSTOM_SAFE_ASSERT_RETURN(std::strerror(errno), shmfd >= 0, false);

        DISTRHO_CUSTOM_SAFE_ASSERT_RETURN(std::strerror(errno), ftruncate(shmfd, static_cast<off_t>(kDataSize)) == 0, fail_deinit());

       #ifdef MAP_LOCKED
        ptr = mmap(nullptr, kDataSize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, shmfd, 0);
        if (ptr == nullptr || ptr == MAP_FAILED)
       #endif
        {
            ptr = mmap(nullptr, kDataSize, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0);
        }
        DISTRHO_CUSTOM_SAFE_ASSERT_RETURN(std::strerror(errno), ptr != nullptr, fail_deinit());
        DISTRHO_CUSTOM_SAFE_ASSERT_RETURN(std::strerror(errno), ptr != MAP_FAILED, fail_deinit());

       #ifndef MAP_LOCKED
        mlock(ptr, kDataSize);
       #endif
      #endif

        data = static_cast<Data*>(ptr);

        std::memset(data, 0, kDataSize);
        data->magic = 1337;

       #ifdef DISTRHO_OS_WINDOWS
        data->sem1 = CreateSemaphoreA(&sa, 0, 1, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(data->sem1 != nullptr, fail_deinit());

        data->sem2 = CreateSemaphoreA(&sa, 0, 1, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(data->sem2 != nullptr, fail_deinit());
       #else
        port = portBaseNum;
       #endif

        return true;
    }

    void deinit()
    {
       #ifdef DISTRHO_OS_WINDOWS
        if (data != nullptr)
        {
            if (data->sem1 != nullptr)
            {
                CloseHandle(data->sem1);
                data->sem1 = nullptr;
            }

            if (data->sem2 != nullptr)
            {
                CloseHandle(data->sem2);
                data->sem2 = nullptr;
            }

            UnmapViewOfFile(data);
            data = nullptr;
        }

        if (shm != nullptr)
        {
            CloseHandle(shm);
            shm = nullptr;
        }
       #else
        if (data != nullptr)
        {
            munmap(data, kDataSize);
            data = nullptr;
        }

        if (shmfd >= 0)
        {
            close(shmfd);
            shmfd = -1;
        }

        if (port != 0)
        {
            char shmName[32] = {};
            std::snprintf(shmName, 31, "/mod-desktop-shm-%d", port);
            shm_unlink(shmName);
            port = 0;
        }
       #endif
    }

    // ----------------------------------------------------------------------------------------------------------------

    bool sync()
    {
        if (data == nullptr)
            return false;

        data->midiEventCount = 0;
        std::memset(data->audio, 0, sizeof(float) * 128 * 2);

        post();
        return wait();
    }

    void stopWait()
    {
        if (data == nullptr)
            return;

        data->magic = 7331;
        data->midiEventCount = 0;
        std::memset(data->audio, 0, sizeof(float) * 128 * 2);

        post();
        if (wait())
            data->magic = 1337;
    }

    bool process()
    {
        // unlock RT waiter
        post();

        // wait for processing
        return wait();
    }

private:
    // ----------------------------------------------------------------------------------------------------------------
    // shared memory details

   #ifdef DISTRHO_OS_WINDOWS
    HANDLE shm = nullptr;
   #else
    int shmfd = -1;
   #endif

    static constexpr const size_t kDataSize = sizeof(Data) + sizeof(float) * 128 * 2;

    // ----------------------------------------------------------------------------------------------------------------
    // semaphore details

    void post()
    {
      #if defined(DISTRHO_OS_WINDOWS)
        ReleaseSemaphore(data->sem1, 1, nullptr);
      #else
        const bool unlocked = __sync_bool_compare_and_swap(&data->sem1, 0, 1);
        DISTRHO_SAFE_ASSERT_RETURN(unlocked,);
       #ifdef DISTRHO_OS_MAC
        __ulock_wake(0x1000003, &data->sem1, 0);
       #else
        syscall(__NR_futex, &data->sem1, FUTEX_WAKE, 1, nullptr, nullptr, 0);
       #endif
      #endif
    }

    bool wait()
    {
      #if defined(DISTRHO_OS_WINDOWS)
        return WaitForSingleObject(data->sem2, 1000) == WAIT_OBJECT_0;
      #else
       #ifdef DISTRHO_OS_MAC
        const uint32_t timeout = 1000000;
       #else
        const timespec timeout = { 1, 0 };
       #endif

        for (;;)
        {
            if (__sync_bool_compare_and_swap(&data->sem2, 1, 0))
                return true;

           #ifdef DISTRHO_OS_MAC
            if (__ulock_wait(0x3, &data->sem2, 0, timeout) != 0)
           #else
            if (syscall(__NR_futex, &data->sem2, FUTEX_WAIT, 0, &timeout, nullptr, 0) != 0)
           #endif
                if (errno != EAGAIN && errno != EINTR)
                    return false;
        }
       #endif
    }

   bool fail_deinit()
   {
       deinit();
       return false;
   }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedMemory)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
