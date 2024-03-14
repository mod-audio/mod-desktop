// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "DistrhoUtils.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

void* addWebView(uintptr_t parentWinId, uint port);
void destroyWebView(void* webview);
void reloadWebView(void* webview);
void resizeWebView(void* webview, uint offset, uint width, uint height);

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
