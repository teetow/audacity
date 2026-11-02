/*
 * Audacity: A Digital Audio Editor
 */
#include "screencapaudiodrivercontroller.h"

#include <algorithm>

#include "framework/global/settings.h"

using namespace au::screencap;
using namespace au::audio;

namespace {
//! NOTE A Windows machine with built-in audio and a Focusrite interface, the setup the
//! tutorials describe (Alex records through a Scarlett). Hosts as AU4 names them on
//! Windows; WASAPI is the one it picks by default there.
const std::vector<std::string> HOSTS { "MME", "Windows DirectSound", "Windows WASAPI" };
const std::string DEFAULT_HOST("Windows WASAPI");
const std::vector<std::string> OUTPUT_DEVICES { "Speakers (Realtek(R) Audio)", "Speakers (Focusrite USB Audio)" };
const std::vector<std::string> INPUT_DEVICES { "Microphone Array (Realtek(R) Audio)", "Analogue 1 + 2 (Focusrite USB Audio)" };
constexpr int INPUT_CHANNELS_AVAILABLE = 2;

//! NOTE An empty device is the system default, as in the real controller's settings
const muse::Settings::Key HOST_KEY("screencap", "screencap/audio/host");
const muse::Settings::Key OUTPUT_DEVICE_KEY("screencap", "screencap/audio/playbackDevice");
const muse::Settings::Key INPUT_DEVICE_KEY("screencap", "screencap/audio/recordingDevice");
const muse::Settings::Key INPUT_CHANNELS_KEY("screencap", "screencap/audio/inputChannels");

bool contains(const std::vector<std::string>& list, const std::string& value)
{
    return std::find(list.begin(), list.end(), value) != list.end();
}

AudioDeviceSelection selection(const muse::Settings::Key& key)
{
    const std::string device = muse::settings()->value(key).toString();
    return device.empty() ? AudioDeviceSelection() : AudioDeviceSelection(device);
}

muse::Val settingValue(const AudioDeviceSelection& device)
{
    return muse::Val(device.value_or(std::string()));
}
}

ScreencapAudioDriverController::ScreencapAudioDriverController(std::shared_ptr<IAudioDriverController> real)
    : m_real(std::move(real))
{
}

void ScreencapAudioDriverController::init()
{
    //! NOTE A fresh install: the default host, system default devices, one input channel
    muse::settings()->setDefaultValue(HOST_KEY, muse::Val(DEFAULT_HOST));
    muse::settings()->setDefaultValue(OUTPUT_DEVICE_KEY, muse::Val(std::string()));
    muse::settings()->setDefaultValue(INPUT_DEVICE_KEY, muse::Val(std::string()));
    muse::settings()->setDefaultValue(INPUT_CHANNELS_KEY, muse::Val(1));

    //! NOTE Whatever changes the routing (a choice in the menu, settings being put back)
    //! reaches listeners the same way
    const auto onChanged = [this](const muse::Settings::Key& key, AudioConfigurationField field) {
        muse::settings()->valueChanged(key).onReceive(this, [this, field](const muse::Val&) {
            m_configurationChanged.send(AudioConfigurationDelta { fieldMask(field) });
            if (field == AudioConfigurationField::OutputDevice) {
                m_usedOutputDeviceChanged.send(usedOutputDevice());
            } else if (field == AudioConfigurationField::InputDevice) {
                m_usedInputDeviceChanged.send(usedInputDevice());
            }
        });
    };
    onChanged(HOST_KEY, AudioConfigurationField::Api);
    onChanged(OUTPUT_DEVICE_KEY, AudioConfigurationField::OutputDevice);
    onChanged(INPUT_DEVICE_KEY, AudioConfigurationField::InputDevice);
    onChanged(INPUT_CHANNELS_KEY, AudioConfigurationField::InputChannels);
}

AudioConfiguration ScreencapAudioDriverController::configuration() const
{
    //! NOTE The real controller's buffer, latency and sample format settings, with made-up routing
    AudioConfiguration configuration = m_real->configuration();
    configuration.api = muse::settings()->value(HOST_KEY).toString();
    configuration.outputDevice = selection(OUTPUT_DEVICE_KEY);
    configuration.inputDevice = selection(INPUT_DEVICE_KEY);
    configuration.inputChannels = muse::settings()->value(INPUT_CHANNELS_KEY).toInt();
    return configuration;
}

std::string ScreencapAudioDriverController::usedOutputDevice() const
{
    return selection(OUTPUT_DEVICE_KEY).value_or(OUTPUT_DEVICES.front());
}

std::string ScreencapAudioDriverController::usedInputDevice() const
{
    return selection(INPUT_DEVICE_KEY).value_or(INPUT_DEVICES.front());
}

muse::async::Channel<AudioConfigurationDelta> ScreencapAudioDriverController::configurationChanged() const
{
    return m_configurationChanged;
}

std::vector<std::string> ScreencapAudioDriverController::apis() const
{
    return HOSTS;
}

std::vector<std::string> ScreencapAudioDriverController::outputDevices() const
{
    return OUTPUT_DEVICES;
}

std::vector<std::string> ScreencapAudioDriverController::outputDevices(const std::string&) const
{
    return OUTPUT_DEVICES;
}

std::string ScreencapAudioDriverController::systemDefaultOutputDevice(const std::string&) const
{
    return OUTPUT_DEVICES.front();
}

std::vector<std::string> ScreencapAudioDriverController::inputDevices() const
{
    return INPUT_DEVICES;
}

std::vector<std::string> ScreencapAudioDriverController::inputDevices(const std::string&) const
{
    return INPUT_DEVICES;
}

std::string ScreencapAudioDriverController::systemDefaultInputDevice(const std::string&) const
{
    return INPUT_DEVICES.front();
}

int ScreencapAudioDriverController::inputChannelsAvailable() const
{
    return INPUT_CHANNELS_AVAILABLE;
}

int ScreencapAudioDriverController::inputChannelsAvailable(const std::string&, const AudioDeviceSelection&) const
{
    return INPUT_CHANNELS_AVAILABLE;
}

std::vector<uint64_t> ScreencapAudioDriverController::sampleRates() const
{
    return m_real->sampleRates();
}

std::vector<std::string> ScreencapAudioDriverController::sampleFormats() const
{
    return m_real->sampleFormats();
}

ApplyResult ScreencapAudioDriverController::apply(const muse::modularity::ContextPtr& requester, const AudioConfigurationChange& change)
{
    if ((change.api && !contains(HOSTS, *change.api))
        || (change.outputDevice && change.outputDevice->has_value() && !contains(OUTPUT_DEVICES, **change.outputDevice))
        || (change.inputDevice && change.inputDevice->has_value() && !contains(INPUT_DEVICES, **change.inputDevice))
        || (change.inputChannels && (*change.inputChannels < 1 || *change.inputChannels > INPUT_CHANNELS_AVAILABLE))) {
        return ApplyResult { ApplyStatus::InvalidConfiguration };
    }

    //! NOTE Listeners hear of each change through the settings (see the constructor)
    const AudioConfiguration before = configuration();
    bool routingChanged = false;
    if (change.api && *change.api != before.api) {
        muse::settings()->setSharedValue(HOST_KEY, muse::Val(*change.api));
        routingChanged = true;
    }
    if (change.outputDevice && *change.outputDevice != before.outputDevice) {
        muse::settings()->setSharedValue(OUTPUT_DEVICE_KEY, settingValue(*change.outputDevice));
        routingChanged = true;
    }
    if (change.inputDevice && *change.inputDevice != before.inputDevice) {
        muse::settings()->setSharedValue(INPUT_DEVICE_KEY, settingValue(*change.inputDevice));
        routingChanged = true;
    }
    if (change.inputChannels && *change.inputChannels != before.inputChannels) {
        muse::settings()->setSharedValue(INPUT_CHANNELS_KEY, muse::Val(*change.inputChannels));
        routingChanged = true;
    }

    //! NOTE Buffer, latency and sample settings aren't routing: the real controller keeps them
    AudioConfigurationChange rest;
    rest.bufferLength = change.bufferLength;
    rest.automaticLatencyCompensation = change.automaticLatencyCompensation;
    rest.latencyCompensation = change.latencyCompensation;
    rest.defaultSampleRate = change.defaultSampleRate;
    rest.defaultSampleFormat = change.defaultSampleFormat;
    rest.asioUseDeviceSampleRate = change.asioUseDeviceSampleRate;
    const bool hasRest = rest.bufferLength || rest.automaticLatencyCompensation || rest.latencyCompensation
                         || rest.defaultSampleRate || rest.defaultSampleFormat || rest.asioUseDeviceSampleRate;
    if (hasRest) {
        const ApplyResult result = m_real->apply(requester, rest);
        if (!result.succeeded()) {
            return result;
        }
    }

    return ApplyResult { routingChanged || hasRest ? ApplyStatus::Applied : ApplyStatus::NoChange };
}

ApplyResult ScreencapAudioDriverController::rescan()
{
    //! NOTE The made-up devices are always there: a rescan finds the same list
    m_audioDeviceListChanged.notify();
    return ApplyResult { ApplyStatus::NoChange };
}

ApplyResult ScreencapAudioDriverController::reload(const muse::modularity::ContextPtr&)
{
    return ApplyResult { ApplyStatus::NoChange };
}

ApplyResult ScreencapAudioDriverController::openAsioDriverSettings(const AudioRoutingChange&)
{
    return ApplyResult { ApplyStatus::NoAsioDevice };
}

muse::async::Notification ScreencapAudioDriverController::audioDeviceListChanged() const
{
    return m_audioDeviceListChanged;
}

muse::async::Channel<std::string> ScreencapAudioDriverController::usedOutputDeviceChanged() const
{
    return m_usedOutputDeviceChanged;
}

muse::async::Channel<std::string> ScreencapAudioDriverController::usedInputDeviceChanged() const
{
    return m_usedInputDeviceChanged;
}
