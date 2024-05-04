// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "WebView.hpp"
#include "DistrhoPluginInfo.h"

#define WC_ERR_INVALID_CHARS 0
#include "gui/choc_WebView.h"

#ifdef __APPLE__
# import <Cocoa/Cocoa.h>
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

void* addWebView(const uintptr_t parentWinId, const double scaleFactor, const uint port)
{
    std::unique_ptr<choc::ui::WebView> webview = std::make_unique<choc::ui::WebView>();
    DISTRHO_SAFE_ASSERT_RETURN(webview->loadedOK(), nullptr);

    void* const handle = webview->getViewHandle();
    DISTRHO_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);

    char url[32] = {};
    std::snprintf(url, 31, "http://127.0.0.1:%u/", port);
    webview->navigate(url);

   #if defined(__APPLE__)
    NSView* const view = static_cast<NSView*>(handle);

    [reinterpret_cast<NSView*>(parentWinId) addSubview:view];
    [view setFrame:NSMakeRect(0,
                              kVerticalOffset,
                              DISTRHO_UI_DEFAULT_WIDTH,
                              DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset)];
   #elif defined(_WIN32)
    const HWND hwnd = static_cast<HWND>(handle);

    LONG_PTR flags = GetWindowLongPtr(hwnd, -16);
    flags = (flags & ~WS_POPUP) | WS_CHILD;
    SetWindowLongPtr(hwnd, -16, flags);

    SetParent(hwnd, reinterpret_cast<HWND>(parentWinId));
    SetWindowPos(hwnd, nullptr,
                 0,
                 kVerticalOffset * scaleFactor,
                 DISTRHO_UI_DEFAULT_WIDTH * scaleFactor,
                 (DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset) * scaleFactor,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ShowWindow(hwnd, SW_SHOW);
   #endif

    return webview.release();
}

void destroyWebView(void* const webviewptr)
{
    delete static_cast<choc::ui::WebView*>(webviewptr);
}

void reloadWebView(void* const webviewptr, const uint port)
{
    choc::ui::WebView* const webview = static_cast<choc::ui::WebView*>(webviewptr);

    char url[32] = {};
    std::snprintf(url, 31, "http://127.0.0.1:%u/", port);
    webview->navigate(url);
}

void resizeWebView(void* const webviewptr, const uint offset, const uint width, const uint height)
{
    choc::ui::WebView* const webview = static_cast<choc::ui::WebView*>(webviewptr);

   #if defined(__APPLE__)
    NSView* const view = static_cast<NSView*>(webview->getViewHandle());
    [view setFrame:NSMakeRect(0, offset, width, height)];
   #elif defined(_WIN32)
    const HWND hwnd = static_cast<HWND>(webview->getViewHandle());
    SetWindowPos(hwnd, nullptr, 0, offset, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
   #endif
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
