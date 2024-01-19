# MOD Desktop App

**MOD Audio is on the desktop!**

This is the source code repository for MOD Desktop App, a little tool by [MOD Audio](https://mod.audio/) that combines all the goodies from MOD for the desktop platform packaged in a convenient installer.

![screenshot](mod-desktop-app.png "mod-desktop-app")

## Current status

At this point this tool should be considered in beta state.
Most things already work but we are still tweaking and fixing a lot, specially due to MOD related software and plugins never been tested on Windows systems before.
Feedback and testing is very much appreciated, make sure to report issues you find during your own testing.

Current known issues:

- Handling of Windows filepaths is not always correct (differences between POSIX vs Windows path separators)
- When mod-host crashes it will stop the UI process, instead of being automatically restarted like in MOD units

## Download

MOD Desktop App has builds for:

- Linux x86_64
- macOS universal (Intel + Apple Sillicon)
- Windows 64bit

You can find them in the [releases section](https://github.com/moddevices/mod-desktop-app/releases).

## Development

If you want to contribute, here are a few items where help would be appreciated:

- Debugging and fixing Windows specific issues within mod-ui (*)
- Documentation regarding "universal" ASIO drivers, like ASIO4ALL and FlexASIO, and how to set them nicely for MOD Desktop App

(*) Note: on Windows the default installation has html/css/js files in `C:\Program Files\MOD Desktop App\html` and python files in `C:\Program Files\MOD Desktop App\mod`, which can be directly modified, making it very easy and convenient to try out any changes.

Also help in these areas, but they are much more involved:
- Create a JACK-API-compatible node-based audio graph, to be used for eventually running "mod-desktop-app as a plugin" (there are a few opensource libs for this already, like the one included in miniaudio)
- Create a virtual sound device for macOS that can send audio into a JACK client, similar to JACK-Router and WineASIO projects

## License

MOD Desktop App is licensed under AGPLv3+, see [LICENSE](LICENSE) for more details.
