// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "WebView.hpp"
#include "DistrhoPluginInfo.h"

#ifdef USING_CHOC
#include "choc/gui/choc_WebView.h"
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

void* addWebView(const uintptr_t parentWinId, const double scaleFactor, const uint port)
{
#ifdef USING_CHOC
    std::unique_ptr<choc::ui::WebView> webview = std::make_unique<choc::ui::WebView>(choc::ui::WebView());
    DISTRHO_SAFE_ASSERT_RETURN(webview->loadedOK(), nullptr);

    const HWND handle = static_cast<HWND>(webview->getViewHandle());
    DISTRHO_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);

    char url[32] = {};
    std::snprintf(url, 31, "http://127.0.0.1:%u/", port);
    webview->navigate(url);

    auto flags = GetWindowLongPtr(handle, -16);
    flags = (flags & ~(decltype (flags)) WS_POPUP) | (decltype (flags)) WS_CHILD;
    SetWindowLongPtr(handle, -16, flags);

    SetParent(handle, reinterpret_cast<HWND>(parentWinId));
    SetWindowPos(handle, nullptr,
                 0, kVerticalOffset * scaleFactor,
                 DISTRHO_UI_DEFAULT_WIDTH * scaleFactor,
                 (DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset) * scaleFactor,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ShowWindow(handle, SW_SHOW);

    return webview.release();
#else
    return nullptr;
#endif
}

void destroyWebView(void* const webviewptr)
{
#ifdef USING_CHOC
    delete static_cast<choc::ui::WebView*>(webviewptr);
#endif
}

void reloadWebView(void* const webviewptr, const uint port)
{
#ifdef USING_CHOC
    choc::ui::WebView* const webview = static_cast<choc::ui::WebView*>(webviewptr);

    char url[32] = {};
    std::snprintf(url, 31, "http://127.0.0.1:%u/", port);
    webview->navigate(url);
#endif
}

void resizeWebView(void* const webviewptr, const uint offset, const uint width, const uint height)
{
#ifdef USING_CHOC
    choc::ui::WebView* const webview = static_cast<choc::ui::WebView*>(webviewptr);

    const HWND handle = static_cast<HWND>(webview->getViewHandle());
    SetWindowPos(handle, nullptr, 0, offset, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
#endif
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
