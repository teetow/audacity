/*
 * Audacity: A Digital Audio Editor
 */
#include "screencapmode.h"

#include <QtGlobal>

#include "appshell/testflowstartup.h"
#include "effects/effects_base/ieffectsproviderinitializer.h"

#if (defined (_MSCVER) || defined (_MSC_VER))
#include <windows.h>
#endif

#include "log.h"

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

void screencap::prepareStartup(const muse::modularity::ContextPtr& ctx)
{
    if (!isEnabled()) {
        return;
    }

    au::appshell::installTestflowStartupScenario(ctx);

    auto effectsInitializer = muse::modularity::ioc(ctx)->resolve<au::effects::IEffectsProviderInitializer>("app");
    IF_ASSERT_FAILED(effectsInitializer) {
        return;
    }
    effectsInitializer->setStartupPluginValidationPolicy(au::effects::StartupPluginValidationPolicy::Skip);
}
