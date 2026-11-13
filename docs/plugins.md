# Using your own LV2 plugins in MOD Desktop

MOD Desktop ships with a fixed set of plugins, and it also loads LV2 plugins you build or download yourself. This page
explains where to put them, what they have to be built for, and what to look at when one does not load.

## 1. Build the plugin for your computer

A plugin built for a MOD Dwarf, Duo or Duo X is an ARM Linux binary and will not load in MOD Desktop. MOD Desktop needs
a native binary for the computer it runs on:

| Computer | Binary in the bundle | Build for |
| --- | --- | --- |
| macOS, Apple Silicon | `.dylib` | `arm64` (or universal `arm64` + `x86_64`) |
| macOS, Intel | `.dylib` | `x86_64` (or universal) |
| Windows | `.dll` | 64-bit (`x86_64`) |
| Linux | `.so` | `x86_64` |

Build on the operating system you target: a Mac for macOS, a Windows machine (or a cross toolchain) for Windows.

Two ways to get there:

- **Your own plugin, any build system.** Build it natively as you would for any LV2 host. Keep the binary
  self-contained: link libraries statically where you can, because a library found on your machine (Homebrew, a
  system package) may not be there on another one. [PawPaw](https://github.com/DISTRHO/PawPaw) provides static
  libraries for this: `./bootstrap-plugins.sh <target>`, then `source local.env <target>` and build your plugin in the
  same shell. Targets are `macos-universal-10.15`, `win64` and `linux-x86_64`.
- **A plugin that already has a MOD recipe** (from
  [mod-plugin-builder](https://github.com/mod-audio/mod-plugin-builder)): MOD Desktop builds its own plugin set from
  those recipes. From a checkout of this repository, bootstrap the toolchain once with
  `./src/PawPaw/bootstrap-mod.sh <target>`, then build a recipe with
  `./utils/plugin-builder/plugin-builder.sh <target> <recipe>`. The bundle lands in
  `~/PawPawBuilds/targets/<target>/lib/lv2/`.

A modgui is optional. A plugin without one works and shows the generic box, as on a MOD unit.

## 2. Put the bundle where MOD Desktop looks

Copy the whole `.lv2` folder (manifest, `.ttl` files, binary, and `modgui/` if it has one) into the `lv2` folder inside
MOD Desktop's documents folder:

| OS | Folder |
| --- | --- |
| macOS | `~/Documents/MOD Desktop/lv2/` |
| Windows | `Documents\MOD Desktop\lv2\` |
| Linux | `~/Documents/MOD Desktop/lv2/` (your desktop's Documents folder, which may have a translated name) |

MOD Desktop creates `MOD Desktop` the first time it runs, but not the `lv2` folder inside it: create it yourself.
This folder is always scanned.

The system LV2 folders (`~/Library/Audio/Plug-Ins/LV2` and `/Library/Audio/Plug-Ins/LV2` on macOS, `%LOCALAPPDATA%\LV2`
and `%COMMONPROGRAMFILES%\LV2` on Windows, `~/.lv2`, `/usr/local/lib/lv2` and `/usr/lib/lv2` on Linux, or your
`LV2_PATH` there if you set one) are scanned only when **Show all LV2 plugins** is ticked in the MOD Desktop window. That brings in every
plugin installed for your other audio software too, which is why it is off by default.

MOD Desktop inside a DAW (the VST3, CLAP, AU or LV2 plugin) only scans its own `lv2` folder, never the system
folders. Use the `lv2` folder if you want your plugin in both.

If **Only show plugins with a MODGUI** is ticked, a plugin without a modgui is hidden.

## 3. Restart the audio

Plugins are scanned when the audio starts. After adding or replacing a bundle, press **Stop** and then **Start** in the
MOD Desktop window, and reload the web interface.

## 4. macOS: signing and quarantine

- MOD Desktop **0.14.2 and later** load plugins that are not signed by MOD. Earlier signed builds (0.0.12, 0.0.13
  and the test builds before 0.14.2) refuse them: the plugin appears in the plugin list, but adding it fails with "Error adding effect".
- The binary still needs a valid code signature for Apple Silicon. The ad-hoc signature that Apple's linker adds by
  itself is enough; if you changed the binary after linking, sign it again with `codesign -f -s - <file>.dylib`.
- For a bundle you downloaded, remove the quarantine flag: `xattr -cr "<bundle>.lv2"`.

## 5. When a plugin does not load

1. Read the log. The MOD Desktop app writes what the audio engine prints to `launcher.log` in the documents folder
   above (`MOD Desktop/launcher.log`). For more detail, tick **Show logging panel**, then **Verbose mod-host**, and restart
   the audio. The reason a plugin fails to load (library not found, wrong architecture, refused signature) appears
   there.
2. Check the architecture: `file <binary>` (macOS also `lipo -archs <file>.dylib`) must show the one from step 1.
3. Check that the plugin's URI in `manifest.ttl` matches the one in its own `.ttl` file and in the binary.
4. Check missing libraries: `otool -L` on macOS, `ldd` on Linux. Anything outside the system folders must be present on
   the machine, or linked statically.

## Good to know

- MOD Desktop runs the audio engine at 48 kHz, with a block size of 64, 128 or 256 frames (the **Buffer size**
  setting). Inside a DAW it always runs at 48 kHz with 128 frames. Test your plugin at these settings.
- A plugin that works in MOD Desktop still has to be built for the MOD unit (with mod-plugin-builder) to run there.
- Developer reference: [MOD Plugin SDK, DESKTOP.md](https://github.com/mod-audio/mod-plugin-sdk/blob/main/DESKTOP.md).
