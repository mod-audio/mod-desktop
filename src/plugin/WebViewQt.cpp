// SPDX-FileCopyrightText: 2023-2024 MOD Audio UG
// SPDX-License-Identifier: AGPL-3.0-or-later

// TODO split build

#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWebEngineWidgets/QWebEngineView>

#undef signals

#include <gtk/gtkx.h>
#include <webkit2/webkit2.h>

#include <clocale>
#include <cstdio>
#include <dlfcn.h>
#include <new>
#include <X11/Xlib.h>

#include "DistrhoPluginInfo.h"
#include "DistrhoUtils.hpp"

// #include "../systray/qrc_mod-desktop.hpp"
// #include "../systray/utils.cpp"

// -----------------------------------------------------------------------------------------------------------

#define JOIN(A, B) A ## B

#define GTK3SYM(S) \
    using JOIN(gtk3_, S) = decltype(&S); \
    JOIN(gtk3_, S) S = reinterpret_cast<JOIN(gtk3_, S)>(dlsym(nullptr, #S)); \
    DISTRHO_SAFE_ASSERT_RETURN(S != nullptr, false);

// -----------------------------------------------------------------------------------------------------------
// gtk3 variant

static bool gtk3(Display* const display, const Window winId, double scaleFactor, const char* const url)
{
    void* const lib = dlopen("libwebkit2gtk-4.0.so.37_____", RTLD_NOW|RTLD_GLOBAL);
    DISTRHO_SAFE_ASSERT_RETURN(lib != nullptr, false);

    GTK3SYM(g_type_check_instance_cast)
    GTK3SYM(gdk_set_allowed_backends)
    GTK3SYM(gtk_container_add)
    GTK3SYM(gtk_container_get_type)
    GTK3SYM(gtk_init_check)
    GTK3SYM(gtk_main)
    GTK3SYM(gtk_plug_get_id)
    GTK3SYM(gtk_plug_get_type)
    GTK3SYM(gtk_plug_new)
    GTK3SYM(gtk_widget_show_all)
    GTK3SYM(gtk_window_get_type)
    GTK3SYM(gtk_window_move)
    GTK3SYM(gtk_window_set_default_size)
    GTK3SYM(webkit_settings_new)
    GTK3SYM(webkit_settings_set_hardware_acceleration_policy)
    GTK3SYM(webkit_settings_set_javascript_can_access_clipboard)
    GTK3SYM(webkit_web_view_get_type)
    GTK3SYM(webkit_web_view_load_uri)
    GTK3SYM(webkit_web_view_new_with_settings)

    const int gdkScale = std::fmod(scaleFactor, 1.0) >= 0.75
                       ? static_cast<int>(scaleFactor + 0.5)
                       : static_cast<int>(scaleFactor);

    if (gdkScale != 1)
    {
        char scale[8] = {};
        std::snprintf(scale, 7, "%d", gdkScale);
        setenv("GDK_SCALE", scale, 1);

        std::snprintf(scale, 7, "%.2f", (1.0 / scaleFactor) * 1.2);
        setenv("GDK_DPI_SCALE", scale, 1);
    }
    else if (scaleFactor > 1.0)
    {
        char scale[8] = {};
        std::snprintf(scale, 7, "%.2f", (1.0 / scaleFactor) * 1.4);
        setenv("GDK_DPI_SCALE", scale, 1);
    }

    scaleFactor /= gdkScale;

    gdk_set_allowed_backends("x11");

    if (! gtk_init_check (nullptr, nullptr))
        return false;

    GtkWidget* const window = gtk_plug_new(winId);
    DISTRHO_SAFE_ASSERT_RETURN(window != nullptr, false);

    gtk_window_set_default_size(GTK_WINDOW(window),
                                DISTRHO_UI_DEFAULT_WIDTH * scaleFactor,
                                (DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset) * scaleFactor);
    gtk_window_move(GTK_WINDOW(window), 0, kVerticalOffset * scaleFactor);

    WebKitSettings* const settings = webkit_settings_new();
    DISTRHO_SAFE_ASSERT_RETURN(settings != nullptr, false);

    webkit_settings_set_javascript_can_access_clipboard(settings, true);
    webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);

    GtkWidget* const webview = webkit_web_view_new_with_settings(settings);
    DISTRHO_SAFE_ASSERT_RETURN(webview != nullptr, false);

    webkit_web_view_load_uri (WEBKIT_WEB_VIEW (webview), url);

    gtk_container_add(GTK_CONTAINER(window), webview);

    gtk_widget_show_all(window);

    Window wid = gtk_plug_get_id(GTK_PLUG(window));
    XMapWindow(display, wid);
    XFlush(display);

    gtk_main();

    dlclose(lib);
    return true;
}

// -----------------------------------------------------------------------------------------------------------
// qt5webengine variant

// using _QApplication_QApplication = decltype(QApplication::QApplication);

#define QT5SYM(S, NAME, SN) \
    using SN = decltype(&S); \
    SN NAME = reinterpret_cast<SN>(dlsym(nullptr, #SN)); \
    DISTRHO_SAFE_ASSERT_RETURN(NAME != nullptr, false);

static bool qt5webengine(const Window winId, const double scaleFactor, const char* const url)
{
    void* const lib = dlopen("libQt5WebEngineWidgets.so.5", RTLD_NOW|RTLD_GLOBAL);
    DISTRHO_SAFE_ASSERT_RETURN(lib != nullptr, false);

//     qOverload<int, QProcess::ExitStatus>(
//     using qt5webengine_qapp = decltype(&QApplication::QApplication);
//     qt5webengine_qapp qapp = reinterpret_cast<qt5webengine_qapp>(dlsym(nullptr, "QApplication::QApplication"));
//     DISTRHO_SAFE_ASSERT_RETURN(qapp != nullptr, false);

//     QT5SYM(QGuiApplication::QGuiApplication, QApplication_QApplication, _ZN15QGuiApplication4execEv)
    QT5SYM(QApplication::setAttribute, QApplication_setAttribute, _ZN16QCoreApplication12setAttributeEN2Qt20ApplicationAttributeEb)
    QT5SYM(QApplication::exec, QApplication_exec, _ZN15QGuiApplication4execEv)
//     QT5SYM(QWebEngineView::move, QWebEngineView_move, _ZN7QWidget4moveERK6QPoint)
//     QT5SYM(QWebEngineView::setFixedSize, QWebEngineView_setFixedSize, _ZN7QWidget12setFixedSizeEii)
//     QT5SYM(QWebEngineView::winId, QWebEngineView_winId, _ZNK7QWidget5winIdEv)
//     QT5SYM(QWebEngineView::windowHandle, QWebEngineView_windowHandle, _ZNK7QWidget12windowHandleEv)
//     QT5SYM(QWebEngineView::setUrl, QWebEngineView_setUrl, _ZN14QWebEngineView6setUrlERK4QUrl)
//     QT5SYM(QWebEngineView::show, QWebEngineView_show, _ZN7QWidget4showEv)
    QT5SYM(QWindow::fromWinId, QWindow_fromWinId, _ZN7QWindow9fromWinIdEy)

    unsetenv("QT_FONT_DPI");
    unsetenv("QT_SCREEN_SCALE_FACTORS");
    unsetenv("QT_USE_PHYSICAL_DPI");
    setenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0", 1);

    char scale[8] = {};
    std::snprintf(scale, 7, "%.2f", scaleFactor);
    setenv("QT_SCALE_FACTOR", scale, 1);

    QApplication_setAttribute(Qt::AA_X11InitThreads, true);
    QApplication_setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication_setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    static int argc = 0;
    static char* argv[] = { nullptr };

    uint8_t app_data[sizeof(QApplication)];
    QApplication* const app = reinterpret_cast<QApplication*>(app_data);
    new(app)QApplication(argc, argv);

    QWindow* const parentWindow = QWindow_fromWinId(winId);

    uint8_t webview_data[sizeof(QWebEngineView)];
    QWebEngineView* const webview = reinterpret_cast<QWebEngineView*>(webview_data);
    new(webview)QWebEngineView();

    webview->move(0, kVerticalOffset);
    webview->setFixedSize(DISTRHO_UI_DEFAULT_WIDTH,
                          DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset);
    webview->winId();
    webview->windowHandle()->setParent(parentWindow);
    webview->setUrl(QUrl(QString::fromLocal8Bit(url)));
    webview->show();

    QApplication_exec();

    webview->~QWebEngineView();
    app->~QApplication();

    dlclose(lib);
    return true;
}

// -----------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: %s x11-win-id scale-factor port\n", argv[0]);
        return 2;
    }

    uselocale(newlocale(LC_NUMERIC_MASK, "C", nullptr));

    const Window winId = std::strtoul(argv[1], nullptr, 10);
    DISTRHO_SAFE_ASSERT_RETURN(winId != 0, 1);

    const double scaleFactor = std::atof(argv[2]);
    DISTRHO_SAFE_ASSERT_RETURN(scaleFactor > 0.0, 1);

    Display* const display = XOpenDisplay(nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(display != nullptr, 1);

    char url[32] = {};
    std::snprintf(url, 31, "http://127.0.0.1:%s/", argv[3]);

    const bool ok = qt5webengine(winId, scaleFactor, url) ||
                    gtk3(display, winId, scaleFactor, url);

    XCloseDisplay(display);

    return ok ? 0 : 1;
}

// -----------------------------------------------------------------------------------------------------------
