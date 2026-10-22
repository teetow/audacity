/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <memory>

#include "framework/global/modularity/imodulesetup.h"

namespace au::screencap {
class ScreencapController;

//! Commands for capture rigs, driven over remote control in screencap mode.
class ScreencapModule : public muse::modularity::IModuleSetup
{
public:
    std::string moduleName() const override;
    void resolveImports() override;

    muse::modularity::IContextSetup* newContext(const muse::modularity::ContextPtr& ctx) const override;
};

class ScreencapContext : public muse::modularity::IContextSetup
{
public:
    ScreencapContext(const muse::modularity::ContextPtr& ctx)
        : muse::modularity::IContextSetup(ctx) {}

    void registerExports() override;
    void resolveImports() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

private:
    std::shared_ptr<ScreencapController> m_controller;
};
}
