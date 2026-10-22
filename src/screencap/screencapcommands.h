/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include "framework/rcommand/commandtypes.h"

namespace au::screencap {
inline static const muse::rcommand::Command SCREENCAP_LOOKS_COMMAND("command://screencap/looks");
inline static const muse::rcommand::Command SCREENCAP_LOOK_COMMAND("command://screencap/look");
inline static const muse::rcommand::Command SCREENCAP_CONTROLS_COMMAND("command://screencap/controls");
inline static const muse::rcommand::Command SCREENCAP_TRIGGER_COMMAND("command://screencap/trigger");
inline static const muse::rcommand::Command SCREENCAP_RECT_COMMAND("command://screencap/rect");
inline static const muse::rcommand::Command SCREENCAP_POPUPS_COMMAND("command://screencap/popups");
inline static const muse::rcommand::Command SCREENCAP_CLOSE_POPUPS_COMMAND("command://screencap/close-popups");
inline static const muse::rcommand::Command SCREENCAP_CAPTURE_COMMAND("command://screencap/capture");
inline static const muse::rcommand::Command SCREENCAP_DISPATCH_COMMAND("command://screencap/dispatch");
inline static const muse::rcommand::Command SCREENCAP_USER_INPUT_COMMAND("command://screencap/user-input");
}
