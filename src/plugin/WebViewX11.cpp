// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "WebView.hpp"

#include "ChildProcess.hpp"
#include "extra/String.hpp"

#include <dlfcn.h>
#include <linux/limits.h>
#include <X11/Xlib.h>
// #include <X11/Xutil.h>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

struct WebViewIPC {
    ChildProcess p;
    ::Display* display;
    ::Window childWindow;
    ::Window ourWindow;
};

// -----------------------------------------------------------------------------------------------------------

void* addWebView(uintptr_t viewptr)
{
    ::Display* const display = XOpenDisplay(nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(display != nullptr, nullptr);

    WebViewIPC* const ipc = new WebViewIPC();
    ipc->display = display;
    ipc->childWindow = 0;
    ipc->ourWindow = viewptr;

    char webviewTool[PATH_MAX] = {};

    {
        Dl_info info = {};
        dladdr((void*)addWebView, &info);

        if (info.dli_fname[0] == '.')
        {
            getcwd(webviewTool, PATH_MAX - 1);
            std::strncat(webviewTool, info.dli_fname + 1, PATH_MAX - 1);
        }
        else if (info.dli_fname[0] != '/')
        {
            getcwd(webviewTool, PATH_MAX - 1);
            std::strncat(webviewTool, "/", PATH_MAX - 1);
            std::strncat(webviewTool, info.dli_fname, PATH_MAX - 1);
        }
        else
        {
            std::strncpy(webviewTool, info.dli_fname, PATH_MAX - 1);
        }
    }

    if (char* const c = std::strrchr(webviewTool, '/'))
        *c = 0;

    std::strncat(webviewTool, "/MOD-Desktop-WebView", PATH_MAX - 1);

    const String viewStr(viewptr);
    const char* const args[] = { webviewTool, "-platform", "xcb", "-xembed", viewStr.buffer(), nullptr };
    ipc->p.start2(args);

    return ipc;
}

void destroyWebView(void* const webviewptr)
{
    WebViewIPC* const ipc = static_cast<WebViewIPC*>(webviewptr);

    XCloseDisplay(ipc->display);
    delete ipc;
}

void reloadWebView(void* const webviewptr)
{
}

void resizeWebView(void* const webviewptr, const uint offset, const uint width, const uint height)
{
    WebViewIPC* const ipc = static_cast<WebViewIPC*>(webviewptr);

    if (ipc->childWindow == 0)
    {
        ::Window rootWindow, parentWindow;
        ::Window* childWindows = nullptr;
        uint numChildren = 0;

        XFlush(ipc->display);
        XQueryTree(ipc->display, ipc->ourWindow, &rootWindow, &parentWindow, &childWindows, &numChildren);

        if (numChildren == 0 || childWindows == nullptr)
            return;

        ipc->childWindow = childWindows[0];
        XFree(childWindows);
    }

    XMoveResizeWindow(ipc->display, ipc->childWindow, 0, offset, width, height);
    XFlush(ipc->display);
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
