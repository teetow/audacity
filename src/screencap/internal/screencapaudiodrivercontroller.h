/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <memory>

#include "framework/global/async/asyncable.h"

#include "audio/driver/iaudiodrivercontroller.h"

namespace au::screencap {
//! Stands in for the audio driver controller in screencap mode, so captures show a
//! made-up set of hosts and devices, never the capturing machine's own. Choosing a
//! device or rescanning changes only this made-up state (and tells listeners, as the
//! real one does); nothing reaches the real audio setup. The made-up routing is kept in
//! settings of its own, so putting settings back puts it back too. What doesn't identify
//! a machine (buffer, latency, sample rates and formats) comes from the real controller.
class ScreencapAudioDriverController : public audio::IAudioDriverController, public muse::async::Asyncable
{
public:
    explicit ScreencapAudioDriverController(std::shared_ptr<audio::IAudioDriverController> real);

    //! Once settings are loaded: loading replaces whatever was registered before
    void init();

    audio::AudioConfiguration configuration() const override;
    muse::async::Channel<audio::AudioConfigurationDelta> configurationChanged() const override;

    std::vector<std::string> apis() const override;
    std::vector<std::string> outputDevices() const override;
    std::vector<std::string> outputDevices(const std::string& api) const override;
    std::string systemDefaultOutputDevice(const std::string& api) const override;
    std::vector<std::string> inputDevices() const override;
    std::vector<std::string> inputDevices(const std::string& api) const override;
    std::string systemDefaultInputDevice(const std::string& api) const override;
    int inputChannelsAvailable() const override;
    int inputChannelsAvailable(const std::string& api, const audio::AudioDeviceSelection& inputDevice) const override;
    std::vector<uint64_t> sampleRates() const override;
    std::vector<std::string> sampleFormats() const override;

    audio::ApplyResult apply(const muse::modularity::ContextPtr& requester, const audio::AudioConfigurationChange& change) override;
    audio::ApplyResult rescan() override;
    audio::ApplyResult reload(const muse::modularity::ContextPtr& requester) override;
    audio::ApplyResult openAsioDriverSettings(const audio::AudioRoutingChange& routing) override;

    muse::async::Notification audioDeviceListChanged() const override;

    muse::async::Channel<std::string> usedOutputDeviceChanged() const override;
    muse::async::Channel<std::string> usedInputDeviceChanged() const override;

private:
    std::string usedOutputDevice() const;
    std::string usedInputDevice() const;

    std::shared_ptr<audio::IAudioDriverController> m_real;

    muse::async::Channel<audio::AudioConfigurationDelta> m_configurationChanged;
    muse::async::Notification m_audioDeviceListChanged;
    muse::async::Channel<std::string> m_usedOutputDeviceChanged;
    muse::async::Channel<std::string> m_usedInputDeviceChanged;
};
}
