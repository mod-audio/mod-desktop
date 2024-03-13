// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "DistrhoUI.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

void* addWebView(void* view);
void reloadWebView(void* webview);
void resizeWebView(void* webview, uint offset, uint width, uint height);

// -----------------------------------------------------------------------------------------------------------

class DesktopUI : public UI
{
    void* webview = nullptr;

public:
    DesktopUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        webview = addWebView(reinterpret_cast<void*>(getWindow().getNativeWindowHandle()));

        const double scaleFactor = getScaleFactor();

        if (d_isNotEqual(scaleFactor, 1.0))
        {
            setGeometryConstraints((DISTRHO_UI_DEFAULT_WIDTH - 100) * scaleFactor,
                                   DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor);
            setSize(DISTRHO_UI_DEFAULT_WIDTH * scaleFactor, DISTRHO_UI_DEFAULT_HEIGHT * scaleFactor);
        }
        else
        {
            setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH - 100, DISTRHO_UI_DEFAULT_HEIGHT);
        }
    }

    ~DesktopUI() override
    {
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t, float) override
    {
    }

   /**
      A state has changed on the plugin side.
      This is called by the host to inform the UI about state changes.
    */
    void stateChanged(const char*, const char*) override
    {
        // nothing here
    }

   /* --------------------------------------------------------------------------------------------------------
    * Widget Callbacks */

   /**
      The drawing function.
    */
    void onDisplay() override
    {
    }

    bool onMouse(const MouseEvent& ev) override
    {
        if (UI::onMouse(ev))
            return true;

        reloadWebView(webview);
        return true;
    }

    void onResize(const ResizeEvent& ev) override
    {
        UI::onResize(ev);

        uint offset = 20;
        uint width = ev.size.getWidth();
        uint height = ev.size.getHeight() - offset;

       #ifdef DISTRHO_OS_MAC
        const double scaleFactor = getScaleFactor();
        offset /= scaleFactor;
        width /= scaleFactor;
        height /= scaleFactor;
       #endif

        resizeWebView(webview, offset, width, height);
    }

    // -------------------------------------------------------------------------------------------------------

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesktopUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new DesktopUI();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
