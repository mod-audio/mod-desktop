/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_AUDIO_RING_BUFFER_HPP_INCLUDED
#define DISTRHO_AUDIO_RING_BUFFER_HPP_INCLUDED

#include "DistrhoUtils.hpp"

#ifdef DISTRHO_OS_WINDOWS
# include <winsock2.h>
# include <windows.h>
#else
# include <sys/mman.h>
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// AudioRingBuffer class

class AudioRingBuffer
{
public:
    /*
     * Constructor for uninitialised ring buffer.
     * A call to setRingBuffer is required to tied this control to a ring buffer struct;
     *
     */
    AudioRingBuffer() noexcept {}

    /*
     * Destructor.
     */
    ~AudioRingBuffer() noexcept
    {
        deleteBuffer();
    }

    bool createBuffer(const uint8_t numChannels, const uint32_t numSamples) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(buffer.buf == nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(numChannels > 0, false);
        DISTRHO_SAFE_ASSERT_RETURN(numSamples > 0, false);

        const uint32_t p2samples = d_nextPowerOf2(numSamples);

        try {
            buffer.buf = new float*[numChannels];
            for (uint8_t c=0; c<numChannels; ++c)
                buffer.buf[c] = new float[p2samples];
        } DISTRHO_SAFE_EXCEPTION_RETURN("HeapRingBuffer::createBuffer", false);

        buffer.samples = p2samples;
        buffer.channels = numChannels;
        buffer.head = buffer.tail = 0;
        errorReading = errorWriting = false;

       #ifdef DISTRHO_OS_WINDOWS
        VirtualLock(buffer.buf, sizeof(float*) * numChannels);

        for (uint8_t c=0; c<numChannels; ++c)
            VirtualLock(buffer.buf[c], sizeof(float) * p2samples);
       #else
        mlock(buffer.buf, sizeof(float*) * numChannels);

        for (uint8_t c=0; c<numChannels; ++c)
            mlock(buffer.buf[c], sizeof(float) * p2samples);
       #endif

        return true;
    }

    /** Delete the previously allocated buffer. */
    void deleteBuffer() noexcept
    {
        if (buffer.buf == nullptr)
            return;

        for (uint8_t c=0; c<buffer.channels; ++c)
            delete[] buffer.buf[c];
        delete[] buffer.buf;
        buffer.buf  = nullptr;

        buffer.samples = buffer.head = buffer.tail = 0;
        buffer.channels = 0;
    }

    // ----------------------------------------------------------------------------------------------------------------

    uint32_t getNumSamples() const noexcept
    {
        return buffer.samples;
    }

    uint32_t getNumReadableSamples() const noexcept
    {
        const uint32_t wrap = buffer.head >= buffer.tail ? 0 : buffer.samples;

        return wrap + buffer.head - buffer.tail;
    }

    uint32_t getNumWritableSamples() const noexcept
    {
        const uint32_t wrap = buffer.tail > buffer.head ? 0 : buffer.samples;

        return wrap + buffer.tail - buffer.head - 1;
    }

    // ----------------------------------------------------------------------------------------------------------------

    /*
     * Reset the ring buffer read and write positions, marking the buffer as empty.
     * Requires a buffer struct tied to this class.
     */
    void flush() noexcept
    {
        buffer.head = buffer.tail = 0;
        errorWriting = false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    bool read(float* const* const buffers, const uint32_t samples) noexcept
    {
        // empty
        if (buffer.head == buffer.tail)
            return false;

        const uint32_t head = buffer.head;
        const uint32_t tail = buffer.tail;
        const uint32_t wrap = head > tail ? 0 : buffer.samples;

        if (samples > wrap + head - tail)
        {
            if (! errorReading)
            {
                errorReading = true;
                d_stderr2("RingBuffer::tryRead(%p, %u): failed, not enough space", buffers, samples);
            }
            return false;
        }

        uint32_t readto = tail + samples;

        if (readto > buffer.samples)
        {
            readto -= buffer.samples;

            if (samples == 1)
            {
                for (uint8_t c=0; c<buffer.channels; ++c)
                    std::memcpy(buffers[c], buffer.buf[c] + tail, sizeof(float));
            }
            else
            {
                const uint32_t firstpart = buffer.samples - tail;

                for (uint8_t c=0; c<buffer.channels; ++c)
                {
                    std::memcpy(buffers[c], buffer.buf[c] + tail, firstpart * sizeof(float));
                    std::memcpy(buffers[c] + firstpart, buffer.buf[c], readto * sizeof(float));
                }
            }
        }
        else
        {
            for (uint8_t c=0; c<buffer.channels; ++c)
                std::memcpy(buffers[c], buffer.buf[c] + tail, samples * sizeof(float));

            if (readto == buffer.samples)
                readto = 0;
        }

        buffer.tail = readto;
        errorReading = false;
        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    bool write(const float* const* const buffers, const uint32_t samples) noexcept
    {
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(samples < buffer.samples, samples, buffer.samples, false);

        const uint32_t head = buffer.head;
        const uint32_t tail = buffer.tail;
        const uint32_t wrap = tail > head ? 0 : buffer.samples;

        if (samples >= wrap + tail - head)
        {
            if (! errorWriting)
            {
                errorWriting = true;
                d_stderr2("RingBuffer::tryWrite(%p, %u): failed, not enough space", buffers, samples);
            }
            return false;
        }

        uint32_t writeto = head + samples;

        if (writeto > buffer.samples)
        {
            writeto -= buffer.samples;

            if (samples == 1)
            {
                for (uint8_t c=0; c<buffer.channels; ++c)
                    std::memcpy(buffer.buf[c], buffers[c], sizeof(float));
            }
            else
            {
                const uint32_t firstpart = buffer.samples - head;

                for (uint8_t c=0; c<buffer.channels; ++c)
                {
                    std::memcpy(buffer.buf[c] + head, buffers[c], firstpart * sizeof(float));
                    std::memcpy(buffer.buf[c], buffers[c] + firstpart, writeto * sizeof(float));
                }
            }
        }
        else
        {
            for (uint8_t c=0; c<buffer.channels; ++c)
                std::memcpy(buffer.buf[c] + head, buffers[c], samples * sizeof(float));

            if (writeto == buffer.samples)
                writeto = 0;
        }

        buffer.head = writeto;
        errorWriting = false;
        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    /** Buffer struct. */
    struct Buffer {
        uint32_t samples;
        uint32_t head;
        uint32_t tail;
        uint8_t channels;
        float** buf;
    } buffer = { 0, 0, 0, 0, nullptr };

    /** Whether read errors have been printed to terminal. */
    bool errorReading = false;

    /** Whether write errors have been printed to terminal. */
    bool errorWriting = false;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRingBuffer)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_AUDIO_RING_BUFFER_HPP_INCLUDED
