// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "DistrhoUtils.hpp"

#ifdef DISTRHO_OS_WINDOWS
#else
# include <fcntl.h>
# include <sys/mman.h>
# ifdef DISTRHO_OS_MAC
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

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class SharedMemory
{
public:
    struct Data {
        uint32_t magic;
        int32_t padding1;
       #if defined(DISTRHO_OS_MAC)
        char bootname1[32];
        char bootname2[32];
       #elif defined(DISTRHO_OS_WINDOWS)
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

    SharedMemory()
    {
    }

    ~SharedMemory()
    {
        deinit();
    }

    bool init()
    {
        void* ptr;

      #ifdef DISTRHO_OS_WINDOWS
        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        shm = CreateFileMappingA(INVALID_HANDLE_VALUE,
                                 &sa,
                                 PAGE_READWRITE|SEC_COMMIT,
                                 0,
                                 static_cast<DWORD>(kDataSize),
                                 "/mod-desktop-test1");
        DISTRHO_SAFE_ASSERT_RETURN(shm != nullptr, false);

        ptr = MapViewOfFile(shm, FILE_MAP_ALL_ACCESS, 0, 0, kDataSize);
        DISTRHO_SAFE_ASSERT_RETURN(ptr != nullptr, fail_deinit());

        VirtualLock(ptr, kDataSize);
      #else
        // FIXME
        shm_unlink("/mod-desktop-test1");

        shmfd = shm_open("/mod-desktop-test1", O_CREAT|O_EXCL|O_RDWR, 0600);
        DISTRHO_SAFE_ASSERT_RETURN(shmfd >= 0, false);

        DISTRHO_SAFE_ASSERT_RETURN(ftruncate(shmfd, static_cast<off_t>(kDataSize)) == 0, fail_deinit());

       #ifdef MAP_LOCKED
        ptr = mmap(nullptr, kDataSize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, shmfd, 0);
        if (ptr == nullptr || ptr == MAP_FAILED)
       #endif
        {
            ptr = mmap(nullptr, kDataSize, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0);
        }
        DISTRHO_SAFE_ASSERT_RETURN(ptr != nullptr, fail_deinit());
        DISTRHO_SAFE_ASSERT_RETURN(ptr != MAP_FAILED, fail_deinit());

       #ifndef MAP_LOCKED
        mlock(ptr, kDataSize);
       #endif
      #endif

        data = static_cast<Data*>(ptr);

        std::memset(data, 0, kDataSize);
        data->magic = 1337;

      #if defined(DISTRHO_OS_MAC)
        task = mach_task_self();

        mach_port_t bootport1, bootport2;
        DISTRHO_SAFE_ASSERT_RETURN(task_get_bootstrap_port(task, &bootport1) == KERN_SUCCESS, fail_deinit());
        DISTRHO_SAFE_ASSERT_RETURN(task_get_bootstrap_port(task, &bootport2) == KERN_SUCCESS, fail_deinit());
        DISTRHO_SAFE_ASSERT_RETURN(semaphore_create(task, &sem1, SYNC_POLICY_FIFO, 0) == KERN_SUCCESS, fail_deinit());
        DISTRHO_SAFE_ASSERT_RETURN(semaphore_create(task, &sem2, SYNC_POLICY_FIFO, 0) == KERN_SUCCESS, fail_deinit());

        static int bootcounter = 0;
        std::snprintf(data->bootname1, 31, "mdskt_%d_%d_%p", ++bootcounter, getpid(), &sem1);
        std::snprintf(data->bootname2, 31, "mdskt_%d_%d_%p", ++bootcounter, getpid(), &sem2);
        data->bootname1[31] = data->bootname2[31] = '\0';

       #pragma clang diagnostic push
       #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        DISTRHO_SAFE_ASSERT_RETURN(bootstrap_register(bootport1, data->bootname1, sem1) == KERN_SUCCESS, fail_deinit());
        DISTRHO_SAFE_ASSERT_RETURN(bootstrap_register(bootport2, data->bootname2, sem2) == KERN_SUCCESS, fail_deinit());
       #pragma clang diagnostic pop
      #elif defined(DISTRHO_OS_WINDOWS)
        data->sem1 = CreateSemaphoreA(&sa, 0, 1, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(data->sem1 != nullptr, fail_deinit());

        data->sem2 = CreateSemaphoreA(&sa, 0, 1, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(data->sem2 != nullptr, fail_deinit());
      #endif

        return true;
    }

    void deinit()
    {
       #if defined(DISTRHO_OS_MAC)
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
            shm_unlink("/mod-desktop-test1");
            shmfd = -1;
        }
       #endif
    }

    // ----------------------------------------------------------------------------------------------------------------

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

    // ----------------------------------------------------------------------------------------------------------------

private:
    static constexpr const size_t kDataSize = sizeof(Data) + sizeof(float) * 128 * 2;

    // ----------------------------------------------------------------------------------------------------------------
    // shared memory details

   #ifdef DISTRHO_OS_WINDOWS
    HANDLE shm;
   #else
    int shmfd = -1;
   #endif

   #ifdef DISTRHO_OS_MAC
    mach_port_t task = MACH_PORT_NULL;
    semaphore_t sem1 = MACH_PORT_NULL;
    semaphore_t sem2 = MACH_PORT_NULL;
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // semaphore details

   #if defined(DISTRHO_OS_MAC)
    void post()
    {
        semaphore_signal(sem1);
    }

    bool wait()
    {
        const mach_timespec timeout = { 1, 0 };
        return semaphore_timedwait(sem2, timeout) == KERN_SUCCESS;
    }
   #elif defined(DISTRHO_OS_WINDOWS)
    void post()
    {
        ReleaseSemaphore(data->sem1, 1, nullptr);
    }

    bool wait()
    {
        return WaitForSingleObject(data->sem2, 1000) == WAIT_OBJECT_0;
    }
   #else
    void post()
    {
        const bool unlocked = __sync_bool_compare_and_swap(&data->sem1, 0, 1);
        DISTRHO_SAFE_ASSERT_RETURN(unlocked,);
        syscall(__NR_futex, &data->sem1, FUTEX_WAKE, 1, nullptr, nullptr, 0);
    }

    bool wait()
    {
        const timespec timeout = { 1, 0 };

        for (;;)
        {
            if (__sync_bool_compare_and_swap(&data->sem2, 1, 0))
                return true;

            if (syscall(__NR_futex, &data->sem2, FUTEX_WAIT, 0, &timeout, nullptr, 0) != 0)
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

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
