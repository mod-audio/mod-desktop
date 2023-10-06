# MOD App

**MOD Audio is on the desktop!**

This is the source code repository for MOD App, a little tool by [MOD Audio](https://mod.audio/) that combines all the goodies from MOD for the desktop platform packaged in a convenient installer.

![screenshot](mod-app.png "mod-app")

## Current status

At this point this tool should be considered VERY EXPERIMENTAL.
Specially due to MOD related software and plugins never been tested on Windows systems before.
Feedback and testing is very much appreciated, make sure to report issues you find during your own testing.

In particular the following issues are known to happen:

- Pedalboard save crashes mod-ui, due to being unable to create a screenshot/thumbnail for it
- Handling of Windows filepaths is not always correct (differences between POSIX vs Windows path separators)
- jackd.exe (through mod-host) asks for public network permissions which are not needed
- JACK is used "as-is", which assumes it is not running yet. TBD if we use a custom server name or something else
- MIDI usage crashes jackd.exe, issue reported as [jack2#931](https://github.com/jackaudio/jack2/issues/931)

## Download

MOD App only has builds for Windows 64bit for now, find them in the [releases section](https://github.com/moddevices/mod-app/releases).

## License

MOD App is licensed under AGPLv3+, see [LICENSE](LICENSE) for more details.  
