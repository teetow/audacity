/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <memory>

#include "framework/global/modularity/imodulesetup.h"

namespace au::screencap {
class ScreencapController;
class ScreencapAudioDriverController;

//! Commands for capture rigs, driven over remote control in screencap mode.
class ScreencapModule : public muse::modularity::IModuleSetup
{
public:
    std::string moduleName() const override;
    void registerExports() override;
    void resolveImports() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

    muse::modularity::IContextSetup* newContext(const muse::modularity::ContextPtr& ctx) const override;

private:
    std::shared_ptr<ScreencapAudioDriverController> m_audioDriverController;
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
