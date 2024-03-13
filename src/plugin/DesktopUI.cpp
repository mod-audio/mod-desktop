// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "DistrhoUI.hpp"
#include "DistrhoPluginUtils.hpp"
#include "NanoButton.hpp"
#include "WebView.hpp"

#include "utils.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class DesktopUI : public UI,
                  public ButtonEventHandler::Callback
{
    Button buttonRefresh;
    Button buttonOpenWebGui;
    Button buttonOpenUserFilesDir;
    String label;
    void* webview = nullptr;

public:
    DesktopUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT),
          buttonRefresh(this, this),
          buttonOpenWebGui(this, this),
          buttonOpenUserFilesDir(this, this)
    {
        loadSharedResources();

        const double scaleFactor = getScaleFactor();

        buttonRefresh.setId(1);
        buttonRefresh.setLabel("Refresh");
        buttonRefresh.setFontScale(scaleFactor);
        buttonRefresh.setAbsolutePos(2 * scaleFactor, 2 * scaleFactor);
        buttonRefresh.setSize(70 * scaleFactor, 26 * scaleFactor);

        buttonOpenWebGui.setId(2);
        buttonOpenWebGui.setLabel("Open in Web Browser");
        buttonOpenWebGui.setFontScale(scaleFactor);
        buttonOpenWebGui.setAbsolutePos(74 * scaleFactor, 2 * scaleFactor);
        buttonOpenWebGui.setSize(150 * scaleFactor, 26 * scaleFactor);

        buttonOpenUserFilesDir.setId(3);
        buttonOpenUserFilesDir.setLabel("Open User Files Dir");
        buttonOpenUserFilesDir.setFontScale(scaleFactor);
        buttonOpenUserFilesDir.setAbsolutePos(226 * scaleFactor, 2 * scaleFactor);
        buttonOpenUserFilesDir.setSize(140 * scaleFactor, 26 * scaleFactor);

        label = "MOD Desktop ";
        label += getPluginFormatName();
        label += " v" VERSION;

        webview = addWebView(getWindow().getNativeWindowHandle());

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
        destroyWebView(webview);
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(const uint32_t index, const float value) override
    {
        if (index == kParameterBasePortNumber)
        {
            // TODO set port
            reloadWebView(webview);
        }
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
    void onNanoDisplay() override
    {
        const double scaleFactor = getScaleFactor();

        fillColor(255, 255, 255, 255);
        fontSize(14 * scaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        text(getWidth() / 2, kVerticalOffset * scaleFactor / 2, label, nullptr);
    }

    void buttonClicked(SubWidget* const widget, int button) override
    {
        switch (widget->getId())
        {
        case 1:
            reloadWebView(webview);
            break;
        case 2:
            openWebGui();
            break;
        case 3:
            openUserFilesDir();
            break;
        }
    }

    void onResize(const ResizeEvent& ev) override
    {
        UI::onResize(ev);

        const double scaleFactor = getScaleFactor();

        uint offset = kVerticalOffset * scaleFactor;
        uint width = ev.size.getWidth();
        uint height = ev.size.getHeight() - offset;

       #ifdef DISTRHO_OS_MAC
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
