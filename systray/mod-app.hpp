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
#endif

QString getUserFilesDir();
void writeMidiChannelsToProfile(int pedalboard, int snapshot);

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
        QString uidMain;
        QString uidInput;
        bool canInput = false;
        bool canOutput = false;
    };
    QList<DeviceInfo> devices;

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
        connect(ui.gb_logs, &QGroupBox::toggled, this, &AppWindow::showLogs);

        const QIcon icon(":/mod-logo.svg");

        openGuiAction = new QAction(tr("&Open GUI"), this);
        connect(openGuiAction, &QAction::triggered, this, &AppWindow::openGui);

        openUserFilesAction = new QAction(tr("&Open User Files"), this);
        connect(openUserFilesAction, &QAction::triggered, this, &AppWindow::openUserFilesDir);

        settingsAction = new QAction(tr("&Settings"), this);
        connect(settingsAction, &QAction::triggered, this, &QMainWindow::show);

        quitAction = new QAction(tr("&Quit"), this);
        connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

        sysmenu = new QMenu(this);
        sysmenu->addAction(openGuiAction);
        sysmenu->addAction(openUserFilesAction);
        sysmenu->addAction(settingsAction);
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
       #else
        setenv("JACK_NO_AUDIO_RESERVATION", "1", 1);
        setenv("JACK_NO_START_SERVER", "1", 1);
       #endif

       #ifdef Q_OS_MAC
        constexpr const AudioObjectPropertyAddress propDevices = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioObjectPropertyScopeGlobal,
            .mSelector = kAudioHardwarePropertyDevices,
        };
        constexpr const AudioObjectPropertyAddress propDefaultInDevice = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioObjectPropertyScopeGlobal,
            .mSelector = kAudioHardwarePropertyDefaultInputDevice,
        };
        constexpr const AudioObjectPropertyAddress propDefaultOutDevice = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioObjectPropertyScopeGlobal,
            .mSelector = kAudioHardwarePropertyDefaultOutputDevice,
        };
        constexpr const AudioObjectPropertyAddress propDeviceName = {
            .mElement  = kAudioObjectPropertyElementMaster,
            .mScope    = kAudioObjectPropertyScopeGlobal,
            .mSelector = kAudioDevicePropertyDeviceNameCFString,
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
        if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propDevices, 0, nullptr, &outPropDataSize) == kAudioHardwareNoError)
        {
			const uint32_t numDevices = outPropDataSize / sizeof(AudioObjectID);

            AudioObjectID* const deviceIDs = new AudioObjectID[numDevices];

            if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &propDevices, 0, nullptr, &outPropDataSize, deviceIDs) == kAudioHardwareNoError)
            {
                AudioObjectID deviceID = kAudioDeviceUnknown;
                outPropDataSize = sizeof(deviceID);
                if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &propDefaultOutDevice, 0, nullptr, &outPropDataSize, &deviceID) == kAudioHardwareNoError && deviceID != kAudioDeviceUnknown)
                {
                    DeviceInfo devInfo = { QString::number(deviceID), {}, false, true };

                    outPropDataSize = 0;
                    if (AudioObjectGetPropertyDataSize(deviceID, &propInputStreams, 0, nullptr, &outPropDataSize) == kAudioHardwareNoError && outPropDataSize != 0)
                    {
                        devInfo.canInput = true;
                    }
                    else
                    {
                        deviceID = kAudioDeviceUnknown;
                        outPropDataSize = sizeof(deviceID);
                        if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &propDefaultInDevice, 0, nullptr, &outPropDataSize, &deviceID) == kAudioHardwareNoError && deviceID != kAudioDeviceUnknown)
                        {
                            devInfo.uidInput = QString::number(deviceID);
                            devInfo.canInput = true;
                        }
                    }

                    ui.cb_device->addItem("Default");
                    devices.append(devInfo);
                }

                for (uint32_t i = 0; i < numDevices; ++i)
                {
                    deviceID = deviceIDs[i];

                    DeviceInfo devInfo = { QString::number(deviceID), {}, false, false };
                    QString devName;

                    CFStringRef devNameCF = {};
                    outPropDataSize = sizeof(devNameCF);
                    if (AudioObjectGetPropertyData(deviceID, &propDeviceName, 0, nullptr, &outPropDataSize, &devNameCF) == kAudioHardwareNoError)
                        devName = QString::fromCFString(devNameCF);
                    else
                        devName = devInfo.uidMain;

                    outPropDataSize = 0;
                    if (AudioObjectGetPropertyDataSize(deviceID, &propInputStreams, 0, nullptr, &outPropDataSize) == kAudioHardwareNoError && outPropDataSize != 0)
                        devInfo.canInput = true;

                    outPropDataSize = 0;
                    if (AudioObjectGetPropertyDataSize(deviceID, &propOutputStreams, 0, nullptr, &outPropDataSize) == kAudioHardwareNoError && outPropDataSize != 0)
                        devInfo.canOutput = true;

                    // TODO separate out vs in combo-boxes
                    if (! devInfo.canOutput) continue;

                    ui.cb_device->addItem(devName);
                    devices.append(devInfo);
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
            QString matchingInputDevName;

            for (PaHostApiIndex i = 0; i < numHostApis; ++i)
                apis.push_back(Pa_GetHostApiInfo(i)->name);

            const PaDeviceIndex numDevices = Pa_GetDeviceCount();

            for (PaDeviceIndex i = 0; i < numDevices; ++i)
            {
                const PaDeviceInfo* const devInfo = Pa_GetDeviceInfo(i);
                const QString& hostApiName(apis[devInfo->hostApi]);

               #ifdef Q_OS_WIN
                if (hostApiName != "ASIO" && hostApiName != "Windows WASAPI")
                {
                    matchingInputDevName.clear();
                    continue;
                }

                if (hostApiName == "Windows WASAPI")
                {
                    printf("DEBUG WASAPI %d %s %d %d\n",
                           i, devInfo->name, devInfo->maxInputChannels, devInfo->maxOutputChannels);
                }
               #endif

                const QString devName(QString::fromUtf8(devInfo->name));

                if (devInfo->maxOutputChannels == 0)
                {
                    if (devInfo->maxInputChannels == 0)
                        continue;

                   #ifdef Q_OS_WIN
                    if (hostApiName == "Windows WASAPI")
                        matchingInputDevName = devName;
                    else
                        matchingInputDevName.clear();
                   #endif

                    continue;
                }

               #ifndef Q_OS_WIN
                if (devName.contains("Loopback: PCM") || (devName != "pulse" && ! devName.contains("hw:")))
                    continue;
               #endif

                const QString uidMain(hostApiName + "::" + devName);
                const QString uidInput(devInfo->maxInputChannels == 0 && !matchingInputDevName.isEmpty()
                    ? hostApiName + "::" + matchingInputDevName
                    : "");
                matchingInputDevName.clear();

                ui.cb_device->addItem(fullName);
                devices.append({
                    uidMain, uidInput,
                    devInfo->maxInputChannels > 0,
                    devInfo->maxInputChannels > 0 || !uidInput.isEmpty()
                });
            }

            Pa_Terminate();
        }
       #endif

        if (ui.cb_device->count())
            ui.b_start->setEnabled(true);

        printf("--------------------------------------------------------\n");
    }

protected:
    void closeEvent(QCloseEvent* const event) override
    {
        saveSettings();

        if (event->spontaneous() && isVisible() && systray->isVisible())
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
            close();
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
        settings.setValue("Geometry", saveGeometry());
        settings.setValue("AudioDevice", ui.cb_device->currentText());
        settings.setValue("AudioBufferSize", ui.cb_buffersize->currentText());
        settings.setValue("EnableMIDI", ui.gb_midi->isChecked());
        settings.setValue("PedalboardsMidiChannel", ui.sp_midi_pb->value());
        settings.setValue("SnapshotsMidiChannel", ui.sp_midi_ss->value());
        settings.setValue("ShowLogs", ui.gb_logs->isChecked());
        settings.setValue("SuccessfullyStarted", successfullyStarted);
    }

    void loadSettings()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        const QSettings settings;

        const bool logsEnabled = settings.value("ShowLogs", false).toBool();
        ui.gb_logs->setChecked(logsEnabled);
        ui.tab_logs->setEnabled(logsEnabled);
        ui.tab_logs->setVisible(logsEnabled);

        const QString audioDevice(settings.value("AudioDevice").toString());
        if (! audioDevice.isEmpty())
        {
            const int index = ui.cb_device->findText(audioDevice);
            if (index >= 0)
                ui.cb_device->setCurrentIndex(index);
        }

        ui.cb_buffersize->setCurrentIndex(settings.value("AudioBufferSize", "128").toString() == "256" ? 1 : 0);
        ui.cb_autostart->setChecked(settings.value("AutoStart", false).toBool());

        ui.gb_midi->setChecked(settings.value("EnableMIDI", false).toBool());
        ui.sp_midi_pb->setValue(settings.value("PedalboardsMidiChannel", 0).toInt());
        ui.sp_midi_pb->setValue(settings.value("SnapshotsMidiChannel", 0).toInt());

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

    void setRunning()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        successfullyStarted = true;
        ui.cb_device->setEnabled(false);
        ui.cb_buffersize->setEnabled(false);
        ui.b_start->setEnabled(false);
        ui.b_stop->setEnabled(true);
        ui.b_opengui->setEnabled(true);
        ui.gb_midi->setEnabled(true);
        systray->setToolTip(tr("MOD App: Running"));
        systray->showMessage(tr("MOD App"), tr("Running"), QSystemTrayIcon::Information);
    }

    void setStopped()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        ui.cb_device->setEnabled(true);
        ui.cb_buffersize->setEnabled(true);
        ui.b_start->setEnabled(true);
        ui.b_stop->setEnabled(false);
        ui.b_opengui->setEnabled(false);
        ui.gb_midi->setEnabled(true);
        systray->setToolTip(tr("MOD App: Stopped"));
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
            QTimer::singleShot(1000, &processUI, &AppProcess::startSlot);
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

        const bool midiEnabled = ui.gb_midi->isChecked();
        const int deviceIndex = ui.cb_device->currentIndex();

        if (deviceIndex < 0 || deviceIndex >= devices.size())
            return;

        const DeviceInfo& devInfo(devices[deviceIndex]);

        QStringList arguments = {
            "-R",
            "-S",
            "-C",
           #ifdef Q_OS_WIN
            ".\\jack\\jack-session.conf",
           #else
            "./jack/jack-session.conf",
           #endif
        };

        if (midiEnabled)
        {
            arguments.append("-X");
           #if defined(Q_OS_WIN)
            arguments.append("winmme");
           #elif defined(Q_OS_MAC)
            arguments.append("coremidi");
           #else
            arguments.append("alsarawmidi");
           #endif
        }

        arguments.append("-d");
       #ifdef Q_OS_MAC
        arguments.append("coreaudio");
       #else
        arguments.append("portaudio");
       #endif

        arguments.append("-r");
        arguments.append("48000");

        arguments.append("-p");
        arguments.append(ui.cb_buffersize->currentIndex() == 0 ? "128" : "256");


        // forced duplex
        if (! devInfo.uidInput.isEmpty())
        {
            arguments.append("-P");
            arguments.append(devInfo.uidMain);
            arguments.append("-C");
            arguments.append(devInfo.uidInput);
        }
        else
        {
            if (devInfo.canInput && devInfo.canOutput)
            {
                // regular device duplex mode
                arguments.append("-d");
            }
            else if (devInfo.canInput)
            {
                // capture only
                arguments.append("-C");
            }
            else
            {
                // playback only
                arguments.append("-P");
            }

            arguments.append(devInfo.uidMain);
        }

       #if 0 // !(defined(Q_OS_WIN) || defined(Q_OS_MAC))
        if (! midiEnabled)
        {
            arguments.append("-X");
            arguments.append("seqmidi");
        }
       #endif

        processHost.setArguments(arguments);

        ui.text_host->appendPlainText("Starting jackd using:");
        ui.text_host->appendPlainText(arguments.join(" "));

        ui.b_start->setEnabled(false);

        startingHost = true;
        processHost.start();
    }

    void stop()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        ui.b_stop->setEnabled(false);

        stopUIIfNeeded();
        stopHostIfNeeded();

        ui.b_stop->setEnabled(true);
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
            setVisible(!isVisible());
            break;
        default:
            break;
        }
    }

    void messageClicked()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
    }

    void showLogs(bool toggled)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        ui.tab_logs->setEnabled(toggled);
        ui.tab_logs->setVisible(toggled);

        if (toggled)
        {
            ui.tab_logs->resize(1, heightLogs);
            resize(width(), height() + heightLogs);
        }
        else
        {
            // heightLogs = ui.tab_logs->height();
            resize(width(), height() - heightLogs);
            adjustSize();
        }
    }
};
