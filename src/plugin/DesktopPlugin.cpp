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
    float* shmBuffers[2] = {};
    float* tmpBuffers[2] = {};
    uint32_t numFramesInShmBuffer = 0;
    uint32_t numFramesInTmpBuffer = 0;

public:
    DesktopPlugin()
        : Plugin(0, 0, 0)
    {
        if (isDummyInstance())
            return;

        const String sampleRateStr(static_cast<int>(getSampleRate()));
        const char* const jackd_args[] = {
            P "/jackd", "-R", "-S", "-n", "mod-desktop", "-C", P "/jack/jack-session.conf", "-d", "desktop", "-r", sampleRateStr.buffer(), nullptr
        };
        const char* const mod_ui_args[] = {
            P "/mod-ui", nullptr
        };

        if (shm.init() && jackd.start(jackd_args) && mod_ui.start(mod_ui_args))
        {
            processing = true;
            shm.getAudioData(shmBuffers);
            bufferSizeChanged(getBufferSize());
        }
    }

    ~DesktopPlugin()
    {
        mod_ui.stop();
        jackd.stop();
        shm.deinit();

        delete[] tmpBuffers[0];
        delete[] tmpBuffers[1];
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

    void activate()
    {
        shm.reset();

        firstTimeProcessing = true;
        numFramesInShmBuffer = numFramesInTmpBuffer = 0;
    }

    void deactivate()
    {
    }

   /**
      Run/process function for plugins without MIDI input.
    */
    void run(const float** const inputs, float** const outputs, const uint32_t frames) override
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
            shmBuffers[0][ti] = inputs[0][i];
            shmBuffers[1][ti] = inputs[1][i];

            if (++ti == 128)
            {
                ti = 0;

                if (! shm.process(tmpBuffers, to))
                {
                    d_stdout("shm processing failed");
                    processing = false;
                    std::memset(outputs[0], 0, sizeof(float) * frames);
                    std::memset(outputs[1], 0, sizeof(float) * frames);
                    return;
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
        }
        else if (framesDone != frames)
        {
            const uint32_t framesToCopy = frames - framesDone;
            std::memcpy(outputs[0] + framesDone, tmpBuffers[0], sizeof(float) * framesToCopy);
            std::memcpy(outputs[1] + framesDone, tmpBuffers[1], sizeof(float) * framesToCopy);

            to -= framesToCopy;
            std::memmove(tmpBuffers[0], tmpBuffers[0] + framesToCopy, sizeof(float) * to);
            std::memmove(tmpBuffers[1], tmpBuffers[1] + framesToCopy, sizeof(float) * to);
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
            P "/jackd", "-R", "-S", "-n", "mod-desktop", "-C", P "/jack/jack-session.conf", "-d", "desktop", "-r", sampleRateStr.buffer(), nullptr
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
