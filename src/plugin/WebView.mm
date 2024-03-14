// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "WebView.hpp"

#include "DistrhoPluginInfo.h"

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

struct WebViewImpl {
    NSView* const view;
    WKWebView* const webview;
    NSURLRequest* const urlreq;
};

// -----------------------------------------------------------------------------------------------------------

void* addWebView(const uintptr_t parentWinId, const uint port)
{
    NSView* const view = reinterpret_cast<NSView*>(parentWinId);

    const CGRect rect = CGRectMake(0,
                                   kVerticalOffset,
                                   DISTRHO_UI_DEFAULT_WIDTH,
                                   DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset);

    WKWebView* const webview = [[WKWebView alloc] initWithFrame: rect];
    [[[webview configuration] preferences] setValue: @(true) forKey: @"developerExtrasEnabled"];
    [view addSubview:webview];

    char url[32];
    std::snprintf(url, 31, "http://127.0.0.1:%u/", port);
    NSString* const nsurl = [[NSString alloc]
        initWithBytes:url
               length:std::strlen(url)
             encoding:NSUTF8StringEncoding];
    NSURLRequest* const urlreq = [[NSURLRequest alloc] initWithURL: [NSURL URLWithString: nsurl]];
    [nsurl release];

    [webview loadRequest: urlreq];
    [webview setHidden:NO];

    return new WebViewImpl{view, webview, urlreq};
}

void destroyWebView(void* const webview)
{
    WebViewImpl* const impl = static_cast<WebViewImpl*>(webview);

    [impl->webview setHidden:YES];
    [impl->webview removeFromSuperview];
    [impl->urlreq release];

    delete impl;
}

void reloadWebView(void* const webview)
{
    WebViewImpl* const impl = static_cast<WebViewImpl*>(webview);

    [impl->webview loadRequest: impl->urlreq];
}

void resizeWebView(void* const webview, const uint offset, const uint width, const uint height)
{
    WebViewImpl* const impl = static_cast<WebViewImpl*>(webview);

    [impl->webview setFrame:CGRectMake(0, offset, width, height)];
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
