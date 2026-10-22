/*
 * Audacity: A Digital Audio Editor
 */
#include "userinputblocker.h"

#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QQuickWindow>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

using namespace au::screencap;

UserInputBlocker::~UserInputBlocker()
{
    setBlocking(false);
}

bool UserInputBlocker::isSupported()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

bool UserInputBlocker::isBlocking() const
{
    return m_blocking;
}

void UserInputBlocker::setBlocking(bool blocking)
{
    if (m_blocking == blocking || !isSupported()) {
        return;
    }

    m_blocking = blocking;

    if (!blocking) {
        QCoreApplication::instance()->removeNativeEventFilter(this);
        return;
    }

    QCoreApplication::instance()->installNativeEventFilter(this);

    //! NOTE What the pointer hovers stays hovered until it leaves; make it leave now
    for (QWindow* window : QGuiApplication::allWindows()) {
        if (qobject_cast<QQuickWindow*>(window)) {
            QEvent leave(QEvent::Leave);
            QCoreApplication::sendEvent(window, &leave);
        }
    }
}

bool UserInputBlocker::nativeEventFilter(const QByteArray& eventType, void* message, qintptr*)
{
#ifdef Q_OS_WIN
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG") {
        return false;
    }

    const UINT msg = static_cast<const MSG*>(message)->message;
    const bool mouse = msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST;
    const bool nonClientMouse = (msg >= WM_NCMOUSEMOVE && msg <= WM_NCXBUTTONDBLCLK) || msg == WM_NCMOUSEHOVER;
    const bool hover = msg == WM_MOUSEHOVER;
    const bool keyboard = msg >= WM_KEYFIRST && msg <= WM_KEYLAST;
    const bool pointer = msg >= WM_NCPOINTERUPDATE && msg <= WM_POINTERHWHEEL && msg != WM_POINTERACTIVATE;
    const bool touch = msg == WM_TOUCH || msg == WM_GESTURE;
    return mouse || nonClientMouse || hover || keyboard || pointer || touch;
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    return false;
#endif
}
