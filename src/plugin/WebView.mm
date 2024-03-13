// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "DistrhoUtils.hpp"

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

void* addWebView(void* view);
void reloadWebView(void* webview);
void resizeWebView(void* webview, uint offset, uint width, uint height);

void* addWebView(void* const viewptr)
{
    NSView* const view = static_cast<NSView*>(viewptr);
    WKWebView* const webview = [[WKWebView alloc] initWithFrame: CGRectMake(0, 25, 225, 300)];

    [[[webview configuration] preferences] setValue: @(true) forKey: @"developerExtrasEnabled"];

    [view addSubview:webview];
    [webview setHidden:NO];

    return webview;
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
