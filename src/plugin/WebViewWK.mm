// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "WebView.hpp"

#include "DistrhoPluginInfo.h"

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

// -----------------------------------------------------------------------------------------------------------

#define MACRO_NAME2(a, b, c) a ## b ## c
#define MACRO_NAME(a, b, c) MACRO_NAME2(a, b, c)

#define WEBVIEW_DELEGATE_CLASS_NAME \
    MACRO_NAME(WebViewDelegate_, _, DISTRHO_NAMESPACE)

@interface WEBVIEW_DELEGATE_CLASS_NAME : NSObject<WKUIDelegate>
@end

@implementation WEBVIEW_DELEGATE_CLASS_NAME

- (void)webView:(WKWebView*)webview
    runJavaScriptAlertPanelWithMessage:(NSString*)message
                      initiatedByFrame:(WKFrameInfo*)frame
                     completionHandler:(void (^)(void))completionHandler
{
	NSAlert* const alert = [[NSAlert alloc] init];
	[alert addButtonWithTitle:@"OK"];
    [alert setInformativeText:message];
	[alert setMessageText:@"Alert"];

    dispatch_async(dispatch_get_main_queue(), ^
    {
        [alert beginSheetModalForWindow:[webview window]
                      completionHandler:^(NSModalResponse)
        {
            completionHandler();
            [alert release];
        }];
    });
}

- (void)webView:(WKWebView*)webview
    runJavaScriptConfirmPanelWithMessage:(NSString*)message
                        initiatedByFrame:(WKFrameInfo*)frame
                       completionHandler:(void (^)(BOOL))completionHandler
{
	NSAlert* const alert = [[NSAlert alloc] init];
	[alert addButtonWithTitle:@"OK"];
	[alert addButtonWithTitle:@"Cancel"];
    [alert setInformativeText:message];
	[alert setMessageText:@"Confirm"];

    dispatch_async(dispatch_get_main_queue(), ^
    {
        [alert beginSheetModalForWindow:[webview window]
                      completionHandler:^(NSModalResponse result)
        {
            completionHandler(result == NSAlertFirstButtonReturn);
            [alert release];
        }];
    });
}

- (void)webView:(WKWebView*)webview
    runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
                              defaultText:(NSString*)defaultText
                         initiatedByFrame:(WKFrameInfo*)frame
                        completionHandler:(void (^)(NSString*))completionHandler
{
    NSTextField* const input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 250, 30)];
    [input setStringValue:defaultText];

	NSAlert* const alert = [[NSAlert alloc] init];
    [alert setAccessoryView:input];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert setInformativeText:prompt];
    [alert setMessageText: @"Prompt"];

    dispatch_async(dispatch_get_main_queue(), ^
    {
        [alert beginSheetModalForWindow:[webview window]
                      completionHandler:^(NSModalResponse result)
        {
            [input validateEditing];
            completionHandler(result == NSAlertFirstButtonReturn ? [input stringValue] : nil);
            [alert release];
        }];
    });
}

- (void)webView:(WKWebView*)webview
    runOpenPanelWithParameters:(WKOpenPanelParameters*)params
              initiatedByFrame:(WKFrameInfo*)frame
             completionHandler:(void (^)(NSArray<NSURL*>*))completionHandler
{
    NSOpenPanel* const panel = [[NSOpenPanel alloc] init];

    [panel setAllowsMultipleSelection:[params allowsMultipleSelection]];
    // [panel setAllowedFileTypes:(NSArray<NSString*>*)[params _allowedFileExtensions]];
    [panel setCanChooseDirectories:[params allowsDirectories]];
    [panel setCanChooseFiles:![params allowsDirectories]];

    dispatch_async(dispatch_get_main_queue(), ^
    {
        [panel beginSheetModalForWindow:[webview window]
                      completionHandler:^(NSModalResponse result)
        {
            completionHandler(result == NSModalResponseOK ? [panel URLs] : nil);
            [panel release];
        }];
    });
}

@end

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

struct WebViewImpl {
    NSView* const view;
    WKWebView* const webview;
    NSURLRequest* const urlreq;
    WEBVIEW_DELEGATE_CLASS_NAME* const delegate;
};

// -----------------------------------------------------------------------------------------------------------

void* addWebView(const uintptr_t parentWinId, double, const uint port)
{
    NSView* const view = reinterpret_cast<NSView*>(parentWinId);

    const CGRect rect = CGRectMake(0,
                                   kVerticalOffset,
                                   DISTRHO_UI_DEFAULT_WIDTH,
                                   DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset);

    WKPreferences* const prefs = [[WKPreferences alloc] init];
    [prefs setValue:@YES forKey:@"developerExtrasEnabled"];

    WKWebViewConfiguration* const config = [[WKWebViewConfiguration alloc] init];
    config.preferences = prefs;

    WKWebView* const webview = [[WKWebView alloc] initWithFrame:rect
                                                  configuration:config];
    [view addSubview:webview];

    WEBVIEW_DELEGATE_CLASS_NAME* const delegate = [[WEBVIEW_DELEGATE_CLASS_NAME alloc] init];
    webview.UIDelegate = delegate;

    char url[32];
    std::snprintf(url, 31, "http://127.0.0.1:%u/", port);
    NSString* const nsurl = [[NSString alloc] initWithBytes:url
                                                     length:std::strlen(url)
                                                   encoding:NSUTF8StringEncoding];
    NSURLRequest* const urlreq = [[NSURLRequest alloc] initWithURL: [NSURL URLWithString: nsurl]];

    [webview loadRequest:urlreq];
    [webview setHidden:NO];

    [nsurl release];
    [config release];
    [prefs release];

    return new WebViewImpl{view, webview, urlreq, delegate};
}

void destroyWebView(void* const webview)
{
    WebViewImpl* const impl = static_cast<WebViewImpl*>(webview);

    [impl->webview setHidden:YES];
    [impl->webview removeFromSuperview];
    [impl->urlreq release];
    [impl->delegate release];

    delete impl;
}

void reloadWebView(void* const webview, uint)
{
    WebViewImpl* const impl = static_cast<WebViewImpl*>(webview);

    [impl->webview loadRequest:impl->urlreq];
}

void resizeWebView(void* const webview, const uint offset, const uint width, const uint height)
{
    WebViewImpl* const impl = static_cast<WebViewImpl*>(webview);

    [impl->webview setFrame:CGRectMake(0, offset, width, height)];
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

#undef MACRO_NAME
#undef MACRO_NAME2
