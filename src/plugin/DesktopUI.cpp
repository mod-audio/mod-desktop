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
    String error;
    String errorDetail;
    int port = 0;
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

        buttonOpenUserFilesDir.setId(2);
        buttonOpenUserFilesDir.setLabel("Open User Files Dir");
        buttonOpenUserFilesDir.setFontScale(scaleFactor);
        buttonOpenUserFilesDir.setAbsolutePos(74 * scaleFactor, 2 * scaleFactor);
        buttonOpenUserFilesDir.setSize(140 * scaleFactor, 26 * scaleFactor);

        buttonOpenWebGui.setId(3);
        buttonOpenWebGui.setLabel("Open in Web Browser");
        buttonOpenWebGui.setFontScale(scaleFactor);
        buttonOpenWebGui.setAbsolutePos(216 * scaleFactor, 2 * scaleFactor);
        buttonOpenWebGui.setSize(150 * scaleFactor, 26 * scaleFactor);
        buttonOpenWebGui.hide();

        label = "MOD Desktop ";
        label += getPluginFormatName();
        label += " v" VERSION;

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
        if (webview != nullptr)
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
            if (d_isZero(value))
                return;

            if (value < 0.f)
            {
                const int nport = d_roundToIntNegative(value);
                DISTRHO_SAFE_ASSERT_RETURN(nport != 0,);

                if (nport == port)
                    return;

                port = nport;

                if (webview != nullptr)
                {
                    destroyWebView(webview);
                    webview = nullptr;
                    buttonOpenWebGui.hide();
                }

                switch (-nport)
                {
                case kErrorAppDirNotFound:
                    error = "Error: MOD Desktop application directory not found";
                    errorDetail = "Make sure to install the standalone and run it at least once";
                    break;
                case kErrorJackdExecFailed:
                    error = "Error: Failed to start jackd";
                    errorDetail = "";
                    break;
                case kErrorModUiExecFailed:
                    error = "Error: Failed to start mod-ui";
                    errorDetail = "";
                    break;
                case kErrorShmSetupFailed:
                    error = "Error initializing MOD Desktop plugin";
                    errorDetail = "Shared memory setup failed";
                    break;
                case kErrorUndefined:
                    error = "Error initializing MOD Desktop plugin";
                    errorDetail = "";
                    break;
                }

                repaint();
                return;
            }

            const int nport = d_roundToUnsignedInt(value);
            DISTRHO_SAFE_ASSERT_RETURN(nport != 0,);

            if (nport == port)
                return;

            if (error.isNotEmpty())
            {
                error.clear();
                errorDetail.clear();
                repaint();
            }

            if (webview != nullptr)
            {
                destroyWebView(webview);
                webview = nullptr;

                buttonOpenWebGui.hide();
                repaint();
            }

            port = nport;
            d_stderr("webview port is %d", kPortNumOffset + port * 3 + 2);
            webview = addWebView(getWindow().getNativeWindowHandle(), getScaleFactor(), kPortNumOffset + port * 3 + 2);

            buttonOpenWebGui.show();
            repaint();
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
        fontSize(18 * scaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        text(getWidth() / 2, kVerticalOffset * scaleFactor / 2, label, nullptr);

        if (error.isNotEmpty())
        {
            fontSize(36 * scaleFactor);
            text(getWidth() / 2, getHeight() / 2 - 18 * scaleFactor, error, nullptr);

            if (errorDetail.isNotEmpty())
            {
                fontSize(18 * scaleFactor);
                text(getWidth() / 2, getHeight() / 2 + 18 * scaleFactor, errorDetail, nullptr);
            }
        }
    }

    void buttonClicked(SubWidget* const widget, int) override
    {
        switch (widget->getId())
        {
        case 1:
            if (webview != nullptr)
                reloadWebView(webview);
            break;
        case 2:
            openUserFilesDir();
            break;
        case 3:
            openWebGui(kPortNumOffset + port * 3 + 2);
            break;
        }
    }

    void onResize(const ResizeEvent& ev) override
    {
        UI::onResize(ev);

        if (webview == nullptr)
            return;

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
