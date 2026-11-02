/*
 * Audacity: A Digital Audio Editor
 */
#include "screencapcommandsregister.h"

#include "framework/global/types/translatablestring.h"

#include "../screencapcommands.h"

using namespace au::screencap;
using namespace muse;
using namespace muse::rcommand;

namespace {
//! NOTE Tools for capture rigs, not user-facing: titles and descriptions aren't translated
CommandInfo info(const Command& command, const char* title, const char* description, InputSchema schema = {})
{
    return CommandInfo { command, TranslatableString::untranslatable(title), TranslatableString::untranslatable(description),
                         std::move(schema), Decoration() };
}

Arg stringArg(const char16_t* description)
{
    return Arg(DataType::String, String(description));
}

const InputSchema CONTROL_SCHEMA({
    { "section", stringArg(u"Navigation section name") },
    { "panel", stringArg(u"Navigation panel name") },
    { "control", stringArg(u"Navigation control name") },
});

const std::vector<CommandInfo> s_commandInfos = {
    info(SCREENCAP_LOOKS_COMMAND, "Looks",
         "The theme codes, and the accent colors of the current theme"),
    info(SCREENCAP_LOOK_COMMAND, "Set look",
         "Set the theme and, optionally, its accent color (one of the theme's, see Looks after switching); stops following the system theme",
         InputSchema({ { "theme", stringArg(u"Theme code") }, { "accent", stringArg(u"Accent color, #rrggbb") } })),
    info(SCREENCAP_CONTROLS_COMMAND, "Controls",
         "Navigation sections; with section, its panels; with section and panel, its controls",
         InputSchema({ { "section", stringArg(u"Navigation section name") }, { "panel", stringArg(u"Navigation panel name") } })),
    info(SCREENCAP_TRIGGER_COMMAND, "Trigger control",
         "Trigger a control as Enter/Space would; fails if it doesn't exist or is disabled", CONTROL_SCHEMA),
    info(SCREENCAP_RECT_COMMAND, "Control rectangle",
         "A control's rectangle in capture coordinates (device pixels from the main window's top-left)", CONTROL_SCHEMA),
    info(SCREENCAP_POPUPS_COMMAND, "Popups",
         "Open popups, menus and dialogs, oldest first, with their rectangles in capture coordinates",
         InputSchema({
        { "wait_for", stringArg(u"A popup's name: answer once it is open and has drawn a frame") },
        { "count", Arg(DataType::Integer, String(u"With wait_for: how many of that name must be open (a submenu has its menu's name); 1 by default")) },
    })),
    info(SCREENCAP_CLOSE_POPUPS_COMMAND, "Close popups",
         "Close every open popup, menu and dialog (a dialog closed this way is cancelled)"),
    info(SCREENCAP_CAPTURE_COMMAND, "Capture",
         "Save the main window with its open popups to a PNG, optionally cropped (capture coordinates)",
         InputSchema({
        { "path", stringArg(u"Absolute path of the PNG to write; its folder must exist") },
        { "x", Arg(DataType::Integer, String(u"Crop left")) },
        { "y", Arg(DataType::Integer, String(u"Crop top")) },
        { "width", Arg(DataType::Integer, String(u"Crop width")) },
        { "height", Arg(DataType::Integer, String(u"Crop height")) },
    })),
    info(SCREENCAP_DISPATCH_COMMAND, "Dispatch",
         "Dispatch a command:// or action:// URI with query parameters; for a command, answers with its result",
         InputSchema({ { "uri", stringArg(u"command://... or action://... URI") } })),
    info(SCREENCAP_USER_INPUT_COMMAND, "User input",
         "Accept or ignore the user's own mouse and keyboard (ignored from startup, so they leave no marks in captures); "
         "ignoring again clears hover, but not marks the app keeps, such as the ruler's pointer line. Commands are unaffected",
         InputSchema({ { "enabled", Arg(DataType::Boolean, String(u"Whether the app reacts to the user's input")) } })),
    info(SCREENCAP_WINDOW_COMMAND, "Window size",
         "Size the main window (device pixels) and move it to the top-left corner of its screen, so popups never "
         "have to move to stay on screen; fails if it doesn't fit there",
         InputSchema({
        { "width", Arg(DataType::Integer, String(u"Width, device pixels")) },
        { "height", Arg(DataType::Integer, String(u"Height, device pixels")) },
    })),
    info(SCREENCAP_INPUT_METER_COMMAND, "Input meter",
         "Show this level on every input meter (both channels); in screencap mode the microphone is never opened, "
         "so the meters show only what this sets",
         InputSchema({
        { "peak", Arg(DataType::Float, String(u"Peak level, dBFS")) },
        { "rms", Arg(DataType::Float, String(u"RMS level, dBFS")) },
    })),
    info(SCREENCAP_VISIBLE_COMMAND, "Visible",
         "Show the app's windows on screen, or keep them see-through and click-through (the default, so a run "
         "doesn't flash on the user's screen); captures look the same either way",
         InputSchema({ { "visible", Arg(DataType::Boolean, String(u"Whether the windows can be seen and clicked")) } })),
    info(SCREENCAP_SETTINGS_SNAPSHOT_COMMAND, "Settings snapshot",
         "Remember every setting's value, for Settings restore to return to"),
    info(SCREENCAP_SETTINGS_RESTORE_COMMAND, "Settings restore",
         "Set every setting that changed since the last snapshot back to its snapshot value, the way the UI sets "
         "it (listeners are told); answers with the settings it restored"),
    info(SCREENCAP_OPEN_COMMAND, "Open",
         "Open an app page or dialog by its URI (audacity://welcomedialog), as the app itself would; answers once "
         "it is asked to open, so wait for it with Popups",
         InputSchema({ { "uri", stringArg(u"The page's URI, with query parameters if it takes any") } })),
};
}

std::string ScreencapCommandsRegister::moduleName() const
{
    return "screencap";
}

const std::vector<Command>& ScreencapCommandsRegister::commandList() const
{
    static std::vector<Command> commands;
    if (commands.empty()) {
        commands.reserve(s_commandInfos.size());
        for (const auto& info : s_commandInfos) {
            commands.push_back(info.command);
        }
    }
    return commands;
}

const std::vector<CommandInfo>& ScreencapCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
