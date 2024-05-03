// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "DistrhoPlugin.hpp"

#include "AudioRingBuffer.hpp"
#include "ChildProcess.hpp"
#include "SharedMemory.hpp"
#include "extra/RingBuffer.hpp"
#include "extra/Runner.hpp"
#include "extra/ScopedPointer.hpp"
#include "utils.hpp"
#include "zita-resampler/resampler.h"

#include <atomic>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class DesktopPlugin : public Plugin,
                      public Runner
{
    static constexpr const uint kMaxMidiSize = 512 * 4;

    ChildProcess jackd;
    ChildProcess mod_ui;
    SharedMemory shm;
    String currentPedalboard;
    bool startingJackd = false;
    bool startingModUI = false;
    std::atomic<bool> processing { false };
    bool shouldStartRunner = true;
    float parameters[kParameterCount] = {};
    float* tempBuffers[2] = {};
    uint numSamplesInTempBuffers = 0;
    uint numSamplesUntilProcessing = 0;
    int portBaseNum = 0;

    AudioRingBuffer audioBufferIn;
    AudioRingBuffer audioBufferOut;
    ScopedPointer<Resampler> resamplerTo48kHz;
    ScopedPointer<Resampler> resamplerFrom48kHz;
    double resamplerRatio = 1.0;

    double midiFrameOffset = 0.0;
    uint8_t* midiRecvBuffer = nullptr;
    HeapRingBuffer midiRingBuffer;

   #ifdef DISTRHO_OS_WINDOWS
    const WCHAR* envp;
   #else
    char* const* envp;
   #endif

public:
    DesktopPlugin()
        : Plugin(kParameterCount, 0, 1),
          envp(nullptr)
    {
        if (isDummyInstance())
        {
            portBaseNum = -kErrorUndefined;
            return;
        }

        int availablePortNum = 0;
        for (int i = 1; i < 999; ++i)
        {
            if (shm.canInit(i))
            {
                availablePortNum = i;
                break;
            }
        }

        if (availablePortNum == 0)
        {
            d_stderr("MOD Desktop: Failed to find available ports");
            parameters[kParameterBasePortNumber] = portBaseNum = -kErrorShmSetupFailed;
            return;
        }

        envp = getEvironment(availablePortNum);

        if (envp == nullptr)
        {
            d_stderr("MOD Desktop: Failed to init environment");
            parameters[kParameterBasePortNumber] = portBaseNum = -kErrorAppDirNotFound;
            return;
        }

        if (! shm.init(availablePortNum))
        {
            d_stderr("MOD Desktop: Failed to init shared memory");
            parameters[kParameterBasePortNumber] = portBaseNum = -kErrorShmSetupFailed;
            return;
        }

        portBaseNum = availablePortNum;

        setupResampler(getSampleRate());

        d_stderr("MOD Desktop: Initial init ok");
    }

    ~DesktopPlugin()
    {
        stopRunner();

        if (processing && jackd.isRunning())
        {
            processing = false;
            shm.stopWait();
        }

        jackd.stop();
        mod_ui.stop();
        shm.deinit();

        delete[] tempBuffers[0];
        delete[] tempBuffers[1];

        if (envp != nullptr)
        {
           #ifndef DISTRHO_OS_WINDOWS
            for (uint i = 0; envp[i] != nullptr; ++i)
                std::free(envp[i]);
           #endif

            delete[] envp;
        }
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Keep services running */

    bool run() override
    {
       #ifdef DISTRHO_OS_WINDOWS
        #define APP_EXT ".exe"
       #else
        #define APP_EXT ""
       #endif

        if (shm.data == nullptr && ! shm.init(portBaseNum))
        {
            d_stderr("MOD Desktop: Failed to init shared memory inside runner");
            parameters[kParameterBasePortNumber] = portBaseNum = -kErrorShmSetupFailed;
            return false;
        }

        if (! jackd.isRunning())
        {
            if (startingJackd)
            {
                d_stderr("MOD Desktop: Failed to get jackd to run");
                startingJackd = false;
                parameters[kParameterBasePortNumber] = portBaseNum = -kErrorJackdExecFailed;
                return false;
            }

            const String appDir(getAppDir());
            const String jackdStr(appDir + DISTRHO_OS_SEP_STR "jackd" APP_EXT);
            const String jacksessionStr(appDir + DISTRHO_OS_SEP_STR "jack" DISTRHO_OS_SEP_STR "jack-session.conf");
            const String servernameStr("mod-desktop-" + String(portBaseNum));
            const String shmportStr(portBaseNum);

            const char* const jackd_args[] = {
                jackdStr.buffer(),
                "-R",
                "-S",
                "-n", servernameStr.buffer(),
                "-C", jacksessionStr.buffer(),
                "-d", "mod-desktop",
                "-r", "48000",
                "-p", "128",
                "-s", shmportStr,
                nullptr
            };

            startingJackd = true;
            if (jackd.start(jackd_args, envp))
            {
                d_stderr("MOD Desktop: jackd exec ok");
                return true;
            }
 
            d_stderr("MOD Desktop: Failed to start jackd");
            parameters[kParameterBasePortNumber] = portBaseNum = -kErrorJackdExecFailed;
            return false;
        }

        startingJackd = false;

        if (! processing)
        {
            if (shm.sync())
                processing = true;

            return true;
        }

        if (! mod_ui.isRunning())
        {
            if (startingModUI)
            {
                d_stderr("MOD Desktop: Failed to get mod-ui to run");
                startingModUI = false;
                parameters[kParameterBasePortNumber] = portBaseNum = -kErrorModUiExecFailed;
                return false;
            }

            const String appDir(getAppDir());
            const String moduiStr(appDir + DISTRHO_OS_SEP_STR "mod-ui" APP_EXT);

            const char* const mod_ui_args[] = {
                moduiStr.buffer(),
                nullptr
            };

            startingModUI = true;
            if (mod_ui.start(mod_ui_args, envp))
            {
                d_stderr("MOD Desktop: mod-ui exec ok");
                return true;
            }

            d_stderr("MOD Desktop: Failed to start jackd");
            parameters[kParameterBasePortNumber] = portBaseNum = -kErrorModUiExecFailed;
            return false;
        }

        if (startingModUI)
        {
            d_stderr("MOD Desktop: Runner setup ok");
            startingModUI = false;
        }

        parameters[kParameterBasePortNumber] = portBaseNum;
        return true;
    }

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
        // return d_version(VERSION[0]-'0', VERSION[2]-'0', VERSION[4]-'0');
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
        switch (index)
        {
        case kParameterBasePortNumber:
            parameter.hints = kParameterIsOutput | kParameterIsInteger;
            parameter.name = "base port number";
            parameter.symbol = "base_port_num";
            parameter.ranges.min = -kErrorUndefined;
            parameter.ranges.max = 512.f;
            parameter.ranges.def = 0.f;
            break;
        }
    }

   /**
      Set a state key and default value.
      This function will be called once, shortly after the plugin is created.
    */
    void initState(uint32_t, State& state) override
    {
        state.hints = kStateIsFilenamePath | kStateIsOnlyForDSP;
        state.key = "pedalboard";
        state.defaultValue = "";
        state.label = "Pedalboard";
    }

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
    */
    float getParameterValue(const uint32_t index) const override
    {
        return parameters[index];
    }

   /**
      Change a parameter value.
    */
    void setParameterValue(const uint32_t index, const float value) override
    {
        // parameters[index] = value;
    }

   /**
      Get the value of an internal state.
      The host may call this function from any non-realtime context.
    */
    String getState(const char* const key) const override
    {
        if (std::strcmp(key, "pedalboard") == 0)
            return currentPedalboard;

        return String();
    }

   /**
      Change an internal state.
    */
    void setState(const char* const key, const char* const value) override
    {
        if (std::strcmp(key, "pedalboard") == 0)
            currentPedalboard = value;
    }

   /* --------------------------------------------------------------------------------------------------------
    * Process */

    void activate() override
    {
        if (shouldStartRunner)
        {
            shouldStartRunner = false;

            if (portBaseNum > 0 && run())
                startRunner(500);
        }

        // make sure we have enough space to cover everything
        const double sampleRate = getSampleRate();
        const uint32_t bufferSize = getBufferSize();
        const uint32_t bufferSizeInput = bufferSize * (sampleRate / 48000.0);
        const uint32_t bufferSizeOutput = bufferSize * (48000.0 / sampleRate);

        audioBufferIn.createBuffer(2, (bufferSizeInput + 8192) * 2);
        audioBufferOut.createBuffer(2, (bufferSizeOutput + 8192) * 2);

        numSamplesInTempBuffers = d_nextPowerOf2((std::max(bufferSizeInput, bufferSizeOutput) + 256) * 2);
        delete[] tempBuffers[0];
        delete[] tempBuffers[1];
        tempBuffers[0] = new float[numSamplesInTempBuffers];
        tempBuffers[1] = new float[numSamplesInTempBuffers];
        std::memset(tempBuffers[0], 0, sizeof(float) * numSamplesInTempBuffers);
        std::memset(tempBuffers[1], 0, sizeof(float) * numSamplesInTempBuffers);

        numSamplesUntilProcessing = d_isNotEqual(sampleRate, 48000.0)
                                  ? d_roundToUnsignedInt(128.0 * (sampleRate / 48000.0))
                                  : 128;

        setLatency(numSamplesUntilProcessing);

        midiFrameOffset = 0.0;
        midiRecvBuffer = new uint8_t[kMaxMidiSize];
        midiRingBuffer.createBuffer(kMaxMidiSize);
    }

    void deactivate() override
    {
        audioBufferIn.deleteBuffer();
        audioBufferOut.deleteBuffer();
        midiRingBuffer.deleteBuffer();

        delete[] tempBuffers[0];
        delete[] tempBuffers[1];
        delete[] midiRecvBuffer;
        tempBuffers[0] = tempBuffers[1] = nullptr;
        midiRecvBuffer = nullptr;
        numSamplesInTempBuffers = 0;
    }

   /**
      Run/process function for plugins without MIDI input.
    */
    void run(const float** const inputs, float** const outputs, const uint32_t frames,
             const MidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        if (! processing)
        {
            std::memset(outputs[0], 0, sizeof(float) * frames);
            std::memset(outputs[1], 0, sizeof(float) * frames);
            return;
        }

        const double resampledFrames = frames * resamplerRatio;

        if (resamplerTo48kHz != nullptr)
        {
            resamplerTo48kHz->inp_count = frames;
            resamplerTo48kHz->out_count = numSamplesInTempBuffers;
            resamplerTo48kHz->inp_data = inputs;
            resamplerTo48kHz->out_data = tempBuffers;
            resamplerTo48kHz->process();
            DISTRHO_SAFE_ASSERT(resamplerTo48kHz->inp_count == 0);

            audioBufferIn.write(tempBuffers, numSamplesInTempBuffers - resamplerTo48kHz->out_count);
        }
        else
        {
            audioBufferIn.write(inputs, frames);
        }

        for (uint32_t i = 0; i < midiEventCount; ++i)
        {
            const MidiEvent& midiEvent(midiEvents[i]);

            if (midiEvent.size >= kMaxMidiSize)
                continue;

            midiRingBuffer.writeUInt(static_cast<uint>(midiFrameOffset + midiEvent.frame * resamplerRatio)) &&
            midiRingBuffer.writeUInt(midiEvent.size) &&
            midiRingBuffer.writeCustomData(midiEvent.size > MidiEvent::kDataSize ? midiEvent.dataExt : midiEvent.data, midiEvent.size);
            midiRingBuffer.commitWrite();
        }

        midiFrameOffset += resampledFrames;
        uint midiFrameOffsetLocal = 0;
        uint lastMidiOutFrame = 0;

        while (audioBufferIn.getNumReadableSamples() >= 128)
        {
            float* shmbuffers[2] = { shm.data->audio, shm.data->audio + 128 };

            audioBufferIn.read(shmbuffers, 128);

            uint midiFrame, midiSize, shmMidiEventCount = 0;
            while (midiRingBuffer.isDataAvailableForReading())
            {
                midiFrame = midiRingBuffer.peekUInt();

                if (midiFrame < midiFrameOffsetLocal)
                    midiFrame = 0;
                else
                    midiFrame -= midiFrameOffsetLocal;

                if (midiFrame >= 128)
                    break;

                midiRingBuffer.readUInt();
                midiSize = midiRingBuffer.readUInt();
                if (midiSize < kMaxMidiSize && midiRingBuffer.readCustomData(midiRecvBuffer, midiSize))
                {
                    // TODO
                    if (midiSize > 4)
                        continue;

                    // TODO data pool
                    shm.data->midiFrames[shmMidiEventCount] = midiFrame;
                    for (uint32_t i = 0; i < midiSize; ++i)
                        shm.data->midiData[shmMidiEventCount * 4 + i] = midiRecvBuffer[i];
                    for (uint32_t i = midiSize; i < 4; ++i)
                        shm.data->midiData[shmMidiEventCount * 4 + i] = 0;

                    if (++shmMidiEventCount == 511)
                        break;
                }
                else
                {
                    d_stderr("midi ringbuffer data race, ignoring future events");
                    midiRingBuffer.flush();
                    break;
                }
            }

            shm.data->midiEventCount = shmMidiEventCount;

            if (! shm.process())
            {
                d_stderr("shm processing failed");
                processing = false;
                std::memset(outputs[0], 0, sizeof(float) * frames);
                std::memset(outputs[1], 0, sizeof(float) * frames);
                return;
            }

            if (resamplerFrom48kHz != nullptr)
            {
                resamplerFrom48kHz->inp_count = 128;
                resamplerFrom48kHz->out_count = numSamplesInTempBuffers;
                resamplerFrom48kHz->inp_data = shmbuffers;
                resamplerFrom48kHz->out_data = tempBuffers;
                resamplerFrom48kHz->process();
                DISTRHO_SAFE_ASSERT(resamplerFrom48kHz->inp_count == 0);

                audioBufferOut.write(tempBuffers, numSamplesInTempBuffers - resamplerFrom48kHz->out_count);
            }
            else
            {
                audioBufferOut.write(shmbuffers, 128);
            }

            // TODO ring buffer out
            for (uint16_t i = 0; i < shm.data->midiEventCount; ++i)
            {
                // so bad..
                midiFrame = std::min<uint>(frames - 1, std::max(0.0, shm.data->midiFrames[i] + midiFrameOffsetLocal - (midiFrameOffset - resampledFrames)) / resamplerRatio);
                lastMidiOutFrame = std::max(midiFrame, lastMidiOutFrame);

                // TODO data pool
                MidiEvent midiEvent = {
                    midiFrame,
                    4,
                    {
                        shm.data->midiData[i * 4 + 0],
                        shm.data->midiData[i * 4 + 1],
                        shm.data->midiData[i * 4 + 2],
                        shm.data->midiData[i * 4 + 3],
                    },
                    nullptr
                };

                if (! writeMidiEvent(midiEvent))
                    break;
            }

            midiFrameOffsetLocal += 128;
            midiFrameOffset = std::max(0.0, midiFrameOffset - 128);
        }

        if (numSamplesUntilProcessing >= frames)
        {
            numSamplesUntilProcessing -= frames;
            std::memset(outputs[0], 0, sizeof(float) * frames);
            std::memset(outputs[1], 0, sizeof(float) * frames);
            return;
        }

        if (numSamplesUntilProcessing != 0)
        {
            const uint32_t start = numSamplesUntilProcessing;
            const uint32_t remaining = frames - start;
            numSamplesUntilProcessing = 0;

            std::memset(outputs[0], 0, sizeof(float) * start);
            std::memset(outputs[1], 0, sizeof(float) * start);

            float* offsetbuffers[2] = {
                outputs[0] + start,
                outputs[1] + start,
            };
            audioBufferOut.read(offsetbuffers, remaining);
            return;
        }

        audioBufferOut.read(outputs, frames);

        for (uint32_t i = lastMidiOutFrame; i < frames; i += 32)
        {
            const MidiEvent midiEvent = {
                i, 1, { 0xFE, 0, 0, 0 }, nullptr
            };

            if (! writeMidiEvent(midiEvent))
                break;
        }
    }

    void sampleRateChanged(const double sampleRate) override
    {
        if (portBaseNum < 0 || shm.data == nullptr)
            return;

        stopRunner();

        if (processing && jackd.isRunning())
        {
            processing = false;
            shm.stopWait();
        }

        jackd.stop();
        shm.deinit();
        setupResampler(sampleRate);

        shouldStartRunner = true;
    }

    void setupResampler(const double sampleRate)
    {
        if (d_isNotEqual(sampleRate, 48000.0))
        {
            resamplerTo48kHz = new Resampler();
            resamplerTo48kHz->setup(sampleRate, 48000, 2, 32);
            resamplerFrom48kHz = new Resampler();
            resamplerFrom48kHz->setup(48000, sampleRate, 2, 32);
            resamplerRatio = sampleRate / 48000.0;
        }
        else
        {
            resamplerRatio = 1.0;
            resamplerTo48kHz = nullptr;
            resamplerFrom48kHz = nullptr;
        }
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
