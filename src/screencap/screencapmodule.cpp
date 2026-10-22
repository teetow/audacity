/*
 * Audacity: A Digital Audio Editor
 */
#include "screencapmodule.h"

#include "framework/global/modularity/ioc.h"
#include "framework/rcommand/icommandsregister.h"
#include "framework/rcommand/icommandsstate.h"

#include "internal/screencapcommandsregister.h"
#include "internal/screencapcommandsstate.h"
#include "internal/screencapcontroller.h"

using namespace au::screencap;
using namespace muse;
using namespace muse::modularity;

static const std::string mname("screencap");

std::string ScreencapModule::moduleName() const
{
    return mname;
}

void ScreencapModule::resolveImports()
{
    auto cr = globalIoc()->resolve<rcommand::ICommandsRegister>(mname);
    if (cr) {
        cr->reg(std::make_shared<ScreencapCommandsRegister>());
    }
}

IContextSetup* ScreencapModule::newContext(const ContextPtr& ctx) const
{
    return new ScreencapContext(ctx);
}

void ScreencapContext::registerExports()
{
    m_controller = std::make_shared<ScreencapController>(iocContext());
}

void ScreencapContext::resolveImports()
{
    auto cs = ioc()->resolve<rcommand::ICommandsState>(mname);
    if (cs) {
        cs->reg(std::make_shared<ScreencapCommandsState>());
    }
}

void ScreencapContext::onInit(const IApplication::RunMode& mode)
{
    if (mode != IApplication::RunMode::GuiApp) {
        return;
    }

    m_controller->init();
}
