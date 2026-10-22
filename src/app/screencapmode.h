/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include "framework/global/modularity/ioc.h"

//! Screencap mode: with the environment variable MUSE_SCREENCAP set, the app runs
//! unattended for capture rigs (see screencap/README.md in the screencap branch).
namespace au::app::screencap {
bool isEnabled();

//! Right after the QApplication is created: on Windows, refuse window activation
//! that isn't a mouse click, so the app never takes the focus by itself.
void installNoActivate();

//! Before the startup scenario: start empty, without startup dialogs, and skip
//! the plugin validation question.
void prepareStartup(const muse::modularity::ContextPtr& ctx);

//! Once the startup scenario has run: open the remote-control channel, so a
//! client that connects knows the app is ready.
void startRemoteControl(const muse::modularity::ContextPtr& ctx);
}
