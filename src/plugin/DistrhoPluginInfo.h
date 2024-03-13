// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#define DISTRHO_PLUGIN_BRAND   "MOD Audio"
#define DISTRHO_PLUGIN_NAME    "MOD Desktop"
#define DISTRHO_PLUGIN_URI     "https://mod.audio/desktop/"
#define DISTRHO_PLUGIN_CLAP_ID "audio.mod.desktop"

#define DISTRHO_PLUGIN_BRAND_ID  MODa
#define DISTRHO_PLUGIN_UNIQUE_ID dskt

#define DISTRHO_PLUGIN_HAS_UI           1
#define DISTRHO_PLUGIN_IS_RT_SAFE       0
#define DISTRHO_PLUGIN_NUM_INPUTS       2
#define DISTRHO_PLUGIN_NUM_OUTPUTS      2
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  1
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 1
#define DISTRHO_PLUGIN_WANT_STATE       1
#define DISTRHO_UI_FILE_BROWSER         0
#define DISTRHO_UI_DEFAULT_WIDTH        1170
#define DISTRHO_UI_DEFAULT_HEIGHT       600
#define DISTRHO_UI_USE_NANOVG           1
#define DISTRHO_UI_USER_RESIZABLE       1

static const constexpr unsigned int kVerticalOffset = 30;

enum Parameters {
    kParameterBasePortNumber,
    kParameterCount
};
