@ECHO OFF

cd /D "%~dp0"
setx LV2_PATH "%cd%\plugins"
.\jackd.exe -R -S -X winmme -C .\jack\jack-session.conf -d portaudio -r 48000 -p 128 -d "ASIO:WineASIO Driver"
