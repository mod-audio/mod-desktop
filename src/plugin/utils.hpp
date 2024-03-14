// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "DistrhoPluginUtils.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

/* Get the MOD Desktop application directory.
 */
const char* getAppDir();

/* Get environment to be used for a child process.
 */
#ifdef _WIN32
const WCHAR* getEvironment(uint portBaseNum);
#else
char* const* getEvironment(uint portBaseNum);
#endif

/* Open a web browser with the mod-ui URL as address.
 */
void openWebGui(uint port);

/* Open the "user files" directory in a file manager/explorer.
 */
void openUserFilesDir();

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
