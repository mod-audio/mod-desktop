// SPDX-FileCopyrightText: 2023 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "ui_mod-app.hpp"

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
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

class AppWindow : public QMainWindow
{
//     Q_OBJECT

    Ui_AppWindow ui;

    QAction* openAction;
    QAction* quitAction;
    QMenu* sysmenu;
    QSystemTrayIcon* systray;

    int heightLogs = 100;

    QProcess processHost;
    QProcess processUI;
    bool startingHost = false;
    bool startingUI = false;
    bool stoppingHost = false;
    bool stoppingUI = false;
    int timerId = 0;

public:
    AppWindow()
        : processHost(this),
          processUI(this)
    {
        ui.setupUi(this);

        ui.b_start->setEnabled(false);
        ui.b_stop->setEnabled(false);
        ui.b_opengui->setEnabled(false);
        ui.gb_logs->setChecked(false);
        ui.tab_logs->hide();
        adjustSize();

        connect(ui.b_start, &QAction::triggered, this, &AppWindow::start);
        connect(ui.b_stop, &QAction::triggered, this, &AppWindow::stop);
        connect(ui.b_opengui, &QAction::triggered, this, &AppWindow::openGui);
        connect(ui.gb_logs, &QGroupBox::toggled, this, &AppWindow::showLogs);

        const QIcon icon(":/mod-logo.svg");

        openAction = new QAction(tr("&Open"), this);
        connect(openAction, &QAction::triggered, this, &AppWindow::show);

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

        processHost.setProcessChannelMode(QProcess::MergedChannels);
        processUI.setProcessChannelMode(QProcess::MergedChannels);

        connect(&processHost, QProcess::error, this, &AppWindow::hostStartError);
        connect(&processHost, QProcess::started, this, &AppWindow::hostStartSuccess);
        connect(&processHost, QProcess::finished, this, &AppWindow::hostFinished);

        connect(&processUI, QProcess::error, this, &AppWindow::uiStartError);
        connect(&processUI, QProcess::started, this, &AppWindow::uiStartSuccess);
        connect(&processUI, QProcess::finished, this, &AppWindow::uiFinished);

        timerId = startTimer(500);
    }

    void fillInDeviceList()
    {
        printf("--------------------------------------------------------\n");

       #ifdef _WIN32
        SetEnvironmentVariableA("JACK_NO_AUDIO_RESERVATION", "1");
        SetEnvironmentVariableA("JACK_NO_START_SERVER", "1");

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

    void show()
    {
        systray->show();
        QMainWindow::show();
    }

protected:
    void closeEvent(QCloseEvent* const event) override
    {
        if (!event->spontaneous() || !isVisible())
            return;

        if (systray->isVisible())
        {
            QMessageBox::information(this, tr("Systray"),
                                     tr("The program will keep running in the "
                                        "system tray. To terminate the program, "
                                        "choose <b>Quit</b> in the context menu "
                                        "of the system tray entry."));
            hide();
            event->ignore();
        }

//         saveSettings();
// 
//         if processUI.state() != QProcess.NotRunning:
//             self.fStoppingUI = True
//             processUI.terminate()
//             if not processUI.waitForFinished(500):
//                 processUI.kill()
// 
//         if processHost.state() != QProcess.NotRunning:
//             self.fStoppingBackend = True
//             processHost.terminate()
//             if not processBackend.waitForFinished(500):
//                 processBackend.kill()

    }

    void timerEvent(QTimerEvent* const event) override
    {
        if (event->timerId() == timerId)
        {
            if (startingHost || processHost.state() != QProcess::NotRunning)
            {
//                 text = str(processHost.readAll().trimmed(), encoding="utf-8", errors="ignore")
//                 if text: self.ui.text_backend.appendPlainText(text)
            }

            if (startingUI || processUI.state() != QProcess::NotRunning)
            {
//                 text = str(processUI.readAll().trimmed(), encoding="utf-8", errors="ignore")
//                 if text: self.ui.text_ui.appendPlainText(text)
            }

            startingHost = startingUI = false;
        }

        QMainWindow::timerEvent(event);
    }

private:
    void saveSettings()
    {
        QSettings settings;
        settings.setValue("Geometry", saveGeometry());
    }

    void loadSettings()
    {
//         QSettings settings;
//         if (settings.contains("Geometry"))
//             restoreGeometry(settings.value("Geometry", ""));
    }

    QString getProcessErrorAsString(QProcess::ProcessError error)
    {
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

    void stopHostIfNeeded()
    {
        if (processHost.state() == QProcess::NotRunning)
            return;

        stoppingHost = true;
        processHost.terminate();
    }

    void stopUIIfNeeded()
    {
        if (processUI.state() == QProcess::NotRunning)
            return;

        stoppingUI = true;
        processUI.terminate();
    }

private slots:
    void start()
    {
        if (processHost.state() != QProcess::NotRunning)
        {
            qWarning() << "start ignored";
            return;
        }

        ui.text_host->clear();
        ui.text_ui->clear();
        ui.b_start->setEnabled(false);
        ui.b_stop->setEnabled(true);

        startingHost = true;
//         processHost.start(self.localPathToFile("mod-host"), ["-p", "5555", "-f", "5556", "-n"]);
    }

    void stop()
    {
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
    }

    void hostStartError(QProcess::ProcessError error)
    {
        stopUIIfNeeded();

        // crashed while stopping, ignore
        if (error == QProcess::Crashed && stoppingHost)
            return;

        if (processUI.state() != QProcess::NotRunning)
        {
            stoppingUI = true;
            processUI.terminate();
        }

        QMessageBox::critical(nullptr, tr("Error"), tr("Could not start MOD Host.\n") + getProcessErrorAsString(error));
    }

    void uiStartError(QProcess::ProcessError error)
    {
        stopHostIfNeeded();

        // crashed while stopping, ignore
        if (error == QProcess::Crashed && stoppingUI)
            return;

        if (processHost.state() != QProcess::NotRunning)
        {
            stoppingHost = true;
            processHost.terminate();
        }

        QMessageBox::critical(nullptr, tr("Error"), tr("Could not start MOD UI.\n") + getProcessErrorAsString(error));
    }

    void hostStartSuccess()
    {
        startingUI = true;
//         processUI.start(self.localPathToFile("browsepy"));
    }

    void uiStartSuccess()
    {
        ui.b_start->setEnabled(false);
        ui.b_stop->setEnabled(true);
    }

    void hostFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        stoppingHost = false;
        stopUIIfNeeded();

        ui.b_start->setEnabled(true);
        ui.b_stop->setEnabled(false);
    }

    void uiFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        stoppingUI = false;
        stopHostIfNeeded();
    }

    void iconActivated(QSystemTrayIcon::ActivationReason reason)
    {
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
    }

    void showLogs(bool toggled)
    {
        if (toggled)
        {
            ui.tab_logs->show();
            ui.tab_logs->resize(1, heightLogs);
            resize(width(), height() + heightLogs);
        }
        else
        {
            // heightLogs = ui.tab_logs->height();
            ui.tab_logs->hide();
            resize(width(), height() - heightLogs);
            adjustSize();
        }
    }
};
