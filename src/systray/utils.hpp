// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QtWidgets/QWidget>

static inline
QWidget* getLastParentOrSelf(QWidget* const w) noexcept
{
    if (QWidget* const p = dynamic_cast<QWidget*>(w->parent()))
        return getLastParentOrSelf(p);

    return w;
}

/* Setup environment, should be the first call in the application.
 * Also creates needed directories and change current working directory to where this application lives in.
 */
void initEvironment();

/* Get the LV2 path to use, optionally including all/global plugins too.
 */
QString getLV2Path(bool includeSystemPlugins);

/* Open a web browser with the mod-ui URL as address.
 */
void openWebGui();

/* Open the "user files" directory in a file manager/explorer.
 */
void openUserFilesDir();

/* Whether the application should try to force a dark mode.
 * Currently only supported on Windows.
 */
bool shouldUseDarkMode();

/* Setup signal handling so that Ctrl+C cleanly closes the application.
 */
void setupControlCloseSignal();

/* Setup dark mode for an application, also changing to "Fusion" theme.
 */
void setupDarkModePalette(QApplication& app);

/* Setup OS-native dark mode for a window.
 * Currently only supported on Windows.
 */
void setupDarkModeWindow(QMainWindow& window);

/* Save MIDI related settings to user profile.
 * This is meant to be used before starting mod-ui, so it then reads from these settings.
 */
void writeMidiChannelsToProfile(int pedalboard, int snapshot);
