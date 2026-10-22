/*
 * Audacity: A Digital Audio Editor
 */
#include "screencapcommandsstate.h"

using namespace au::screencap;
using namespace muse::rcommand;

std::string ScreencapCommandsState::moduleName() const
{
    return "screencap";
}

CommandState ScreencapCommandsState::commandState(const Command&) const
{
    return CommandState(true);
}

muse::async::Channel<Command, CommandState> ScreencapCommandsState::commandStateChanged() const
{
    return m_commandStateChanged;
}
