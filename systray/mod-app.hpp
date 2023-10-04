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
#endif

#ifdef Q_OS_WIN
#include <portaudio.h>
#include <windows.h>
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
};

class AppWindow : public QMainWindow
{
//     Q_OBJECT

    Ui_AppWindow ui;

    QAction* openAction = nullptr;
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
    int timerId = 0;

public:
    AppWindow(const QString& cwd)
        : processHost(this, cwd),
          processUI(this, cwd)
    {
        ui.setupUi(this);

        connect(ui.b_start, &QPushButton::clicked, this, &AppWindow::start);
        connect(ui.b_stop, &QPushButton::clicked, this, &AppWindow::stop);
        connect(ui.b_opengui, &QPushButton::clicked, this, &AppWindow::openGui);
        connect(ui.gb_logs, &QGroupBox::toggled, this, &AppWindow::showLogs);

        const QIcon icon(":/mod-logo.svg");

        openAction = new QAction(tr("&Open GUI"), this);
        connect(openAction, &QAction::triggered, this, &AppWindow::openGui);

        settingsAction = new QAction(tr("&Settings"), this);
        connect(settingsAction, &QAction::triggered, this, &QMainWindow::show);

        quitAction = new QAction(tr("&Quit"), this);
        connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

        sysmenu = new QMenu(this);
        sysmenu->addAction(openAction);
        sysmenu->addSeparator();
        sysmenu->addAction(quitAction);

        systray = new QSystemTrayIcon(icon, this);
        systray->setContextMenu(sysmenu);
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

    void fillInDeviceList()
    {
        printf("--------------------------------------------------------\n");

       #ifdef Q_OS_WIN
        SetEnvironmentVariableW(L"JACK_NO_AUDIO_RESERVATION", L"1");
        SetEnvironmentVariableW(L"JACK_NO_START_SERVER", L"1");

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
                
                if (hostApiName != "ASIO" && hostApiName != "Windows WASAPI")
                    continue;

                wchar_t wdevName[MAX_PATH + 2];
                MultiByteToWideChar(CP_UTF8, 0, devInfo->name, -1, wdevName, MAX_PATH-1);
                ui.cb_device->addItem(hostApiName + "::" + QString::fromWCharArray(wdevName));
            }

            Pa_Terminate();

            if (ui.cb_device->count())
                ui.b_start->setEnabled(true);
        }
       #else
        setenv("JACK_NO_AUDIO_RESERVATION", "1", 1);
        setenv("JACK_NO_START_SERVER", "1", 1);

        // TODO
       #endif

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
    }

    void timerEvent(QTimerEvent* const event) override
    {
        if (event->timerId() == timerId)
        {
            if (startingHost || stoppingHost || processHost.state() != QProcess::NotRunning)
            {
                const QByteArray text = processHost.readAll().trimmed();
                if (! text.isEmpty())
                    ui.text_host->appendPlainText(text);
            }

            if (startingUI || stoppingUI || processUI.state() != QProcess::NotRunning)
            {
                const QByteArray text = processUI.readAll().trimmed();
                if (! text.isEmpty())
                    ui.text_ui->appendPlainText(text);
            }

            startingHost = startingUI = false;
        }

        QMainWindow::timerEvent(event);
    }

private:
    void saveSettings()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        QSettings settings;
        settings.setValue("FirstRun", false);
        settings.setValue("Geometry", saveGeometry());
        settings.setValue("AudioDevice", ui.cb_device->currentText());
        settings.setValue("AudioBufferSize", ui.cb_buffersize->currentText());
        settings.setValue("ShowLogs", ui.tab_logs->isEnabled());
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

        adjustSize();

        if (settings.contains("Geometry"))
            restoreGeometry(settings.value("Geometry").toByteArray());

        if (settings.value("FirstRun", true).toBool())
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
        ui.cb_device->setEnabled(false);
        ui.b_start->setEnabled(false);
        ui.b_stop->setEnabled(true);
        ui.b_opengui->setEnabled(true);
        systray->setToolTip(tr("Running"));
    }

    void setStopped()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        ui.cb_device->setEnabled(true);
        ui.b_start->setEnabled(true);
        ui.b_stop->setEnabled(false);
        ui.b_opengui->setEnabled(false);
        systray->setToolTip(tr("Stopped"));
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

private slots:
    void start()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        if (processHost.state() != QProcess::NotRunning)
        {
            qWarning() << "start ignored";
            return;
        }

        ui.text_host->clear();
        ui.text_ui->clear();

        const QStringList arguments = {
            "-R",
            "-S",
           #if defined(Q_OS_WIN)
            // "-X", "winmme",
           #elif defined(Q_OS_MAC)
            "-X", "coremidi",
           #endif
            "-C",
           #ifdef Q_OS_WIN
            ".\\jack\\jack-session.conf",
           #else
            "./jack/jack-session.conf",
           #endif
            "-d",
           #if defined(Q_OS_WIN)
            "portaudio",
           #elif defined(Q_OS_MAC)
            "coreaudio",
           #else
            "alsa",
           #endif
            "-r", "48000",
            "-p",
            ui.cb_buffersize->currentIndex() == 0 ? "128" : "256",
            "-d",
            ui.cb_device->currentText(),
           #if !(defined(Q_OS_WIN) || defined(Q_OS_MAC))
            "-X", "seqmidi",
           #endif
        };
        processHost.setArguments(arguments);

        ui.b_start->setEnabled(false);

        startingHost = true;
        processHost.start();
    }

    void stop()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        if (processHost.state() == QProcess::NotRunning)
        {
            qWarning() << "stop ignored";
            stopUIIfNeeded();
            return;
        }

        ui.b_stop->setEnabled(false);

        stoppingUI = true;
        processUI.terminate();
    }

    void openGui()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        QDesktopServices::openUrl(QUrl("http://127.0.0.1:18181"));
    }

    void hostStartError(QProcess::ProcessError error)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        stopUIIfNeeded();

        // crashed while stopping, ignore
        if (error == QProcess::Crashed && stoppingHost)
            return;

        if (processUI.state() != QProcess::NotRunning)
        {
            stoppingUI = true;
            processUI.terminate();
        }

        // setRunning();

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

        showErrorMessage(tr("Could not start MOD UI.\n") + getProcessErrorAsString(error));
    }

    void hostStartSuccess()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        startingUI = true;
        processUI.start();
    }

    void uiStartSuccess()
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        setRunning();
    }

    void hostFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        stoppingHost = false;
        stopUIIfNeeded();
        setStopped();
    }

    void uiFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        stoppingUI = false;
        stopHostIfNeeded();
    }

    void iconActivated(QSystemTrayIcon::ActivationReason reason)
    {
        printf("----------- %s %d\n", __FUNCTION__, __LINE__);
        switch (reason)
        {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            setVisible(!isVisible());
            break;
        case QSystemTrayIcon::MiddleClick:
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
