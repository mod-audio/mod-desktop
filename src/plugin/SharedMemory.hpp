// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "DistrhoUtils.hpp"

#ifdef DISTRHO_OS_WINDOWS
#else
# include <fcntl.h>
# include <sys/mman.h>
# ifdef DISTRHO_OS_MAC
#  include "extra/Runner.hpp"
#  include "extra/ScopedPointer.hpp"
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
// Based on jack2 mach semaphore implementation
// Copyright (C) 2021 Peter Bridgman

#ifdef DISTRHO_OS_MAC
class SemaphoreServerRunner : public Runner {
    const mach_port_t port;
    const semaphore_t sem;
    bool running = true;

public:
    SemaphoreServerRunner(const mach_port_t p, const semaphore_t s)
        : Runner(),
          port(p),
          sem(s)
    {
        startRunner(0);
    }

    void invalidate()
    {
        running = false;

        const mach_port_t task = mach_task_self();
        mach_port_destroy(task, port);
        semaphore_destroy(task, sem);

        stopRunner();
    }

protected:
    bool run() override
    {
        struct {
            mach_msg_header_t hdr;
            mach_msg_trailer_t trailer;
        } msg;

        if (mach_msg(&msg.hdr, MACH_RCV_MSG, 0, sizeof(msg), port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL) != MACH_MSG_SUCCESS)
            return running;

        msg.hdr.msgh_local_port = sem;
        msg.hdr.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE, MACH_MSG_TYPE_COPY_SEND);
        mach_msg(&msg.hdr, MACH_SEND_MSG, sizeof(msg.hdr), 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,MACH_PORT_NULL);

        return running;
    }
};
#endif

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
        std::snprintf(shmName, 31, "\\Global\\mod-desktop-shm-%d", portBaseNum);
        const HANDLE shm = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shmName);

        if (shm == nullptr)
            return true;

        CloseHandle(shm);
       #else
        std::snprintf(shmName, 31, "/mod-desktop-shm-%d", portBaseNum);
        const int fd = shm_open(shmName, O_RDWR, 0);

        if (fd < 0)
            return true;

        close(fd);
        shm_unlink(shmName);
       #endif

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

        shmfd = shm_open(shmName, O_CREAT|O_EXCL|O_RDWR, 0600);
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

       #ifdef DISTRHO_OS_MAC
        task = mach_task_self();

        mach_port_t bootport;
        DISTRHO_SAFE_ASSERT_RETURN(task_get_bootstrap_port(task, &bootport) == KERN_SUCCESS, fail_deinit());
        DISTRHO_SAFE_ASSERT_RETURN(semaphore_create(task, &sem1, SYNC_POLICY_FIFO, 0) == KERN_SUCCESS, fail_deinit());
        DISTRHO_SAFE_ASSERT_RETURN(semaphore_create(task, &sem2, SYNC_POLICY_FIFO, 0) == KERN_SUCCESS, fail_deinit());

        DISTRHO_SAFE_ASSERT_RETURN(mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &port1) == KERN_SUCCESS, fail_deinit());
        DISTRHO_SAFE_ASSERT_RETURN(mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &port2) == KERN_SUCCESS, fail_deinit());

        DISTRHO_SAFE_ASSERT_RETURN(mach_port_insert_right(task, port1, port1, MACH_MSG_TYPE_MAKE_SEND) == KERN_SUCCESS, fail_deinit());
        DISTRHO_SAFE_ASSERT_RETURN(mach_port_insert_right(task, port2, port2, MACH_MSG_TYPE_MAKE_SEND) == KERN_SUCCESS, fail_deinit());

        std::snprintf(data->bootname1, 31, "audio.mod.desktop.port%d.1", portBaseNum);
        std::snprintf(data->bootname2, 31, "audio.mod.desktop.port%d.2", portBaseNum);
        data->bootname1[31] = data->bootname2[31] = '\0';

       #pragma clang diagnostic push
       #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        DISTRHO_SAFE_ASSERT_RETURN(bootstrap_register(bootport, data->bootname1, port1) == KERN_SUCCESS, fail_deinit());
        DISTRHO_SAFE_ASSERT_RETURN(bootstrap_register(bootport, data->bootname2, port2) == KERN_SUCCESS, fail_deinit());
       #pragma clang diagnostic pop

        semServer1 = new SemaphoreServerRunner(port1, sem1);
        semServer2 = new SemaphoreServerRunner(port2, sem2);
       #endif

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
       #ifdef DISTRHO_OS_MAC
        if (semServer1 != nullptr)
        {
            semServer1->invalidate();
            port1 = MACH_PORT_NULL;
            sem1 = MACH_PORT_NULL;
            semServer1 = nullptr;
        }

        if (semServer2 != nullptr)
        {
            semServer2->invalidate();
            port2 = MACH_PORT_NULL;
            sem2 = MACH_PORT_NULL;
            semServer2 = nullptr;
        }

        if (port1 != MACH_PORT_NULL)
        {
            mach_port_destroy(task, port1);
            port1 = MACH_PORT_NULL;
        }

        if (port2 != MACH_PORT_NULL)
        {
            mach_port_destroy(task, port2);
            port2 = MACH_PORT_NULL;
        }

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

    void reset()
    {
        if (data == nullptr)
            return;

        data->midiEventCount = 0;
        std::memset(data->audio, 0, sizeof(float) * 128 * 2);
    }

    bool sync()
    {
        if (data == nullptr)
            return false;

        reset();
        post();
        return wait();
    }

    void stopWait()
    {
        if (data == nullptr)
            return;

        data->magic = 7331;
        post();
        wait();
        data->magic = 1337;
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
    mach_port_t port1 = MACH_PORT_NULL;
    mach_port_t port2 = MACH_PORT_NULL;
    semaphore_t sem1 = MACH_PORT_NULL;
    semaphore_t sem2 = MACH_PORT_NULL;
    ScopedPointer<SemaphoreServerRunner> semServer1;
    ScopedPointer<SemaphoreServerRunner> semServer2;
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
