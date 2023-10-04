[Setup]
ArchitecturesInstallIn64BitMode=x64
AppName=mod-app
AppPublisher=DISTRHO
AppPublisherURL=https://github.com/DISTRHO/Cardinal/
AppSupportURL=https://github.com/DISTRHO/Cardinal/issues/
AppUpdatesURL=https://github.com/DISTRHO/Cardinal/releases/
AppVersion={#VERSION}
DefaultDirName={commonpf64}\mod-app
DisableDirPage=yes
DisableWelcomePage=no
LicenseFile=..\LICENSE
OutputBaseFilename=mod-app-win64-{#VERSION}-installer
OutputDir=.
UsePreviousAppDir=no

[Types]
Name: "normal"; Description: "Full installation";

[Files]
; icon
; Source: ".\mod-app.ico"; DestDir: "{app}"; Flags: ignoreversion;
; jack
Source: "..\build\jackd.exe"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\build\libjack64.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\build\libjackserver64.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\build\jack\jack-session.conf"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\build\jack\jack_portaudio.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\build\jack\jack_winmme.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\build\jack\mod-host.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\build\jack\mod-midi-broadcaster.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\build\jack\mod-midi-merger.dll"; DestDir: "{app}"; Flags: ignoreversion;
; mod-ui
Source: "..\build\mod-ui.exe"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\build\libpython3.8.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\build\lib\*.dll"; DestDir: "{app}\lib"; Flags: ignoreversion;
Source: "..\build\lib\*.zip"; DestDir: "{app}\lib"; Flags: ignoreversion;
Source: "..\build\mod\*.py"; DestDir: "{app}\mod"; Flags: ignoreversion;
Source: "..\build\mod\communication\*.py"; DestDir: "{app}\mod\communication"; Flags: ignoreversion;
Source: "..\build\modtools\*.py"; DestDir: "{app}\modtools"; Flags: ignoreversion;
Source: "..\build\default.pedalboard\manifest.ttl"; DestDir: "{app}\default.pedalboard"; Flags: ignoreversion;
Source: "..\build\default.pedalboard\screenshot.png"; DestDir: "{app}\default.pedalboard"; Flags: ignoreversion;
Source: "..\build\default.pedalboard\thumbnail.png"; DestDir: "{app}\default.pedalboard"; Flags: ignoreversion;
Source: "..\build\html\*.html"
Source: "..\build\html\favicon.ico"
; plugins
