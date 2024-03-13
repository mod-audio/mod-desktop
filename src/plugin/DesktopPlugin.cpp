// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "DistrhoPlugin.hpp"

#include "Time.hpp"

#include "ChildProcess.hpp"
#include "SharedMemory.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class DesktopPlugin : public Plugin
{
    ChildProcess jackd;
    ChildProcess mod_ui;
    SharedMemory shm;
    bool processing = false;
    bool firstTimeProcessing = true;
    float parameters[24] = {};
    float* tmpBuffers[2] = {};
    uint32_t numFramesInShmBuffer = 0;
    uint32_t numFramesInTmpBuffer = 0;

   #ifdef DISTRHO_OS_WINDOWS
    const CHAR* envp;
   #else
    char** envp;
   #endif

public:
    DesktopPlugin()
        : Plugin(0, 0, 0)
    {
        if (isDummyInstance())
            return;

      #ifdef DISTRHO_OS_WINDOWS
        envp = (
            "MOD_DATA_DIR=" P "\\data\0"
            "MOD_FACTORY_PEDALBOARDS_DIR=" P "\\pedalboards\0"
            "LV2_PATH=" P "\\plugins\0"
            "\0"
        );
      #else
        uint envsize = 0;
        while (environ[envsize] != nullptr)
            ++envsize;

        envp = new char*[envsize + 8];

        for (uint i = 0; i < envsize; ++i)
            envp[i] = strdup(environ[i]);

        envp[envsize] = strdup("MOD_DESKTOP=1");
        envp[envsize + 1] = strdup("LANG=en_US.UTF-8");
       #ifdef DISTRHO_OS_MAC
        envp[envsize + 2] = strdup("DYLD_LIBRARY_PATH=" P "/MacOS");
        envp[envsize + 3] = strdup("JACK_DRIVER_DIR=" P "/MacOS/jack");
        envp[envsize + 4] = strdup("MOD_DATA_DIR=/Users/falktx/Documents/MOD Desktop");
        envp[envsize + 5] = strdup("MOD_FACTORY_PEDALBOARDS_DIR=" P "/Resources/pedalboards");
        envp[envsize + 6] = strdup("LV2_PATH=" P "/LV2");
        envp[envsize + 7] = nullptr;
       #else
        envp[envsize + 2] = strdup("LD_LIBRARY_PATH=" P);
        envp[envsize + 3] = strdup("JACK_DRIVER_DIR=" P "/jack");
        envp[envsize + 4] = strdup("MOD_DATA_DIR=" P "/data");
        envp[envsize + 5] = strdup("MOD_FACTORY_PEDALBOARDS_DIR=" P "/pedalboards");
        envp[envsize + 6] = strdup("LV2_PATH=" P "/plugins");
        envp[envsize + 7] = nullptr;
       #endif
      #endif

        const String sampleRateStr(static_cast<int>(getSampleRate()));
        const char* const jackd_args[] = {
           #if defined(DISTRHO_OS_MAC)
            P "/MacOS/jackd",
           #elif defined(DISTRHO_OS_WINDOWS)
            P "\\jackd.exe",
           #else
            P "/jackd",
           #endif
            "-R",
            "-S",
            "-n", "mod-desktop", "-C",
           #if defined(DISTRHO_OS_MAC)
            P "/MacOS/jack/jack-session.conf",
           #elif defined(DISTRHO_OS_WINDOWS)
            P "\\jack\\jack-session.conf",
           #else
            P "/jack/jack-session.conf",
           #endif
            "-d", "desktop", "-r", sampleRateStr.buffer(), nullptr
        };
        const char* const mod_ui_args[] = {
           #if defined(DISTRHO_OS_MAC)
            P "/MacOS/mod-ui",
           #elif defined(DISTRHO_OS_WINDOWS)
            P "\\mod-ui.exe",
           #else
            P "/mod-ui",
           #endif
            nullptr
        };

        if (shm.init() && jackd.start(jackd_args, envp))
        {
            processing = true;
            bufferSizeChanged(getBufferSize());

            d_msleep(500);
            mod_ui.start(mod_ui_args, envp);
        }
    }

    ~DesktopPlugin()
    {
        if (processing && jackd.isRunning())
            shm.stopWait();

        jackd.stop();
        mod_ui.stop();

        shm.deinit();

        delete[] tmpBuffers[0];
        delete[] tmpBuffers[1];

       #ifdef DISTRHO_OS_WINDOWS
       #else
        for (uint i = 0; envp[i] != nullptr; ++i)
            std::free(envp[i]);

        delete[] envp;
       #endif
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
        return parameters[index];
    }

   /**
      Change a parameter value.
    */
    void setParameterValue(const uint32_t index, const float value) override
    {
        parameters[index] = value;
    }

   /**
      Change an internal state.
    */
    void setState(const char*, const char*) override
    {
    }

   /* --------------------------------------------------------------------------------------------------------
    * Process */

    void activate() override
    {
        shm.reset();

        firstTimeProcessing = true;
        numFramesInShmBuffer = numFramesInTmpBuffer = 0;
    }

    void deactivate() override
    {
    }

   /**
      Run/process function for plugins without MIDI input.
    */
    void run(const float** const inputs, float** const outputs, const uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        if (! processing)
        {
            std::memset(outputs[0], 0, sizeof(float) * frames);
            std::memset(outputs[1], 0, sizeof(float) * frames);
            return;
        }

        uint32_t ti = numFramesInShmBuffer;
        uint32_t to = numFramesInTmpBuffer;
        uint32_t framesDone = 0;

        for (uint32_t i = 0; i < frames; ++i)
        {
            shm.data->audio[ti] = inputs[0][i];
            shm.data->audio[128 + ti] = inputs[1][i];

            if (++ti == 128)
            {
                ti = 0;

                if (midiEventCount != 0)
                {
                    uint16_t mec = shm.data->midiEventCount;

                    while (midiEventCount != 0 && mec != 511)
                    {
                        if (midiEvents->size > 4)
                        {
                            --midiEventCount;
                            ++midiEvents;
                            continue;
                        }

                        if (midiEvents->frame >= framesDone + 128)
                            break;

                        shm.data->midiFrames[mec] = midiEvents->frame - framesDone;
                        shm.data->midiData[mec * 4 + 0] = midiEvents->data[0];
                        shm.data->midiData[mec * 4 + 1] = midiEvents->data[1];
                        shm.data->midiData[mec * 4 + 2] = midiEvents->data[2];
                        shm.data->midiData[mec * 4 + 3] = midiEvents->data[3];

                        --midiEventCount;
                        ++midiEvents;
                        ++mec;
                    }

                    shm.data->midiEventCount = mec;
                }

                if (! shm.process(tmpBuffers, to))
                {
                    d_stdout("shm processing failed");
                    processing = false;
                    std::memset(outputs[0], 0, sizeof(float) * frames);
                    std::memset(outputs[1], 0, sizeof(float) * frames);
                    return;
                }

                for (uint16_t j = 0; j < shm.data->midiEventCount; ++j)
                {
                    MidiEvent midiEvent = {
                        framesDone + shm.data->midiFrames[j] - to,
                        4,
                        {
                            shm.data->midiData[j * 4 + 0],
                            shm.data->midiData[j * 4 + 1],
                            shm.data->midiData[j * 4 + 2],
                            shm.data->midiData[j * 4 + 3],
                        },
                        nullptr
                    };

                    if (! writeMidiEvent(midiEvent))
                        break;
                }

                to += 128;

                if (firstTimeProcessing)
                {
                    firstTimeProcessing = false;
                    std::memset(outputs[0], 0, sizeof(float) * i);
                    std::memset(outputs[1], 0, sizeof(float) * i);
                }
                else
                {
                    const uint32_t framesToCopy = std::min(128u, frames - framesDone);
                    std::memcpy(outputs[0] + framesDone, tmpBuffers[0], sizeof(float) * framesToCopy);
                    std::memcpy(outputs[1] + framesDone, tmpBuffers[1], sizeof(float) * framesToCopy);

                    to -= framesToCopy;
                    framesDone += framesToCopy;
                    std::memmove(tmpBuffers[0], tmpBuffers[0] + framesToCopy, sizeof(float) * to);
                    std::memmove(tmpBuffers[1], tmpBuffers[1] + framesToCopy, sizeof(float) * to);
                }
            }
        }

        if (firstTimeProcessing)
        {
            std::memset(outputs[0], 0, sizeof(float) * frames);
            std::memset(outputs[1], 0, sizeof(float) * frames);

            if (midiEventCount != 0)
            {
                uint16_t mec = shm.data->midiEventCount;

                while (midiEventCount != 0 && mec != 511)
                {
                    if (midiEvents->size > 4)
                    {
                        --midiEventCount;
                        ++midiEvents;
                        continue;
                    }

                    shm.data->midiFrames[mec] = midiEvents->frame;
                    shm.data->midiData[mec * 4 + 0] = midiEvents->data[0];
                    shm.data->midiData[mec * 4 + 1] = midiEvents->data[1];
                    shm.data->midiData[mec * 4 + 2] = midiEvents->data[2];
                    shm.data->midiData[mec * 4 + 3] = midiEvents->data[3];

                    --midiEventCount;
                    ++midiEvents;
                    ++mec;
                }

                shm.data->midiEventCount = mec;
            }
        }
        else if (framesDone != frames)
        {
            const uint32_t framesToCopy = frames - framesDone;
            std::memcpy(outputs[0] + framesDone, tmpBuffers[0], sizeof(float) * framesToCopy);
            std::memcpy(outputs[1] + framesDone, tmpBuffers[1], sizeof(float) * framesToCopy);

            to -= framesToCopy;
            std::memmove(tmpBuffers[0], tmpBuffers[0] + framesToCopy, sizeof(float) * to);
            std::memmove(tmpBuffers[1], tmpBuffers[1] + framesToCopy, sizeof(float) * to);

            if (midiEventCount != 0)
            {
                uint16_t mec = shm.data->midiEventCount;

                while (midiEventCount != 0 && mec != 511)
                {
                    if (midiEvents->size > 4)
                    {
                        --midiEventCount;
                        ++midiEvents;
                        continue;
                    }

                    shm.data->midiFrames[mec] = ti + framesDone - midiEvents->frame;
                    shm.data->midiData[mec * 4 + 0] = midiEvents->data[0];
                    shm.data->midiData[mec * 4 + 1] = midiEvents->data[1];
                    shm.data->midiData[mec * 4 + 2] = midiEvents->data[2];
                    shm.data->midiData[mec * 4 + 3] = midiEvents->data[3];

                    --midiEventCount;
                    ++midiEvents;
                    ++mec;
                }

                shm.data->midiEventCount = mec;
            }
        }

        numFramesInShmBuffer = ti;
        numFramesInTmpBuffer = to;
    }

    void bufferSizeChanged(const uint32_t bufferSize) override
    {
        delete[] tmpBuffers[0];
        delete[] tmpBuffers[1];
        tmpBuffers[0] = new float[bufferSize + 256];
        tmpBuffers[1] = new float[bufferSize + 256];
        std::memset(tmpBuffers[0], 0, sizeof(float) * (bufferSize + 256));
        std::memset(tmpBuffers[1], 0, sizeof(float) * (bufferSize + 256));
    }

    void sampleRateChanged(const double sampleRate) override
    {
        if (! processing)
            return;

        const String sampleRateStr(static_cast<int>(sampleRate));
        const char* const jackd_args[] = {
           #if defined(DISTRHO_OS_MAC)
            P "/MacOS/jackd",
           #elif defined(DISTRHO_OS_WINDOWS)
            P "\\jackd.exe",
           #else
            P "/jackd",
           #endif
            "-R",
            "-S",
            "-n", "mod-desktop", "-C",
           #if defined(DISTRHO_OS_MAC)
            P "/MacOS/jack/jack-session.conf",
           #elif defined(DISTRHO_OS_WINDOWS)
            P "\\jack\\jack-session.conf",
           #else
            P "/jack/jack-session.conf",
           #endif
            "-d", "desktop", "-r", sampleRateStr.buffer(), nullptr
        };

        jackd.stop();
        jackd.start(jackd_args);
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
