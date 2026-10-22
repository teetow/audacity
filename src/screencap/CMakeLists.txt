#
# Audacity: A Digital Audio Editor
#
# Screencap mode: commands a capture rig uses over remote control (see
# screencap/README.md). Registered only when MUSE_SCREENCAP is set.

muse_create_module(screencap NO_UNITY)

target_sources(screencap PRIVATE
    screencapmodule.cpp
    screencapmodule.h
    screencapcommands.h

    internal/screencapcommandsregister.cpp
    internal/screencapcommandsregister.h
    internal/screencapcommandsstate.cpp
    internal/screencapcommandsstate.h
    internal/screencapcontroller.cpp
    internal/screencapcontroller.h
    internal/userinputblocker.cpp
    internal/userinputblocker.h
)

target_link_libraries(screencap PRIVATE
    muse::ui
    muse::uicomponents
    muse::uicomponents_qml
    Qt::Quick
)
