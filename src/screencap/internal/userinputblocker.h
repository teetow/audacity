/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>

namespace au::screencap {
//! Keeps the user's own mouse, keyboard, pen and touch input away from the app.
//! It works on the platform's messages, before Qt turns them into events, so no
//! event filter in the app (popups install their own) ever sees that input.
//! Qt also makes up pointer events of its own (an Enter when a window appears
//! under the cursor, from the cursor's position): those are dropped as events.
//! Clicking still activates a window: the system does that itself.
class UserInputBlocker : public QObject, public QAbstractNativeEventFilter
{
public:
    ~UserInputBlocker() override;

    //! Whether this platform can block input
    static bool isSupported();

    bool isBlocking() const;

    //! Blocking also clears hover, so nothing the pointer touched stays highlighted
    void setBlocking(bool blocking);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool m_blocking = false;
};
}
