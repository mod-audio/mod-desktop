// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "ui_mod-desktop.hpp"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSystemTrayIcon>

#ifdef Q_OS_LINUX
#include <alsa/asoundlib.h>
#endif

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

static bool isdigit(const char* const s)
{
    const size_t len = strlen(s);

    if (len == 0)
        return false;

    for (size_t i=0; i<len; ++i)
    {
        if (std::isdigit(s[i]))
            continue;
        return false;
    }

    return true;
}

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

    const QString cwd;
    AppProcess processHost;
    AppProcess processUI;
    bool startingHost = false;
    bool startingUI = false;
    bool stoppingHost = false;
    bool stoppingUI = false;
    bool successfullyStarted = false;
    int timerId = 0;

   #ifdef Q_OS_WIN
    HANDLE openEvent = nullptr;
   #endif

    struct DeviceInfo {
        QString uid;
        bool canInput = false;
        bool canUseSeparateInput = false;
    };

    enum DeviceInputMode {
        kDeviceModeDuplex = 0,
        kDeviceModeSeparated,
        kDeviceModeNoInput,
    };

    QList<DeviceInfo> devices;
    QStringList inputs;

public:
    AppWindow()
        : cwd(QDir::currentPath()),
          processHost(this, cwd),
          processUI(this, cwd)
    {
        ui.setupUi(this);

        ui.l_version->setText(VERSION);
        ui.gb_audio->setup(ui.tb_audio);
        ui.gb_midi->setup(ui.tb_midi);
        ui.gb_lv2->setup(ui.tb_lv2);
        ui.gb_gui->setup(ui.tb_gui);

       #ifdef Q_OS_LINUX
        ui.l_midi_warn->hide();
       #endif

        connect(ui.b_start, &QPushButton::clicked, this, &AppWindow::start);
        connect(ui.b_stop, &QPushButton::clicked, this, &AppWindow::stop);
        connect(ui.b_opengui, &QPushButton::clicked, this, &AppWindow::openGui);
        connect(ui.b_openuserfiles, &QPushButton::clicked, this, &AppWindow::openUserFilesDir);
        connect(ui.cb_device, &QComboBox::currentTextChanged, this, &AppWindow::updateDeviceDetails);
        connect(ui.cb_verbose_basic, &QCheckBox::toggled, this, &AppWindow::showLogs);

        const QIcon icon(":/res/mod-logo.svg");

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
        systray->setIcon(QPixmap(":/res/mod-logo.svg"));
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
        connect(&processHost, &QProcess::readyReadStandardOutput, this, &AppWindow::hostReadStdOut);

        connect(&processUI, &QProcess::errorOccurred, this, &AppWindow::uiStartError);
        connect(&processUI, &QProcess::started, this, &AppWindow::uiStartSuccess);
        connect(&processUI, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &AppWindow::uiFinished);
        connect(&processUI, &QProcess::readyReadStandardOutput, this, &AppWindow::uiReadStdOut);

        timerId = startTimer(500);

       #ifdef Q_OS_WIN
        openEvent = ::CreateEventA(nullptr, false, false, "Global\\mod-desktop-open");
       #endif
    }

    ~AppWindow()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);

       #ifdef Q_OS_WIN
        if (openEvent != nullptr)
            CloseHandle(openEvent);
       #endif

        close();
    }

    void fillInDeviceList()
    {
        printf("--------------------------------------------------------\n");

        ui.cb_device->blockSignals(true);

       #if defined(Q_OS_LINUX)
        {
            char hwcard[32];
            char reserve[32];

            snd_ctl_t* ctl = nullptr;
            snd_ctl_card_info_t* cardinfo = nullptr;
            snd_pcm_info_t* pcminfo = nullptr;
            snd_ctl_card_info_alloca(&cardinfo);
            snd_pcm_info_alloca(&pcminfo);

            for (int card = -1; snd_card_next(&card) == 0 && card >= 0;)
            {
                snprintf(hwcard, sizeof(hwcard), "hw:%i", card);

                if (snd_ctl_open(&ctl, hwcard, SND_CTL_NONBLOCK) < 0)
                    continue;

                if (snd_ctl_card_info(ctl, cardinfo) >= 0)
                {
                    const char* cardId = snd_ctl_card_info_get_id(cardinfo);
                    const char* cardName = snd_ctl_card_info_get_name(cardinfo);

                    if (cardName != nullptr && *cardName != '\0')
                    {
                        if (std::strcmp(cardName, "Dummy") == 0 ||
                            std::strcmp(cardName, "Loopback") == 0)
                        {
                            snd_ctl_close(ctl);
                            continue;
                        }
                    }

                    if (cardId == nullptr || ::isdigit(cardId))
                    {
                        snprintf(reserve, sizeof(reserve), "%d", card);
                        cardId = reserve;
                    }

                    if (cardName == nullptr || *cardName == '\0')
                        cardName = cardId;

                    for (int device = -1; snd_ctl_pcm_next_device(ctl, &device) == 0 && device >= 0;)
                    {
                        snd_pcm_info_set_device(pcminfo, device);

                        for (int subDevice = 0, nbSubDevice = 1; subDevice < nbSubDevice; ++subDevice)
                        {
                            snd_pcm_info_set_subdevice(pcminfo, subDevice);

                            snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_CAPTURE);
                            const bool isInput = (snd_ctl_pcm_info(ctl, pcminfo) >= 0);

                            snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
                            const bool isOutput = (snd_ctl_pcm_info(ctl, pcminfo) >= 0);

                            if (! (isInput || isOutput))
                                continue;

                            if (nbSubDevice == 1)
                                nbSubDevice = snd_pcm_info_get_subdevices_count(pcminfo);

                            QString strid(QString::fromUtf8(hwcard));
                            QString strname(QString::fromUtf8(cardName));

                            strid += ",";
                            strid += QString::number(device);

                            if (const char* const pcmName = snd_pcm_info_get_name(pcminfo))
                            {
                                if (pcmName[0] != '\0')
                                {
                                    strname += ", ";
                                    strname += QString::fromUtf8(pcmName);
                                }
                            }

                            if (nbSubDevice != 1)
                            {
                                strid += ",";
                                strid += QString::number(subDevice);
                                strname += " {";
                                strname += QString::fromUtf8(snd_pcm_info_get_subdevice_name(pcminfo));
                                strname += "}";
                            }

                            strname += " (";
                            strname += strid;
                            strname += ")";

                            if (isInput)
                            {
                                ui.cb_input->addItem(strname);
                                inputs.append(strid);
                            }

                            if (isOutput)
                            {
                                ui.cb_device->addItem(strname);
                                devices.append({
                                    strid,
                                    isInput,
                                    true
                                });
                            }
                        }
                    }
                }

                snd_ctl_close(ctl);
            }
        }
       #elif defined(Q_OS_MAC)
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
       #endif

       #ifndef Q_OS_MAC
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

                QString devName(QString::fromUtf8(devInfo->name));

               #if defined(Q_OS_LINUX)
                if (hostApiName == "ALSA")
                    continue;
               #elif defined(Q_OS_WIN)
                if (hostApiName == "ASIO")
                {
                    if (devName == "JackRouter" || devName == "MOD Desktop")
                        continue;
                }
                else if (hostApiName != "Windows WASAPI")
                {
                    continue;
                }
               #endif

                if (devInfo->maxOutputChannels == 0 && devInfo->maxInputChannels == 0)
                    continue;

               #ifdef Q_OS_WIN
                const bool canUseSeparateInput = hostApiName == "Windows WASAPI";
               #else
                const bool canUseSeparateInput = false;
               #endif

                const QString uid(hostApiName + "::" + devName);

                if (hostApiName == "JACK" || hostApiName == "JACK Audio Connection Kit")
                    devName = "JACK / PipeWire";
                else if (hostApiName != "Windows WASAPI")
                    devName = uid;

                if (devInfo->maxInputChannels > 0 && canUseSeparateInput)
                {
                    ui.cb_input->addItem(devName);
                    inputs.append(uid);
                }

                if (devInfo->maxOutputChannels > 0)
                {
                    ui.cb_device->addItem(devName);

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
           #ifdef Q_OS_WIN
            if (openEvent != nullptr)
            {
                if (WaitForSingleObject(openEvent, 0) == WAIT_OBJECT_0)
                {
                    show();
                    activateWindow();
                }
            }
           #endif
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
        settings.setValue("AudioInputDevice", ui.cb_input->currentText());
        settings.setValue("AudioBufferSize", ui.cb_buffersize->currentText());
        settings.setValue("EnableMIDI", ui.cb_midi->isChecked());
        settings.setValue("PedalboardsMidiChannel", ui.sp_midi_pb->value());
        settings.setValue("SnapshotsMidiChannel", ui.sp_midi_ss->value());
        settings.setValue("SuccessfullyStarted", successfullyStarted);
        settings.setValue("ExpandedOptionsAudio", ui.tb_audio->isChecked());
        settings.setValue("ExpandedOptionsMIDI", ui.tb_midi->isChecked());
        settings.setValue("ExpandedOptionsLV2", ui.tb_lv2->isChecked());
        settings.setValue("ExpandedOptionsGUI", ui.tb_gui->isChecked());
        settings.setValue("VerboseLogs", ui.cb_verbose_basic->isChecked());
        settings.setValue("VerboseLogsJack", ui.cb_verbose_jackd->isChecked());
        settings.setValue("VerboseLogsHost", ui.cb_verbose_host->isChecked());
        settings.setValue("VerboseLogsUI", ui.cb_verbose_ui->isChecked());

        settings.setValue("AudioInputMode",
                          static_cast<int>(ui.rb_device_separate->isChecked() ? kDeviceModeSeparated :
                                           ui.rb_device_noinput->isChecked() ? kDeviceModeNoInput :
                                           kDeviceModeDuplex));
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

        const QString audioInputDevice(settings.value("AudioInputDevice").toString());
        if (! audioInputDevice.isEmpty())
        {
            const int index = ui.cb_input->findText(audioInputDevice);
            if (index >= 0)
                ui.cb_input->setCurrentIndex(index);
        }

        switch (settings.value("AudioInputMode", kDeviceModeDuplex).toInt())
        {
        case kDeviceModeSeparated:
            ui.rb_device_separate->setEnabled(true);
            ui.rb_device_separate->setChecked(true);
            break;
        case kDeviceModeNoInput:
            ui.rb_device_noinput->setEnabled(true);
            ui.rb_device_noinput->setChecked(true);
            break;
        default:
            ui.rb_device_duplex->setEnabled(true);
            ui.rb_device_duplex->setChecked(true);
            break;
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
        ui.sp_midi_ss->setValue(settings.value("SnapshotsMidiChannel", 0).toInt());

        const bool verboseLogs = settings.value("VerboseLogs", false).toBool();
        ui.gb_logs->setVisible(verboseLogs);
        ui.cb_verbose_basic->setChecked(verboseLogs);
        ui.cb_verbose_jackd->setChecked(settings.value("VerboseLogsJack", false).toBool());
        ui.cb_verbose_host->setChecked(settings.value("VerboseLogsHost", false).toBool());
        ui.cb_verbose_ui->setChecked(settings.value("VerboseLogsUI", false).toBool());

        ui.gb_audio->setCheckedInit(settings.value("ExpandedOptionsAudio", false).toBool());
        ui.gb_midi->setCheckedInit(settings.value("ExpandedOptionsMIDI", false).toBool());
        ui.gb_lv2->setCheckedInit(settings.value("ExpandedOptionsLV2", false).toBool());
        ui.gb_gui->setCheckedInit(settings.value("ExpandedOptionsGUI", false).toBool());

        successfullyStarted = settings.value("SuccessfullyStarted", false).toBool();

        if (settings.contains("Geometry"))
            restoreGeometry(settings.value("Geometry").toByteArray());
        else
            adjustSize();

        if (settings.value("FirstRun", true).toBool() || !ui.cb_autostart->isChecked() || !successfullyStarted)
        {
            setStopped();
            QTimer::singleShot(0, this, &QMainWindow::show);
        }
        else
        {
            QTimer::singleShot(100, this, &AppWindow::start);
        }

        if (QSystemTrayIcon::isSystemTrayAvailable())
        {
            QTimer::singleShot(1, systray, &QSystemTrayIcon::show);
        }
        else
        {
            ui.cb_systray->setChecked(false);
            ui.cb_systray->hide();
        }
    }

    void setStarting()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        startingHost = true;
        ui.b_start->setEnabled(false);
        ui.b_stop->setEnabled(true);
        ui.cb_device->setEnabled(false);
        ui.l_device->setEnabled(false);
        ui.gb_audio->setEnabled(false);
        ui.gb_midi->setEnabled(false);
        ui.gb_lv2->setEnabled(false);
        ui.l_status->setText(tr("Starting..."));
    }

    void setRunning()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        successfullyStarted = true;
        ui.b_opengui->setEnabled(true);
        ui.l_status->setText(tr("Running"));
        systray->setToolTip(tr("MOD Desktop: Running"));
        systray->showMessage(tr("MOD Desktop"), tr("Running"), QSystemTrayIcon::Information);
    }

    void setStopped()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        ui.b_start->setEnabled(true);
        ui.b_stop->setEnabled(false);
        ui.b_opengui->setEnabled(false);
        ui.cb_device->setEnabled(true);
        ui.l_device->setEnabled(true);
        ui.gb_audio->setEnabled(true);
        ui.gb_midi->setEnabled(true);
        ui.gb_lv2->setEnabled(true);
        ui.l_status->setText(tr("Stopped"));
        systray->setToolTip(tr("MOD Desktop: Stopped"));
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

        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
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
            "mod-desktop",
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
       #if defined(Q_OS_LINUX)
        arguments.append(midiEnabled ? "./jack/jack-session-alsamidi.conf" : "./jack/jack-session.conf");
       #elif defined(Q_OS_WIN)
        arguments.append(".\\jack\\jack-session.conf");
       #else
        arguments.append("./jack/jack-session.conf");
       #endif

        if (ui.cb_verbose_jackd->isChecked() && ui.cb_verbose_jackd->isEnabled())
            arguments.append("-v");

        arguments.append("-d");
       #if defined(Q_OS_LINUX)
        arguments.append(devInfo.uid.startsWith("hw:") ? "alsa" : "portaudio");
       #elif defined(Q_OS_MAC)
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

        processHost.setArguments(arguments);

        {
            QProcessEnvironment env(QProcessEnvironment::systemEnvironment());

            env.insert("LV2_PATH", getLV2Path(ui.cb_lv2_all_plugins->isChecked()));

            if (ui.cb_verbose_host->isChecked() && ui.cb_verbose_host->isEnabled())
                env.insert("MOD_LOG", "1");
            else
                env.insert("MOD_LOG", "0");

           #if !(defined(Q_OS_MAC) || defined(Q_OS_WIN))
            char quantum[32] = {};
            std::snprintf(quantum, sizeof(quantum)-1, "%s/48000", ui.cb_buffersize->currentText().toUtf8().constData());
            env.insert("PIPEWIRE_QUANTUM", quantum);
           #endif

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
        ::openWebGui();
    }

    void openUserFilesDir() const
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        ::openUserFilesDir();
    }

    void showLogs(const bool show)
    {
        if (show)
        {
            ui.gb_logs->show();
        }
        else
        {
            ui.gb_logs->hide();
            adjustSize();
        }
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

        if (ui.rb_device_duplex->isChecked() && ! ui.rb_device_duplex->isEnabled())
            ui.rb_device_separate->setChecked(true);

        if (ui.rb_device_separate->isChecked() && ! ui.rb_device_separate->isEnabled())
            ui.rb_device_noinput->setChecked(true);
    }

    void hostStartError(QProcess::ProcessError error)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);

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
        stopUIIfNeeded();
        setStopped();
    }

    void uiFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        startingUI = stoppingUI = false;
        stopHostIfNeeded();
        setStopped();
    }

    void hostReadStdOut()
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

            env.insert("LV2_PATH", getLV2Path(ui.cb_lv2_all_plugins->isChecked()));

            if (ui.cb_verbose_ui->isChecked() && ui.cb_verbose_ui->isEnabled())
                env.insert("MOD_LOG", "2");
            else if (ui.cb_verbose_basic->isChecked())
                env.insert("MOD_LOG", "1");
            else
                env.insert("MOD_LOG", "0");

            if (ui.cb_lv2_all_cv->isChecked())
                env.insert("MOD_UI_ALLOW_REGULAR_CV", "1");

            if (ui.cb_lv2_only_with_modgui->isChecked())
                env.insert("MOD_UI_ONLY_SHOW_PLUGINS_WITH_MODGUI", "1");

            processUI.setProcessEnvironment(env);
            processUI.start();
        }
    }

    void uiReadStdOut()
    {
        const QByteArray text = processUI.readAll().trimmed();

        if (! text.isEmpty())
            ui.text_ui->appendPlainText(text);
    }

    void iconActivated(QSystemTrayIcon::ActivationReason reason)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        switch (reason)
        {
        case QSystemTrayIcon::DoubleClick:
        case QSystemTrayIcon::Trigger:
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
