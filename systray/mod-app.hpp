// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "ui_mod-app.hpp"

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSystemTrayIcon>

#ifdef Q_OS_MAC
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CFString.h>
#else
#include <portaudio.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <cstring>
#endif

QString getUserFilesDir();
void writeMidiChannelsToProfile(int pedalboard, int snapshot);

#ifdef Q_OS_MAC
static bool getDeviceAudioProperty(const AudioObjectID deviceID,
                                   const AudioObjectPropertyAddress* const prop,
                                   uint32_t* const size,
                                   void* const outPtr)
{
    return AudioObjectGetPropertyData(deviceID, prop, 0, nullptr, size, outPtr) == kAudioHardwareNoError;
}

static bool getDeviceAudioPropertySize(const AudioObjectID deviceID,
                                       const AudioObjectPropertyAddress* const prop,
                                       uint32_t* const size)
{
    return AudioObjectGetPropertyDataSize(deviceID, prop, 0, nullptr, size) == kAudioHardwareNoError;
}

static bool getSystemAudioProperty(const AudioObjectPropertyAddress* const prop,
                                   uint32_t* const size,
                                   void* const outPtr)
{
    return getDeviceAudioProperty(kAudioObjectSystemObject, prop, size, outPtr);
}

static bool getSystemAudioPropertySize(const AudioObjectPropertyAddress* const prop, uint32_t* const size)
{
    return getDeviceAudioPropertySize(kAudioObjectSystemObject, prop, size);
}
#endif

class AppProcess : public QProcess
{
public:
    AppProcess(QObject* const parent, const QString& cwd)
        : QProcess(parent)
    {
        setProcessChannelMode(QProcess::MergedChannels);
        setWorkingDirectory(cwd);
    }

    void terminate()
    {
        QProcess::terminate();

        if (! waitForFinished(500))
            kill();
    }

public slots:
    void startSlot()
    {
        start();
    }
};

class AppWindow : public QMainWindow
{
//     Q_OBJECT

    Ui_AppWindow ui;

    QAction* openGuiAction = nullptr;
    QAction* openUserFilesAction = nullptr;
    QAction* settingsAction = nullptr;
    QAction* quitAction = nullptr;
    QMenu* sysmenu = nullptr;
    QSystemTrayIcon* systray = nullptr;

    int heightLogs = 100;

    AppProcess processHost;
    AppProcess processUI;
    bool startingHost = false;
    bool startingUI = false;
    bool stoppingHost = false;
    bool stoppingUI = false;
    bool successfullyStarted = false;
    int timerId = 0;

    struct DeviceInfo {
        QString uid;
        bool canInput = false;
        bool canUseSeparateInput = false;
    };
    QList<DeviceInfo> devices;
    QStringList inputs;

public:
    AppWindow(const QString& cwd)
        : processHost(this, cwd),
          processUI(this, cwd)
    {
        ui.setupUi(this);

        connect(ui.b_start, &QPushButton::clicked, this, &AppWindow::start);
        connect(ui.b_stop, &QPushButton::clicked, this, &AppWindow::stop);
        connect(ui.b_opengui, &QPushButton::clicked, this, &AppWindow::openGui);
        connect(ui.b_openuserfiles, &QPushButton::clicked, this, &AppWindow::openUserFilesDir);
        connect(ui.cb_device, &QComboBox::currentTextChanged, this, &AppWindow::updateDeviceDetails);

        const QIcon icon(":/mod-logo.svg");

        settingsAction = new QAction(tr("&Open Panel Settings"), this);
        connect(settingsAction, &QAction::triggered, this, &QMainWindow::show);

        openGuiAction = new QAction(tr("Open &Web GUI"), this);
        connect(openGuiAction, &QAction::triggered, this, &AppWindow::openGui);

        openUserFilesAction = new QAction(tr("Open &User Files"), this);
        connect(openUserFilesAction, &QAction::triggered, this, &AppWindow::openUserFilesDir);

        quitAction = new QAction(tr("&Quit"), this);
        connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

        sysmenu = new QMenu(this);
        sysmenu->addAction(settingsAction);
        sysmenu->addSeparator();
        sysmenu->addAction(openGuiAction);
        sysmenu->addAction(openUserFilesAction);
        sysmenu->addSeparator();
        sysmenu->addAction(quitAction);

        systray = new QSystemTrayIcon(icon, this);
        systray->setContextMenu(sysmenu);
       #ifndef Q_OS_WIN
        systray->setIcon(QPixmap(":/mod-logo.svg"));
       #endif
        connect(systray, &QSystemTrayIcon::messageClicked, this, &AppWindow::messageClicked);
        connect(systray, &QSystemTrayIcon::activated, this, &AppWindow::iconActivated);

        fillInDeviceList();
        loadSettings();

       #ifdef Q_OS_WIN
        processHost.setProgram(cwd + "\\jackd.exe");
        processUI.setProgram(cwd + "\\mod-ui.exe");
       #else
        processHost.setProgram(cwd + "/jackd");
        processUI.setProgram(cwd + "/mod-ui");
       #endif

        connect(&processHost, &QProcess::errorOccurred, this, &AppWindow::hostStartError);
        connect(&processHost, &QProcess::started, this, &AppWindow::hostStartSuccess);
        connect(&processHost, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &AppWindow::hostFinished);

        connect(&processUI, &QProcess::errorOccurred, this, &AppWindow::uiStartError);
        connect(&processUI, &QProcess::started, this, &AppWindow::uiStartSuccess);
        connect(&processUI, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &AppWindow::uiFinished);

        timerId = startTimer(500);
    }

    ~AppWindow()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);

        close();
    }

    void fillInDeviceList()
    {
        printf("--------------------------------------------------------\n");

       #ifdef Q_OS_WIN
        SetEnvironmentVariableW(L"JACK_NO_AUDIO_RESERVATION", L"1");
        SetEnvironmentVariableW(L"JACK_NO_START_SERVER", L"1");
        SetEnvironmentVariableW(L"PYTHONUNBUFFERED", L"1");
       #else
        setenv("JACK_NO_AUDIO_RESERVATION", "1", 1);
        setenv("JACK_NO_START_SERVER", "1", 1);
        setenv("PYTHONUNBUFFERED", "1", 1);
       #endif

        ui.cb_device->blockSignals(true);

       #ifdef Q_OS_MAC
        constexpr const AudioObjectPropertyAddress propDevices = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioObjectPropertyScopeGlobal,
            .mSelector = kAudioHardwarePropertyDevices,
        };
        constexpr const AudioObjectPropertyAddress propDefaultInputDevice = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioObjectPropertyScopeGlobal,
            .mSelector = kAudioHardwarePropertyDefaultInputDevice,
        };
        constexpr const AudioObjectPropertyAddress propDefaultOutputDevice = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioObjectPropertyScopeGlobal,
            .mSelector = kAudioHardwarePropertyDefaultOutputDevice,
        };
        constexpr const AudioObjectPropertyAddress propDeviceName = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioObjectPropertyScopeGlobal,
            .mSelector = kAudioDevicePropertyDeviceNameCFString,
        };
        constexpr const AudioObjectPropertyAddress propDeviceInputUID = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioDevicePropertyScopeInput,
            .mSelector = kAudioDevicePropertyDeviceUID,
        };
        constexpr const AudioObjectPropertyAddress propDeviceOutputUID = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioDevicePropertyScopeOutput,
            .mSelector = kAudioDevicePropertyDeviceUID,
        };
        constexpr const AudioObjectPropertyAddress propInputStreams = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioDevicePropertyScopeInput,
            .mSelector = kAudioDevicePropertyStreams,
        };
        constexpr const AudioObjectPropertyAddress propOutputStreams = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioDevicePropertyScopeOutput,
            .mSelector = kAudioDevicePropertyStreams,
        };
        uint32_t outPropDataSize = 0;
        if (getSystemAudioPropertySize(&propDevices, &outPropDataSize))
        {
            const uint32_t numDevices = outPropDataSize / sizeof(AudioObjectID);

            AudioObjectID* const deviceIDs = new AudioObjectID[numDevices];

            if (getSystemAudioProperty(&propDevices, &outPropDataSize, deviceIDs))
            {
                AudioObjectID deviceID = kAudioDeviceUnknown;
                outPropDataSize = sizeof(deviceID);
                if (getSystemAudioProperty(&propDefaultOutputDevice, &outPropDataSize, &deviceID) && deviceID != kAudioDeviceUnknown)
                {
                    DeviceInfo devInfo = { "Default", false, false };

                    outPropDataSize = 0;
                    if (getDeviceAudioPropertySize(deviceID, &propInputStreams, &outPropDataSize) && outPropDataSize != 0)
                    {
                        devInfo.canInput = true;
                    }
                    else
                    {
                        deviceID = kAudioDeviceUnknown;
                        outPropDataSize = sizeof(deviceID);
                        if (getSystemAudioProperty(&propDefaultInputDevice, &outPropDataSize, &deviceID) && deviceID != kAudioDeviceUnknown)
                            devInfo.canInput = true;
                    }

                    ui.cb_device->addItem("Default");
                    devices.append(devInfo);
                }

                for (uint32_t i = 0; i < numDevices; ++i)
                {
                    deviceID = deviceIDs[i];

                    CFStringRef cfs = {};
                    DeviceInfo devInfo = { {}, false, true };
                    QString devName;

                    outPropDataSize = 0;
                    if (getDeviceAudioPropertySize(deviceID, &propInputStreams, &outPropDataSize) && outPropDataSize != 0)
                        devInfo.canInput = true;

                    outPropDataSize = 0;
                    if (getDeviceAudioPropertySize(deviceID, &propOutputStreams, &outPropDataSize) && outPropDataSize != 0)
                    {
                        outPropDataSize = sizeof(cfs);
                        if (getDeviceAudioProperty(deviceID, &propDeviceOutputUID, &outPropDataSize, &cfs))
                            devInfo.uid = QString::fromCFString(cfs);
                        else
                            continue;

                        outPropDataSize = sizeof(cfs);
                        if (getDeviceAudioProperty(deviceID, &propDeviceName, &outPropDataSize, &cfs))
                            devName = QString::fromCFString(cfs);
                        else
                            continue;

                        ui.cb_device->addItem(devName);
                        devices.append(devInfo);
                    }

                    if (devInfo.canInput)
                    {
                        outPropDataSize = sizeof(cfs);
                        if (getDeviceAudioProperty(deviceID, &propDeviceInputUID, &outPropDataSize, &cfs))
                            devInfo.uid = QString::fromCFString(cfs);
                        else
                            continue;

                        outPropDataSize = sizeof(cfs);
                        if (getDeviceAudioProperty(deviceID, &propDeviceName, &outPropDataSize, &cfs))
                            devName = QString::fromCFString(cfs);
                        else
                            continue;

                        devInfo.canInput = true;
                        ui.cb_input->addItem(devName);
                        inputs.append(devInfo.uid);
                    }
                }
            }

            printf("-------------------------------------------------------- AudioObjectGetPropertyDataSize %u\n", numDevices);

            delete[] deviceIDs;
        }
        else
        {
            printf("-------------------------------------------------------- AudioObjectGetPropertyDataSize error\n");
        }
       #else
        if (Pa_Initialize() == paNoError)
        {
            const PaHostApiIndex numHostApis = Pa_GetHostApiCount();

            QStringList apis;
            apis.reserve(numHostApis);

            for (PaHostApiIndex i = 0; i < numHostApis; ++i)
                apis.push_back(Pa_GetHostApiInfo(i)->name);

            const PaDeviceIndex numDevices = Pa_GetDeviceCount();

            for (PaDeviceIndex i = 0; i < numDevices; ++i)
            {
                const PaDeviceInfo* const devInfo = Pa_GetDeviceInfo(i);
                const QString& hostApiName(apis[devInfo->hostApi]);

               #ifdef Q_OS_WIN
                if (hostApiName != "ASIO" && hostApiName != "Windows WASAPI")
                    continue;
               #endif

                const QString devName(QString::fromUtf8(devInfo->name));

                if (devInfo->maxOutputChannels == 0 && devInfo->maxInputChannels == 0)
                    continue;

               #ifndef Q_OS_WIN
                if (devName.contains("Loopback: PCM") || (devName != "pulse" && devName != "system" && ! devName.contains("hw:")))
                    continue;
               #endif

               #ifdef Q_OS_WIN
                const bool canUseSeparateInput = hostApiName == "Windows WASAPI";
               #else
                const bool canUseSeparateInput = devName != "pulse" && devName != "system";
               #endif

                const QString uid(hostApiName + "::" + devName);

                if (devInfo->maxInputChannels > 0 && canUseSeparateInput)
                {
                    ui.cb_input->addItem(uid);
                    inputs.append(uid);
                }

                if (devInfo->maxOutputChannels > 0)
                {
                    ui.cb_device->addItem(hostApiName == "JACK" ? "JACK" : uid);
                    devices.append({
                        uid,
                        devInfo->maxInputChannels > 0,
                        canUseSeparateInput
                    });
                }
            }

            Pa_Terminate();
        }
       #endif

        ui.cb_device->blockSignals(false);

        if (ui.cb_device->count())
            ui.b_start->setEnabled(true);

        printf("--------------------------------------------------------\n");
    }

protected:
    void closeEvent(QCloseEvent* const event) override
    {
        saveSettings();

        if (event->spontaneous() && isVisible() && systray->isVisible() && ui.cb_systray->isChecked())
        {
            QMessageBox::information(this, tr("Systray"),
                                     tr("The program will keep running in the "
                                        "system tray. To terminate the program, "
                                        "choose <b>Quit</b> in the context menu "
                                        "of the system tray entry."));
            hide();
            event->ignore();
        }
        else
        {
            systray->hide();
            QMainWindow::closeEvent(event);
        }
    }

    void timerEvent(QTimerEvent* const event) override
    {
        if (timerId != 0 && event->timerId() == timerId)
        {
            if (startingHost || stoppingHost || processHost.state() != QProcess::NotRunning)
                readHostLog();

            if (startingUI || stoppingUI || processUI.state() != QProcess::NotRunning)
            {
                const QByteArray text = processUI.readAll().trimmed();
                if (! text.isEmpty())
                {
                    ui.text_ui->appendPlainText(text);

                    if (text == "Internal client mod-host successfully loaded")
                        startingUI = false;
                }
            }
        }

        QMainWindow::timerEvent(event);
    }

private:
    void saveSettings()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        QSettings settings;
        settings.setValue("FirstRun", false);
        settings.setValue("AutoStart", ui.cb_autostart->isChecked());
        settings.setValue("CloseToSystray", ui.cb_systray->isChecked());
        settings.setValue("Geometry", saveGeometry());
        settings.setValue("AudioDevice", ui.cb_device->currentText());
        settings.setValue("AudioBufferSize", ui.cb_buffersize->currentText());
        settings.setValue("EnableMIDI", ui.cb_midi->isChecked());
        settings.setValue("PedalboardsMidiChannel", ui.sp_midi_pb->value());
        settings.setValue("SnapshotsMidiChannel", ui.sp_midi_ss->value());
        settings.setValue("SuccessfullyStarted", successfullyStarted);
        settings.setValue("VerboseLogs", ui.cb_verbose_basic->isChecked());
        settings.setValue("VerboseLogsJack", ui.cb_verbose_jackd->isChecked());
        settings.setValue("VerboseLogsHost", ui.cb_verbose_host->isChecked());
        settings.setValue("VerboseLogsUI", ui.cb_verbose_ui->isChecked());
    }

    void loadSettings()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        const QSettings settings;

        const QString audioDevice(settings.value("AudioDevice").toString());
        if (! audioDevice.isEmpty())
        {
            const int index = ui.cb_device->findText(audioDevice);
            if (index >= 0)
            {
                ui.cb_device->blockSignals(true);
                ui.cb_device->setCurrentIndex(index);
                ui.cb_device->blockSignals(false);
            }
        }
        updateDeviceDetails();

        const QString bufferSize(settings.value("AudioBufferSize", "128").toString());
        if (QStringList{"64","128","256"}.contains(bufferSize))
        {
            const int index = ui.cb_buffersize->findText(bufferSize);
            if (index >= 0)
                ui.cb_buffersize->setCurrentIndex(index);
        }
        else
        {
            ui.cb_buffersize->setCurrentIndex(1);
        }

        const bool closeToSystray = settings.value("CloseToSystray", true).toBool();
        ui.cb_systray->setChecked(closeToSystray);

        ui.cb_autostart->setChecked(settings.value("AutoStart", false).toBool());

        ui.cb_midi->setChecked(settings.value("EnableMIDI", true).toBool());
        ui.sp_midi_pb->setValue(settings.value("PedalboardsMidiChannel", 0).toInt());
        ui.sp_midi_pb->setValue(settings.value("SnapshotsMidiChannel", 0).toInt());

        ui.cb_verbose_basic->setChecked(settings.value("VerboseLogs", true).toBool());
        ui.cb_verbose_jackd->setChecked(settings.value("VerboseLogsJack", false).toBool());
        ui.cb_verbose_host->setChecked(settings.value("VerboseLogsHost", false).toBool());
        ui.cb_verbose_ui->setChecked(settings.value("VerboseLogsUI", false).toBool());

        successfullyStarted = settings.value("SuccessfullyStarted", false).toBool();

        adjustSize();

        if (settings.contains("Geometry"))
            restoreGeometry(settings.value("Geometry").toByteArray());

        if (settings.value("FirstRun", true).toBool() || !ui.cb_autostart->isChecked() || !successfullyStarted)
        {
            setStopped();
            QTimer::singleShot(0, this, &QMainWindow::show);
        }
        else
        {
            QTimer::singleShot(100, this, &AppWindow::start);
        }

        QTimer::singleShot(1, systray, &QSystemTrayIcon::show);
    }

    void setStarting()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        startingHost = true;
        ui.b_start->setEnabled(false);
        ui.b_stop->setEnabled(true);
        ui.gb_audio->setEnabled(false);
        ui.gb_midi->setEnabled(false);
        ui.gb_lv2->setEnabled(false);
    }

    void setRunning()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        successfullyStarted = true;
        ui.b_opengui->setEnabled(true);
        systray->setToolTip(tr("MOD App: Running"));
        systray->showMessage(tr("MOD App"), tr("Running"), QSystemTrayIcon::Information);
    }

    void setStopped()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        ui.b_start->setEnabled(true);
        ui.b_stop->setEnabled(false);
        ui.b_opengui->setEnabled(false);
        ui.gb_audio->setEnabled(true);
        ui.gb_midi->setEnabled(true);
        ui.gb_lv2->setEnabled(true);
        systray->setToolTip(tr("MOD App: Stopped"));
    }

    QString getLV2Path() const
    {
       #ifdef _WIN32
        WCHAR path[MAX_PATH + 256] = {};
        DWORD pathlen = GetEnvironmentVariableW(L"MOD_LV2_PATH", path, sizeof(path)/sizeof(path[0]));
        DWORD pathmax = pathlen;
       #else
        char path[PATH_MAX + 256] = {};
        std::strcpy(path, std::getenv("MOD_LV2_PATH"));
       #endif

        if (ui.cb_lv2_all_plugins->isChecked())
        {
           #if defined(__APPLE__)
            std::strcat(path, ":~/Library/Audio/Plug-Ins/LV2:/Library/Audio/Plug-Ins/LV2");
           #elif defined(_WIN32)
            pathlen = GetEnvironmentVariableW(L"LOCALAPPDATA", path + pathmax + 1, sizeof(path)/sizeof(path[0]) - pathmax - 1);
            if (pathlen == pathmax)
                pathlen = GetEnvironmentVariableW(L"APPDATA", path + pathmax + 1, sizeof(path)/sizeof(path[0]) - pathmax - 1);
            if (pathlen != pathmax)
            {
                path[pathmax] = L';';
                pathmax += pathlen;
                path[pathmax++] = L'\\';
                path[pathmax++] = L'L';
                path[pathmax++] = L'V';
                path[pathmax++] = L'2';
                path[pathmax] = 0;
            }

            pathlen = GetEnvironmentVariableW(L"COMMONPROGRAMFILES", path + pathmax + 1, sizeof(path)/sizeof(path[0]) - pathmax - 1);
            if (pathlen != pathmax)
            {
                path[pathmax] = L';';
                pathmax += pathlen;
                path[pathmax++] = L'\\';
                path[pathmax++] = L'L';
                path[pathmax++] = L'V';
                path[pathmax++] = L'2';
                path[pathmax] = 0;
            }
           #else
            if (const char* const env = std::getenv("LV2_PATH"))
            {
                std::strcat(path, ":");
                std::strcat(path, env);
            }
            else
            {
                std::strcat(path, ":~/.lv2:/usr/local/lib/lv2:/usr/lib/lv2");
            }
           #endif
        }

       #ifdef _WIN32
        return QString::fromWCharArray(path);
       #else
        return QString::fromUtf8(path);
       #endif
    }

    QString getProcessErrorAsString(QProcess::ProcessError error)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        if (error == QProcess::FailedToStart)
            return tr("Process failed to start.");
        if (error == QProcess::Crashed)
            return tr("Process crashed.");
        if (error == QProcess::Timedout)
            return tr("Process timed out.");
        if (error == QProcess::WriteError)
            return tr("Process write error.");
        return tr("Unkown error.");
    }

    void showErrorMessage(const QString& message)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        if (isVisible() || !QSystemTrayIcon::supportsMessages())
            QMessageBox::critical(nullptr, tr("Error"), message);
        else
            systray->showMessage(tr("Error"), message, QSystemTrayIcon::Critical);
    }

    void readHostLog()
    {
        const QByteArray text = processHost.readAll().trimmed();

        if (text.isEmpty())
            return;

        ui.text_host->appendPlainText(text);

        if (text.contains("Internal client mod-host successfully loaded"))
        {
            startingHost = false;
            startingUI = true;

            QProcessEnvironment env(QProcessEnvironment::systemEnvironment());

            env.insert("LV2_PATH", getLV2Path());

            if (ui.cb_verbose_ui->isChecked() && ui.cb_verbose_ui->isEnabled())
                env.insert("MOD_LOG", "2");
            else if (ui.cb_verbose_basic->isChecked())
                env.insert("MOD_LOG", "1");
            else
                env.insert("MOD_LOG", "0");
            
            if (ui.cb_lv2_all_cv->isChecked())
                env.insert("MOD_UI_ALLOW_REGULAR_CV", "1");

            processUI.setProcessEnvironment(env);
            processUI.start();
        }
    }

    void stopHostIfNeeded()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        if (processHost.state() == QProcess::NotRunning)
            return;

        stoppingHost = true;
        processHost.terminate();
    }

    void stopUIIfNeeded()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        if (processUI.state() == QProcess::NotRunning)
            return;

        stoppingUI = true;
        processUI.terminate();
    }

    void close()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);

        saveSettings();

        if (timerId != 0)
        {
            killTimer(timerId);
            timerId = 0;
        }

        if (processUI.state() != QProcess::NotRunning)
        {
            stoppingUI = true;
            processUI.terminate();
        }

        if (processHost.state() != QProcess::NotRunning)
        {
            stoppingHost = true;
            processHost.terminate();
        }
    }

private slots:
    void start()
    {
        saveSettings();

        successfullyStarted = false;

        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        if (processHost.state() != QProcess::NotRunning)
        {
            printf("----------- %s %d start ignored\n", __FUNCTION__, __LINE__);
            return;
        }

        ui.text_host->clear();
        ui.text_ui->clear();

        writeMidiChannelsToProfile(ui.sp_midi_pb->value(), ui.sp_midi_ss->value());

        const bool midiEnabled = ui.cb_midi->isChecked();
        const int deviceIndex = ui.cb_device->currentIndex();

        if (deviceIndex < 0 || deviceIndex >= devices.size())
            return;

        const DeviceInfo& devInfo(devices[deviceIndex]);

        QStringList arguments = {
            "-R",
            "-S",
            "-n",
            "mod-app",
        };

        if (midiEnabled)
        {
           #if defined(Q_OS_MAC)
            arguments.append("-X");
            arguments.append("coremidi");
           #elif defined(Q_OS_WIN)
            arguments.append("-X");
            arguments.append("winmme");
           #endif
        }

        arguments.append("-C");
       #if defined(Q_OS_WIN)
        arguments.append(".\\jack\\jack-session.conf");
       #elif defined(Q_OS_MAC)
        arguments.append("./jack/jack-session.conf");
       #else
        arguments.append(midiEnabled ? "./jack/jack-session-alsamidi.conf" : "./jack/jack-session.conf");
       #endif

        if (ui.cb_verbose_jackd->isChecked() && ui.cb_verbose_jackd->isEnabled())
            arguments.append("-v");

        arguments.append("-d");
       #ifdef Q_OS_MAC
        arguments.append("coreaudio");
       #else
        arguments.append("portaudio");
       #endif

        arguments.append("-r");
        arguments.append("48000");

        arguments.append("-p");
        arguments.append(ui.cb_buffersize->currentText());

        // regular duplex
        if (ui.rb_device_duplex->isChecked())
        {
           #ifdef Q_OS_MAC
            if (ui.cb_device->currentIndex() != 0)
           #endif
            {
                arguments.append("-d");
                arguments.append(devInfo.uid);
            }
        }
        // split duplex
        else if (ui.rb_device_separate->isChecked())
        {
            arguments.append("-P");
            arguments.append(devInfo.uid);
            arguments.append("-C");
            arguments.append(inputs[ui.cb_input->currentIndex()]);
        }
        // playback only
        else
        {
            arguments.append("-P");
            arguments.append(devInfo.uid);
        }

       #if !(defined(Q_OS_WIN) || defined(Q_OS_MAC))
        if (devInfo.uid == "ALSA::pulse")
        {
            arguments.append("-c");
            arguments.append("2");
        }
       #endif

        processHost.setArguments(arguments);

        {
            QProcessEnvironment env(QProcessEnvironment::systemEnvironment());

            env.insert("JACK_NO_AUDIO_RESERVATION", "1");
            env.insert("JACK_NO_START_SERVER", "1");
            env.insert("LV2_PATH", getLV2Path());

            if (ui.cb_verbose_host->isChecked() && ui.cb_verbose_host->isEnabled())
                env.insert("MOD_LOG", "1");
            else
                env.insert("MOD_LOG", "0");

            processHost.setProcessEnvironment(env);
        }

        ui.text_host->appendPlainText("Starting jackd using:");
        ui.text_host->appendPlainText(arguments.join(" "));

        setStarting();
        processHost.start();
    }

    void stop()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        ui.b_stop->setEnabled(false);

        stopUIIfNeeded();
        stopHostIfNeeded();
    }

    void openGui() const
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        QDesktopServices::openUrl(QUrl("http://127.0.0.1:18181"));
    }

    void openUserFilesDir() const
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        QDesktopServices::openUrl(QUrl::fromLocalFile(getUserFilesDir()));
    }

    void updateDeviceDetails()
    {
        const int deviceIndex = ui.cb_device->currentIndex();
        printf("----------- %s %d | %d %d\n", __FUNCTION__, __LINE__, deviceIndex, (int)devices.size());

        if (deviceIndex < 0 || deviceIndex >= devices.size())
            return;

        const DeviceInfo& devInfo(devices[deviceIndex]);

        ui.rb_device_duplex->setEnabled(devInfo.canInput);
        ui.rb_device_separate->setEnabled(devInfo.canUseSeparateInput);
        ui.rb_device_noinput->setEnabled(true);

        if (! (ui.rb_device_duplex->isChecked() || ui.rb_device_separate->isChecked() || ui.rb_device_noinput->isChecked()))
            ui.rb_device_duplex->setChecked(true);
        else if (devInfo.canInput && ! devInfo.canUseSeparateInput)
            ui.rb_device_duplex->setChecked(true);

        if (ui.rb_device_duplex->isChecked() && ! ui.rb_device_duplex->isEnabled())
            ui.rb_device_separate->setChecked(true);

        if (ui.rb_device_separate->isChecked() && ! ui.rb_device_separate->isEnabled())
            ui.rb_device_noinput->setChecked(true);
    }

    void hostStartError(QProcess::ProcessError error)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        readHostLog();

        // crashed while stopping, ignore
        if (error == QProcess::Crashed && stoppingHost)
            return;

        if (processUI.state() != QProcess::NotRunning)
        {
            stoppingUI = true;
            processUI.terminate();
        }

        setStopped();
        showErrorMessage(tr("Could not start MOD Host.\n") + getProcessErrorAsString(error));
    }

    void uiStartError(QProcess::ProcessError error)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        stopHostIfNeeded();

        // crashed while stopping, ignore
        if (error == QProcess::Crashed && stoppingUI)
            return;

        if (processHost.state() != QProcess::NotRunning)
        {
            stoppingHost = true;
            processHost.terminate();
        }

        setStopped();
        showErrorMessage(tr("Could not start MOD UI.\n") + getProcessErrorAsString(error));
    }

    void hostStartSuccess()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        readHostLog();
    }

    void uiStartSuccess()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        startingUI = false;
        setRunning();
    }

    void hostFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        startingHost = stoppingHost = false;
        readHostLog();
        stopUIIfNeeded();
        setStopped();
    }

    void uiFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        startingUI = stoppingUI = false;
        stopHostIfNeeded();
        setStopped();
    }

    void iconActivated(QSystemTrayIcon::ActivationReason reason)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        switch (reason)
        {
        case QSystemTrayIcon::DoubleClick:
        case QSystemTrayIcon::MiddleClick:
           #ifdef Q_OS_MAC
            show();
           #else
            setVisible(!isVisible());
           #endif
            break;
        default:
            break;
        }
    }

    void messageClicked()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
    }
};
