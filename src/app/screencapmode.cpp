/*
 * Audacity: A Digital Audio Editor
 */
#include "screencapmode.h"

#include <QtGlobal>

#if (defined (_MSCVER) || defined (_MSC_VER))
#include <windows.h>
#endif


using namespace au::app;

bool screencap::isEnabled()
{
    static const bool enabled = qEnvironmentVariableIsSet("MUSE_SCREENCAP");
    return enabled;
}

#if (defined (_MSCVER) || defined (_MSC_VER))
static LRESULT CALLBACK noActivateProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HCBT_ACTIVATE) {
        const CBTACTIVATESTRUCT* activate = reinterpret_cast<const CBTACTIVATESTRUCT*>(lParam);
        if (!activate || !activate->fMouse) {
            return 1; // refuse activation that isn't the user clicking
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

#endif

void screencap::installNoActivate()
{
#if (defined (_MSCVER) || defined (_MSC_VER))
    if (isEnabled()) {
        SetWindowsHookExW(WH_CBT, noActivateProc, nullptr, GetCurrentThreadId());
    }
#endif
}
