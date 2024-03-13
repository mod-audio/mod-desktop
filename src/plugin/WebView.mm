// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "WebView.hpp"

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

void* addWebView(const uintptr_t viewptr)
{
    NSView* const view = reinterpret_cast<NSView*>(viewptr);

    cosnt CGRect rect = CGRectMake(0,
                                   kVerticalOffset,
                                   DISTRHO_UI_DEFAULT_WIDTH,
                                   DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset);

    WKWebView* const webview = [[[WKWebView alloc] initWithFrame: rect] retain];

    [[[webview configuration] preferences] setValue: @(true) forKey: @"developerExtrasEnabled"];

    [view addSubview:webview];
    [webview setHidden:NO];

    return webview;
}

void destroyWebView(void* const webviewptr)
{
    WKWebView* const webview = static_cast<WKWebView*>(webviewptr);

    [webview setHidden:YES];
    [[webview dealloc] release];
}

void reloadWebView(void* const webviewptr)
{
    WKWebView* const webview = static_cast<WKWebView*>(webviewptr);

    [webview loadRequest: [NSURLRequest requestWithURL: [NSURL URLWithString:@"http://127.0.0.1:18181/"]]];
}

void resizeWebView(void* const webviewptr, const uint offset, const uint width, const uint height)
{
    WKWebView* const webview = static_cast<WKWebView*>(webviewptr);

    [webview setFrame:CGRectMake(0, offset, width, height)];
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
