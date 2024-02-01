#include "win64-version.iss"

[Setup]
ArchitecturesInstallIn64BitMode=x64
AppName=MOD Desktop
AppPublisher=MOD Audio
AppPublisherURL=https://mod.audio/
AppSupportURL=https://github.com/moddevices/mod-desktop/issues/
AppUpdatesURL=https://github.com/moddevices/mod-desktop/releases/
AppVersion={#VERSION}
DefaultDirName={commonpf64}\MOD Desktop
DisableDirPage=yes
DisableWelcomePage=no
LicenseFile=..\..\LICENSE
OutputBaseFilename=mod-desktop-{#VERSION}-win64-installer
OutputDir=..\..
UsePreviousAppDir=no

[Types]
Name: "normal"; Description: "Full installation";

[Components]
Name: plugins; Description: "LV2 plugins"; Types: normal; Flags: fixed;

[Files]
; icon
Source: "..\..\res\mod-logo.ico"; DestDir: "{app}"; Flags: ignoreversion;
; jack + mod-host
Source: "..\..\build\jackd.exe"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\..\build\libjack64.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\..\build\libjackserver64.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\..\build\jack\jack-session.conf"; DestDir: "{app}\jack"; Flags: ignoreversion;
Source: "..\..\build\jack\jack_portaudio.dll"; DestDir: "{app}\jack"; Flags: ignoreversion;
Source: "..\..\build\jack\jack_winmme.dll"; DestDir: "{app}\jack"; Flags: ignoreversion;
Source: "..\..\build\jack\mod-host.dll"; DestDir: "{app}\jack"; Flags: ignoreversion;
Source: "..\..\build\jack\mod-midi-broadcaster.dll"; DestDir: "{app}\jack"; Flags: ignoreversion;
Source: "..\..\build\jack\mod-midi-merger.dll"; DestDir: "{app}\jack"; Flags: ignoreversion;
; mod-desktop + qt5
Source: "..\..\build\mod-desktop.exe"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\..\build\Qt5*.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\..\build\generic\q*.dll"; DestDir: "{app}\generic"; Flags: ignoreversion;
Source: "..\..\build\iconengines\q*.dll"; DestDir: "{app}\iconengines"; Flags: ignoreversion;
Source: "..\..\build\imageformats\q*.dll"; DestDir: "{app}\imageformats"; Flags: ignoreversion;
Source: "..\..\build\platforms\q*.dll"; DestDir: "{app}\platforms"; Flags: ignoreversion;
Source: "..\..\build\styles\q*.dll"; DestDir: "{app}\styles"; Flags: ignoreversion;
; mod-ui + python
Source: "..\..\build\mod-pedalboard.exe"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\..\build\mod-ui.exe"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\..\build\libpython3.8.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\..\build\default.pedalboard\manifest.ttl"; DestDir: "{app}\default.pedalboard"; Flags: ignoreversion;
Source: "..\..\build\default.pedalboard\*.png"; DestDir: "{app}\default.pedalboard"; Flags: ignoreversion;
Source: "..\..\build\html\*.html"; DestDir: "{app}\html"; Flags: ignoreversion;
Source: "..\..\build\html\favicon.ico"; DestDir: "{app}\html"; Flags: ignoreversion;
Source: "..\..\build\html\css\*.css"; DestDir: "{app}\html\css"; Flags: ignoreversion;
Source: "..\..\build\html\css\fontello\css\*.css"; DestDir: "{app}\html\css\fontello\css"; Flags: ignoreversion;
Source: "..\..\build\html\css\fontello\font\*.eot"; DestDir: "{app}\html\css\fontello\font"; Flags: ignoreversion;
Source: "..\..\build\html\css\fontello\font\*.svg"; DestDir: "{app}\html\css\fontello\font"; Flags: ignoreversion;
Source: "..\..\build\html\css\fontello\font\*.ttf"; DestDir: "{app}\html\css\fontello\font"; Flags: ignoreversion;
Source: "..\..\build\html\css\fontello\font\*.woff"; DestDir: "{app}\html\css\fontello\font"; Flags: ignoreversion;
Source: "..\..\build\html\css\fontello\font\*.woff2"; DestDir: "{app}\html\css\fontello\font"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-200\*.eot"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-200"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-200\*.svg"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-200"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-200\*.ttf"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-200"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-200\*.woff"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-200"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-200\*.woff2"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-200"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-600\*.eot"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-600"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-600\*.svg"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-600"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-600\*.ttf"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-600"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-600\*.woff"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-600"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-600\*.woff2"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-600"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-700\*.eot"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-700"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-700\*.svg"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-700"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-700\*.ttf"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-700"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-700\*.woff"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-700"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-700\*.woff2"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-700"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-regular\*.eot"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-regular"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-regular\*.svg"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-regular"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-regular\*.ttf"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-regular"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-regular\*.woff"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-regular"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\Ek-Mukta\Ek-Mukta-regular\*.woff2"; DestDir: "{app}\html\fonts\Ek-Mukta\Ek-Mukta-regular"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\comforta\*.ttf"; DestDir: "{app}\html\fonts\comforta"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\cooper\*.eot"; DestDir: "{app}\html\fonts\cooper"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\cooper\*.ttf"; DestDir: "{app}\html\fonts\cooper"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\cooper\*.woff"; DestDir: "{app}\html\fonts\cooper"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\cooper\*.woff2"; DestDir: "{app}\html\fonts\cooper"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\css\*.css"; DestDir: "{app}\html\fonts\css"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\england-hand\*.css"; DestDir: "{app}\html\fonts\england-hand"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\england-hand\*.eot"; DestDir: "{app}\html\fonts\england-hand"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\england-hand\*.svg"; DestDir: "{app}\html\fonts\england-hand"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\england-hand\*.ttf"; DestDir: "{app}\html\fonts\england-hand"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\england-hand\*.woff"; DestDir: "{app}\html\fonts\england-hand"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\epf\*.css"; DestDir: "{app}\html\fonts\epf"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\epf\*.eot"; DestDir: "{app}\html\fonts\epf"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\epf\*.svg"; DestDir: "{app}\html\fonts\epf"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\epf\*.ttf"; DestDir: "{app}\html\fonts\epf"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\epf\*.woff"; DestDir: "{app}\html\fonts\epf"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\nexa\*.css"; DestDir: "{app}\html\fonts\nexa"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\nexa\*.eot"; DestDir: "{app}\html\fonts\nexa"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\nexa\*.svg"; DestDir: "{app}\html\fonts\nexa"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\nexa\*.ttf"; DestDir: "{app}\html\fonts\nexa"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\nexa\*.woff"; DestDir: "{app}\html\fonts\nexa"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\pirulen\*.css"; DestDir: "{app}\html\fonts\pirulen"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\pirulen\*.eot"; DestDir: "{app}\html\fonts\pirulen"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\pirulen\*.ttf"; DestDir: "{app}\html\fonts\pirulen"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\pirulen\*.woff"; DestDir: "{app}\html\fonts\pirulen"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\questrial\*.css"; DestDir: "{app}\html\fonts\questrial"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\questrial\*.eot"; DestDir: "{app}\html\fonts\questrial"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\questrial\*.svg"; DestDir: "{app}\html\fonts\questrial"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\questrial\*.ttf"; DestDir: "{app}\html\fonts\questrial"; Flags: ignoreversion;
Source: "..\..\build\html\fonts\questrial\*.woff"; DestDir: "{app}\html\fonts\questrial"; Flags: ignoreversion;
Source: "..\..\build\html\img\*.gif"; DestDir: "{app}\html\img"; Flags: ignoreversion;
Source: "..\..\build\html\img\*.jpg"; DestDir: "{app}\html\img"; Flags: ignoreversion;
Source: "..\..\build\html\img\*.png"; DestDir: "{app}\html\img"; Flags: ignoreversion;
Source: "..\..\build\html\img\*.svg"; DestDir: "{app}\html\img"; Flags: ignoreversion;
Source: "..\..\build\html\img\cloud\*.png"; DestDir: "{app}\html\img\cloud"; Flags: ignoreversion;
Source: "..\..\build\html\img\favicon\*.png"; DestDir: "{app}\html\img\favicon"; Flags: ignoreversion;
Source: "..\..\build\html\img\icons\*.css"; DestDir: "{app}\html\img\icons"; Flags: ignoreversion;
Source: "..\..\build\html\img\icons\*.svg"; DestDir: "{app}\html\img\icons"; Flags: ignoreversion;
Source: "..\..\build\html\img\icons\*.png"; DestDir: "{app}\html\img\icons"; Flags: ignoreversion;
Source: "..\..\build\html\img\icons\25\*.png"; DestDir: "{app}\html\img\icons\25"; Flags: ignoreversion;
Source: "..\..\build\html\img\icons\36\*.png"; DestDir: "{app}\html\img\icons\36"; Flags: ignoreversion;
Source: "..\..\build\html\img\social\*.png"; DestDir: "{app}\html\img\social"; Flags: ignoreversion;
Source: "..\..\build\html\include\*.html"; DestDir: "{app}\html\include"; Flags: ignoreversion;
Source: "..\..\build\html\js\*.js"; DestDir: "{app}\html\js"; Flags: ignoreversion;
Source: "..\..\build\html\js\utils\*.js"; DestDir: "{app}\html\js\utils"; Flags: ignoreversion;
Source: "..\..\build\html\js\lib\*.js"; DestDir: "{app}\html\js\lib"; Flags: ignoreversion;
Source: "..\..\build\html\js\lib\slick\*.js"; DestDir: "{app}\html\js\lib\slick"; Flags: ignoreversion;
Source: "..\..\build\html\js\lib\slick\*.css"; DestDir: "{app}\html\js\lib\slick"; Flags: ignoreversion;
Source: "..\..\build\html\js\lib\slick\*.gif"; DestDir: "{app}\html\js\lib\slick"; Flags: ignoreversion;
Source: "..\..\build\html\js\lib\slick\fonts\*.eot"; DestDir: "{app}\html\js\lib\slick\fonts"; Flags: ignoreversion;
Source: "..\..\build\html\js\lib\slick\fonts\*.svg"; DestDir: "{app}\html\js\lib\slick\fonts"; Flags: ignoreversion;
Source: "..\..\build\html\js\lib\slick\fonts\*.ttf"; DestDir: "{app}\html\js\lib\slick\fonts"; Flags: ignoreversion;
Source: "..\..\build\html\js\lib\slick\fonts\*.woff"; DestDir: "{app}\html\js\lib\slick\fonts"; Flags: ignoreversion;
Source: "..\..\build\html\resources\*.html"; DestDir: "{app}\html\resources"; Flags: ignoreversion;
Source: "..\..\build\html\resources\pedals\*.css"; DestDir: "{app}\html\resources\pedals"; Flags: ignoreversion;
Source: "..\..\build\html\resources\pedals\*.png"; DestDir: "{app}\html\resources\pedals"; Flags: ignoreversion;
Source: "..\..\build\html\resources\templates\*.html"; DestDir: "{app}\html\resources\templates"; Flags: ignoreversion;
Source: "..\..\build\lib\*.*"; DestDir: "{app}\lib"; Flags: ignoreversion;
Source: "..\..\build\lib\Cryptodome\*.*"; DestDir: "{app}\lib\Cryptodome"; Flags: ignoreversion;
Source: "..\..\build\lib\Cryptodome\Cipher\*.*"; DestDir: "{app}\lib\Cryptodome\Cipher"; Flags: ignoreversion;
Source: "..\..\build\lib\Cryptodome\Hash\*.*"; DestDir: "{app}\lib\Cryptodome\Hash"; Flags: ignoreversion;
Source: "..\..\build\lib\Cryptodome\IO\*.*"; DestDir: "{app}\lib\Cryptodome\IO"; Flags: ignoreversion;
Source: "..\..\build\lib\Cryptodome\Math\*.*"; DestDir: "{app}\lib\Cryptodome\Math"; Flags: ignoreversion;
Source: "..\..\build\lib\Cryptodome\Protocol\*.*"; DestDir: "{app}\lib\Cryptodome\Protocol"; Flags: ignoreversion;
Source: "..\..\build\lib\Cryptodome\PublicKey\*.*"; DestDir: "{app}\lib\Cryptodome\PublicKey"; Flags: ignoreversion;
Source: "..\..\build\lib\Cryptodome\Random\*.*"; DestDir: "{app}\lib\Cryptodome\Random"; Flags: ignoreversion;
Source: "..\..\build\lib\Cryptodome\Signature\*.*"; DestDir: "{app}\lib\Cryptodome\Signature"; Flags: ignoreversion;
Source: "..\..\build\lib\Cryptodome\Util\*.*"; DestDir: "{app}\lib\Cryptodome\Util"; Flags: ignoreversion;
Source: "..\..\build\lib\PIL\*.*"; DestDir: "{app}\lib\PIL"; Flags: ignoreversion;
Source: "..\..\build\mod\*.py"; DestDir: "{app}\mod"; Flags: ignoreversion;
Source: "..\..\build\mod\communication\*.py"; DestDir: "{app}\mod\communication"; Flags: ignoreversion;
Source: "..\..\build\modtools\*.py"; DestDir: "{app}\modtools"; Flags: ignoreversion;
; asio driver
Source: "..\..\build\mod-desktop-asio.dll"; DestDir: "{win}\System32"; Flags: ignoreversion regserver 64bit;
; misc
Source: "..\..\build\mod-hardware-descriptor.json"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\..\build\VERSION"; DestDir: "{app}"; Flags: ignoreversion;
; plugins
#include "win64-plugins.iss"

[Icons]
Name: "{commonprograms}\MOD Desktop"; Filename: "{app}\mod-desktop.exe"; IconFilename: "{app}\mod-logo.ico"; WorkingDir: "{app}"; Comment: "MOD Desktop";
