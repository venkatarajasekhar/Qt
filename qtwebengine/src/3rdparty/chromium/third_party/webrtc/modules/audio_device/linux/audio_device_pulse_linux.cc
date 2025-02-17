/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>

#include "webrtc/modules/audio_device/audio_device_config.h"
#include "webrtc/modules/audio_device/audio_device_utility.h"
#include "webrtc/modules/audio_device/linux/audio_device_pulse_linux.h"

#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

webrtc_adm_linux_pulse::PulseAudioSymbolTable PaSymbolTable;

// Accesses Pulse functions through our late-binding symbol table instead of
// directly. This way we don't have to link to libpulse, which means our binary
// will work on systems that don't have it.
#define LATE(sym) \
  LATESYM_GET(webrtc_adm_linux_pulse::PulseAudioSymbolTable, &PaSymbolTable, sym)

namespace webrtc
{

// ============================================================================
//                              Static Methods
// ============================================================================

AudioDeviceLinuxPulse::AudioDeviceLinuxPulse(const int32_t id) :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _timeEventRec(*EventWrapper::Create()),
    _timeEventPlay(*EventWrapper::Create()),
    _recStartEvent(*EventWrapper::Create()),
    _playStartEvent(*EventWrapper::Create()),
    _ptrThreadPlay(NULL),
    _ptrThreadRec(NULL),
    _recThreadID(0),
    _playThreadID(0),
    _id(id),
    _mixerManager(id),
    _inputDeviceIndex(0),
    _outputDeviceIndex(0),
    _inputDeviceIsSpecified(false),
    _outputDeviceIsSpecified(false),
    sample_rate_hz_(0),
    _recChannels(1),
    _playChannels(1),
    _playBufType(AudioDeviceModule::kFixedBufferSize),
    _initialized(false),
    _recording(false),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _startRec(false),
    _stopRec(false),
    _startPlay(false),
    _stopPlay(false),
    _AGC(false),
    update_speaker_volume_at_startup_(false),
    _playBufDelayFixed(20),
    _sndCardPlayDelay(0),
    _sndCardRecDelay(0),
    _writeErrors(0),
    _playWarning(0),
    _playError(0),
    _recWarning(0),
    _recError(0),
    _deviceIndex(-1),
    _numPlayDevices(0),
    _numRecDevices(0),
    _playDeviceName(NULL),
    _recDeviceName(NULL),
    _playDisplayDeviceName(NULL),
    _recDisplayDeviceName(NULL),
    _playBuffer(NULL),
    _playbackBufferSize(0),
    _playbackBufferUnused(0),
    _tempBufferSpace(0),
    _recBuffer(NULL),
    _recordBufferSize(0),
    _recordBufferUsed(0),
    _tempSampleData(NULL),
    _tempSampleDataSize(0),
    _configuredLatencyPlay(0),
    _configuredLatencyRec(0),
    _paDeviceIndex(-1),
    _paStateChanged(false),
    _paMainloop(NULL),
    _paMainloopApi(NULL),
    _paContext(NULL),
    _recStream(NULL),
    _playStream(NULL),
    _recStreamFlags(0),
    _playStreamFlags(0)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);

    memset(_paServerVersion, 0, sizeof(_paServerVersion));
    memset(&_playBufferAttr, 0, sizeof(_playBufferAttr));
    memset(&_recBufferAttr, 0, sizeof(_recBufferAttr));
    memset(_oldKeyState, 0, sizeof(_oldKeyState));
}

AudioDeviceLinuxPulse::~AudioDeviceLinuxPulse()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);

    Terminate();

    if (_recBuffer)
    {
        delete [] _recBuffer;
        _recBuffer = NULL;
    }
    if (_playBuffer)
    {
        delete [] _playBuffer;
        _playBuffer = NULL;
    }
    if (_playDeviceName)
    {
        delete [] _playDeviceName;
        _playDeviceName = NULL;
    }
    if (_recDeviceName)
    {
        delete [] _recDeviceName;
        _recDeviceName = NULL;
    }

    delete &_recStartEvent;
    delete &_playStartEvent;
    delete &_timeEventRec;
    delete &_timeEventPlay;
    delete &_critSect;
}

void AudioDeviceLinuxPulse::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{

    CriticalSectionScoped lock(&_critSect);

    _ptrAudioBuffer = audioBuffer;

    // Inform the AudioBuffer about default settings for this implementation.
    // Set all values to zero here since the actual settings will be done by
    // InitPlayout and InitRecording later.
    _ptrAudioBuffer->SetRecordingSampleRate(0);
    _ptrAudioBuffer->SetPlayoutSampleRate(0);
    _ptrAudioBuffer->SetRecordingChannels(0);
    _ptrAudioBuffer->SetPlayoutChannels(0);
}

// ----------------------------------------------------------------------------
//  ActiveAudioLayer
// ----------------------------------------------------------------------------

int32_t AudioDeviceLinuxPulse::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = AudioDeviceModule::kLinuxPulseAudio;
    return 0;
}

int32_t AudioDeviceLinuxPulse::Init()
{

    CriticalSectionScoped lock(&_critSect);

    if (_initialized)
    {
        return 0;
    }

    // Initialize PulseAudio
    if (InitPulseAudio() < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to initialize PulseAudio");

        if (TerminatePulseAudio() < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to terminate PulseAudio");
        }

        return -1;
    }

    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    //Get X display handle for typing detection
    _XDisplay = XOpenDisplay(NULL);
    if (!_XDisplay)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
          "  failed to open X display, typing detection will not work");
    }

    // RECORDING
    const char* threadName = "webrtc_audio_module_rec_thread";
    _ptrThreadRec = ThreadWrapper::CreateThread(RecThreadFunc, this,
                                                kRealtimePriority, threadName);
    if (_ptrThreadRec == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to create the rec audio thread");
        return -1;
    }

    unsigned int threadID(0);
    if (!_ptrThreadRec->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the rec audio thread");

        delete _ptrThreadRec;
        _ptrThreadRec = NULL;
        return -1;
    }
    _recThreadID = threadID;

    // PLAYOUT
    threadName = "webrtc_audio_module_play_thread";
    _ptrThreadPlay = ThreadWrapper::CreateThread(PlayThreadFunc, this,
                                                 kRealtimePriority, threadName);
    if (_ptrThreadPlay == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to create the play audio thread");
        return -1;
    }

    threadID = 0;
    if (!_ptrThreadPlay->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the play audio thread");

        delete _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        return -1;
    }
    _playThreadID = threadID;

    _initialized = true;

    return 0;
}

int32_t AudioDeviceLinuxPulse::Terminate()
{

    if (!_initialized)
    {
        return 0;
    }

    Lock();

    _mixerManager.Close();

    // RECORDING
    if (_ptrThreadRec)
    {
        ThreadWrapper* tmpThread = _ptrThreadRec;
        _ptrThreadRec = NULL;
        UnLock();

        tmpThread->SetNotAlive();
        _timeEventRec.Set();
        if (tmpThread->Stop())
        {
            delete tmpThread;
        } else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the rec audio thread");
        }
        // Lock again since we need to protect _ptrThreadPlay.
        Lock();
    }

    // PLAYOUT
    if (_ptrThreadPlay)
    {
        ThreadWrapper* tmpThread = _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();
        _timeEventPlay.Set();
        if (tmpThread->Stop())
        {
            delete tmpThread;
        } else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the play audio thread");
        }
    } else {
      UnLock();
    }

    // Terminate PulseAudio
    if (TerminatePulseAudio() < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to terminate PulseAudio");
        return -1;
    }

    if (_XDisplay)
    {
      XCloseDisplay(_XDisplay);
      _XDisplay = NULL;
    }

    _initialized = false;
    _outputDeviceIsSpecified = false;
    _inputDeviceIsSpecified = false;

    return 0;
}

bool AudioDeviceLinuxPulse::Initialized() const
{
    return (_initialized);
}

int32_t AudioDeviceLinuxPulse::InitSpeaker()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        return -1;
    }

    if (!_outputDeviceIsSpecified)
    {
        return -1;
    }

    // check if default device
    if (_outputDeviceIndex == 0)
    {
        uint16_t deviceIndex = 0;
        GetDefaultDeviceInfo(false, NULL, deviceIndex);
        _paDeviceIndex = deviceIndex;
    } else
    {
        // get the PA device index from
        // the callback
        _deviceIndex = _outputDeviceIndex;

        // get playout devices
        PlayoutDevices();
    }

    // the callback has now set the _paDeviceIndex to
    // the PulseAudio index of the device
    if (_mixerManager.OpenSpeaker(_paDeviceIndex) == -1)
    {
        return -1;
    }

    // clear _deviceIndex
    _deviceIndex = -1;
    _paDeviceIndex = -1;

    return 0;
}

int32_t AudioDeviceLinuxPulse::InitMicrophone()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    if (!_inputDeviceIsSpecified)
    {
        return -1;
    }

    // Check if default device
    if (_inputDeviceIndex == 0)
    {
        uint16_t deviceIndex = 0;
        GetDefaultDeviceInfo(true, NULL, deviceIndex);
        _paDeviceIndex = deviceIndex;
    } else
    {
        // Get the PA device index from
        // the callback
        _deviceIndex = _inputDeviceIndex;

        // get recording devices
        RecordingDevices();
    }

    // The callback has now set the _paDeviceIndex to
    // the PulseAudio index of the device
    if (_mixerManager.OpenMicrophone(_paDeviceIndex) == -1)
    {
        return -1;
    }

    // Clear _deviceIndex
    _deviceIndex = -1;
    _paDeviceIndex = -1;

    return 0;
}

bool AudioDeviceLinuxPulse::SpeakerIsInitialized() const
{
    return (_mixerManager.SpeakerIsInitialized());
}

bool AudioDeviceLinuxPulse::MicrophoneIsInitialized() const
{
    return (_mixerManager.MicrophoneIsInitialized());
}

int32_t AudioDeviceLinuxPulse::SpeakerVolumeIsAvailable(bool& available)
{

    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    // Make an attempt to open up the
    // output mixer corresponding to the currently selected output device.
    if (!wasInitialized && InitSpeaker() == -1)
    {
        // If we end up here it means that the selected speaker has no volume
        // control.
        available = false;
        return 0;
    }

    // Given that InitSpeaker was successful, we know that a volume control exists
    available = true;

    // Close the initialized output mixer
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

int32_t AudioDeviceLinuxPulse::SetSpeakerVolume(uint32_t volume)
{
    if (!_playing) {
      // Only update the volume if it's been set while we weren't playing.
      update_speaker_volume_at_startup_ = true;
    }
    return (_mixerManager.SetSpeakerVolume(volume));
}

int32_t AudioDeviceLinuxPulse::SpeakerVolume(uint32_t& volume) const
{

    uint32_t level(0);

    if (_mixerManager.SpeakerVolume(level) == -1)
    {
        return -1;
    }

    volume = level;

    return 0;
}

int32_t AudioDeviceLinuxPulse::SetWaveOutVolume(
    uint16_t volumeLeft,
    uint16_t volumeRight)
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceLinuxPulse::WaveOutVolume(
    uint16_t& /*volumeLeft*/,
    uint16_t& /*volumeRight*/) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceLinuxPulse::MaxSpeakerVolume(
    uint32_t& maxVolume) const
{

    uint32_t maxVol(0);

    if (_mixerManager.MaxSpeakerVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;

    return 0;
}

int32_t AudioDeviceLinuxPulse::MinSpeakerVolume(
    uint32_t& minVolume) const
{

    uint32_t minVol(0);

    if (_mixerManager.MinSpeakerVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;

    return 0;
}

int32_t AudioDeviceLinuxPulse::SpeakerVolumeStepSize(
    uint16_t& stepSize) const
{

    uint16_t delta(0);

    if (_mixerManager.SpeakerVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;

    return 0;
}

int32_t AudioDeviceLinuxPulse::SpeakerMuteIsAvailable(bool& available)
{

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    // Make an attempt to open up the
    // output mixer corresponding to the currently selected output device.
    //
    if (!wasInitialized && InitSpeaker() == -1)
    {
        // If we end up here it means that the selected speaker has no volume
        // control, hence it is safe to state that there is no mute control
        // already at this stage.
        available = false;
        return 0;
    }

    // Check if the selected speaker has a mute control
    _mixerManager.SpeakerMuteIsAvailable(isAvailable);

    available = isAvailable;

    // Close the initialized output mixer
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

int32_t AudioDeviceLinuxPulse::SetSpeakerMute(bool enable)
{

    return (_mixerManager.SetSpeakerMute(enable));
}

int32_t AudioDeviceLinuxPulse::SpeakerMute(bool& enabled) const
{

    bool muted(0);
    if (_mixerManager.SpeakerMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}

int32_t AudioDeviceLinuxPulse::MicrophoneMuteIsAvailable(bool& available)
{

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    // Make an attempt to open up the
    // input mixer corresponding to the currently selected input device.
    //
    if (!wasInitialized && InitMicrophone() == -1)
    {
        // If we end up here it means that the selected microphone has no volume
        // control, hence it is safe to state that there is no boost control
        // already at this stage.
        available = false;
        return 0;
    }

    // Check if the selected microphone has a mute control
    //
    _mixerManager.MicrophoneMuteIsAvailable(isAvailable);
    available = isAvailable;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceLinuxPulse::SetMicrophoneMute(bool enable)
{

    return (_mixerManager.SetMicrophoneMute(enable));
}

int32_t AudioDeviceLinuxPulse::MicrophoneMute(bool& enabled) const
{

    bool muted(0);
    if (_mixerManager.MicrophoneMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}

int32_t AudioDeviceLinuxPulse::MicrophoneBoostIsAvailable(bool& available)
{

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    // Enumerate all avaliable microphone and make an attempt to open up the
    // input mixer corresponding to the currently selected input device.
    //
    if (!wasInitialized && InitMicrophone() == -1)
    {
        // If we end up here it means that the selected microphone has no volume
        // control, hence it is safe to state that there is no boost control
        // already at this stage.
        available = false;
        return 0;
    }

    // Check if the selected microphone has a boost control
    _mixerManager.MicrophoneBoostIsAvailable(isAvailable);
    available = isAvailable;

    // Close the initialized input mixer
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceLinuxPulse::SetMicrophoneBoost(bool enable)
{

    return (_mixerManager.SetMicrophoneBoost(enable));
}

int32_t AudioDeviceLinuxPulse::MicrophoneBoost(bool& enabled) const
{

    bool onOff(0);

    if (_mixerManager.MicrophoneBoost(onOff) == -1)
    {
        return -1;
    }

    enabled = onOff;

    return 0;
}

int32_t AudioDeviceLinuxPulse::StereoRecordingIsAvailable(bool& available)
{

    if (_recChannels == 2 && _recording) {
      available = true;
      return 0;
    }

    available = false;
    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();
    int error = 0;

    if (!wasInitialized && InitMicrophone() == -1)
    {
        // Cannot open the specified device
        available = false;
        return 0;
    }

    // Check if the selected microphone can record stereo.
    bool isAvailable(false);
    error = _mixerManager.StereoRecordingIsAvailable(isAvailable);
    if (!error)
      available = isAvailable;

    // Close the initialized input mixer
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return error;
}

int32_t AudioDeviceLinuxPulse::SetStereoRecording(bool enable)
{

    if (enable)
        _recChannels = 2;
    else
        _recChannels = 1;

    return 0;
}

int32_t AudioDeviceLinuxPulse::StereoRecording(bool& enabled) const
{

    if (_recChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

int32_t AudioDeviceLinuxPulse::StereoPlayoutIsAvailable(bool& available)
{

    if (_playChannels == 2 && _playing) {
      available = true;
      return 0;
    }

    available = false;
    bool wasInitialized = _mixerManager.SpeakerIsInitialized();
    int error = 0;

    if (!wasInitialized && InitSpeaker() == -1)
    {
        // Cannot open the specified device.
        return -1;
    }

    // Check if the selected speaker can play stereo.
    bool isAvailable(false);
    error = _mixerManager.StereoPlayoutIsAvailable(isAvailable);
    if (!error)
      available = isAvailable;

    // Close the initialized input mixer
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return error;
}

int32_t AudioDeviceLinuxPulse::SetStereoPlayout(bool enable)
{

    if (enable)
        _playChannels = 2;
    else
        _playChannels = 1;

    return 0;
}

int32_t AudioDeviceLinuxPulse::StereoPlayout(bool& enabled) const
{

    if (_playChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

int32_t AudioDeviceLinuxPulse::SetAGC(bool enable)
{

    _AGC = enable;

    return 0;
}

bool AudioDeviceLinuxPulse::AGC() const
{

    return _AGC;
}

int32_t AudioDeviceLinuxPulse::MicrophoneVolumeIsAvailable(
    bool& available)
{

    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    // Make an attempt to open up the
    // input mixer corresponding to the currently selected output device.
    if (!wasInitialized && InitMicrophone() == -1)
    {
        // If we end up here it means that the selected microphone has no volume
        // control.
        available = false;
        return 0;
    }

    // Given that InitMicrophone was successful, we know that a volume control
    // exists
    available = true;

    // Close the initialized input mixer
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceLinuxPulse::SetMicrophoneVolume(uint32_t volume)
{

    return (_mixerManager.SetMicrophoneVolume(volume));
}

int32_t AudioDeviceLinuxPulse::MicrophoneVolume(
    uint32_t& volume) const
{

    uint32_t level(0);

    if (_mixerManager.MicrophoneVolume(level) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  failed to retrive current microphone level");
        return -1;
    }

    volume = level;

    return 0;
}

int32_t AudioDeviceLinuxPulse::MaxMicrophoneVolume(
    uint32_t& maxVolume) const
{

    uint32_t maxVol(0);

    if (_mixerManager.MaxMicrophoneVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;

    return 0;
}

int32_t AudioDeviceLinuxPulse::MinMicrophoneVolume(
    uint32_t& minVolume) const
{

    uint32_t minVol(0);

    if (_mixerManager.MinMicrophoneVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;

    return 0;
}

int32_t AudioDeviceLinuxPulse::MicrophoneVolumeStepSize(
    uint16_t& stepSize) const
{

    uint16_t delta(0);

    if (_mixerManager.MicrophoneVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;

    return 0;
}

int16_t AudioDeviceLinuxPulse::PlayoutDevices()
{

    PaLock();

    pa_operation* paOperation = NULL;
    _numPlayDevices = 1; // init to 1 to account for "default"

    // get the whole list of devices and update _numPlayDevices
    paOperation = LATE(pa_context_get_sink_info_list)(_paContext,
                                                      PaSinkInfoCallback,
                                                      this);

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    return _numPlayDevices;
}

int32_t AudioDeviceLinuxPulse::SetPlayoutDevice(uint16_t index)
{

    if (_playIsInitialized)
    {
        return -1;
    }

    const uint16_t nDevices = PlayoutDevices();

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  number of availiable output devices is %u", nDevices);

    if (index > (nDevices - 1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  device index is out of range [0,%u]", (nDevices - 1));
        return -1;
    }

    _outputDeviceIndex = index;
    _outputDeviceIsSpecified = true;

    return 0;
}

int32_t AudioDeviceLinuxPulse::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType /*device*/)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

int32_t AudioDeviceLinuxPulse::PlayoutDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    const uint16_t nDevices = PlayoutDevices();

    if ((index > (nDevices - 1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    // Check if default device
    if (index == 0)
    {
        uint16_t deviceIndex = 0;
        return GetDefaultDeviceInfo(false, name, deviceIndex);
    }

    // Tell the callback that we want
    // The name for this device
    _playDisplayDeviceName = name;
    _deviceIndex = index;

    // get playout devices
    PlayoutDevices();

    // clear device name and index
    _playDisplayDeviceName = NULL;
    _deviceIndex = -1;

    return 0;
}

int32_t AudioDeviceLinuxPulse::RecordingDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    const uint16_t nDevices(RecordingDevices());

    if ((index > (nDevices - 1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    // Check if default device
    if (index == 0)
    {
        uint16_t deviceIndex = 0;
        return GetDefaultDeviceInfo(true, name, deviceIndex);
    }

    // Tell the callback that we want
    // the name for this device
    _recDisplayDeviceName = name;
    _deviceIndex = index;

    // Get recording devices
    RecordingDevices();

    // Clear device name and index
    _recDisplayDeviceName = NULL;
    _deviceIndex = -1;

    return 0;
}

int16_t AudioDeviceLinuxPulse::RecordingDevices()
{

    PaLock();

    pa_operation* paOperation = NULL;
    _numRecDevices = 1; // Init to 1 to account for "default"

    // Get the whole list of devices and update _numRecDevices
    paOperation = LATE(pa_context_get_source_info_list)(_paContext,
                                                        PaSourceInfoCallback,
                                                        this);

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    return _numRecDevices;
}

int32_t AudioDeviceLinuxPulse::SetRecordingDevice(uint16_t index)
{

    if (_recIsInitialized)
    {
        return -1;
    }

    const uint16_t nDevices(RecordingDevices());

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  number of availiable input devices is %u", nDevices);

    if (index > (nDevices - 1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  device index is out of range [0,%u]", (nDevices - 1));
        return -1;
    }

    _inputDeviceIndex = index;
    _inputDeviceIsSpecified = true;

    return 0;
}

int32_t AudioDeviceLinuxPulse::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType /*device*/)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

int32_t AudioDeviceLinuxPulse::PlayoutIsAvailable(bool& available)
{

    available = false;

    // Try to initialize the playout side
    int32_t res = InitPlayout();

    // Cancel effect of initialization
    StopPlayout();

    if (res != -1)
    {
        available = true;
    }

    return res;
}

int32_t AudioDeviceLinuxPulse::RecordingIsAvailable(bool& available)
{

    available = false;

    // Try to initialize the playout side
    int32_t res = InitRecording();

    // Cancel effect of initialization
    StopRecording();

    if (res != -1)
    {
        available = true;
    }

    return res;
}

int32_t AudioDeviceLinuxPulse::InitPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        return -1;
    }

    if (!_outputDeviceIsSpecified)
    {
        return -1;
    }

    if (_playIsInitialized)
    {
        return 0;
    }

    // Initialize the speaker (devices might have been added or removed)
    if (InitSpeaker() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitSpeaker() failed");
    }

    // Set the play sample specification
    pa_sample_spec playSampleSpec;
    playSampleSpec.channels = _playChannels;
    playSampleSpec.format = PA_SAMPLE_S16LE;
    playSampleSpec.rate = sample_rate_hz_;

    // Create a new play stream
    _playStream = LATE(pa_stream_new)(_paContext, "playStream",
                                      &playSampleSpec, NULL);

    if (!_playStream)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to create play stream, err=%d",
                     LATE(pa_context_errno)(_paContext));
        return -1;
    }

    // Provide the playStream to the mixer
    _mixerManager.SetPlayStream(_playStream);

    if (_ptrAudioBuffer)
    {
        // Update audio buffer with the selected parameters
        _ptrAudioBuffer->SetPlayoutSampleRate(sample_rate_hz_);
        _ptrAudioBuffer->SetPlayoutChannels((uint8_t) _playChannels);
    }

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  stream state %d\n", LATE(pa_stream_get_state)(_playStream));

    // Set stream flags
    _playStreamFlags = (pa_stream_flags_t) (PA_STREAM_AUTO_TIMING_UPDATE
        | PA_STREAM_INTERPOLATE_TIMING);

    if (_configuredLatencyPlay != WEBRTC_PA_NO_LATENCY_REQUIREMENTS)
    {
        // If configuring a specific latency then we want to specify
        // PA_STREAM_ADJUST_LATENCY to make the server adjust parameters
        // automatically to reach that target latency. However, that flag doesn't
        // exist in Ubuntu 8.04 and many people still use that, so we have to check
        // the protocol version of libpulse.
        if (LATE(pa_context_get_protocol_version)(_paContext)
            >= WEBRTC_PA_ADJUST_LATENCY_PROTOCOL_VERSION)
        {
            _playStreamFlags |= PA_STREAM_ADJUST_LATENCY;
        }

        const pa_sample_spec *spec =
            LATE(pa_stream_get_sample_spec)(_playStream);
        if (!spec)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  pa_stream_get_sample_spec()");
            return -1;
        }

        size_t bytesPerSec = LATE(pa_bytes_per_second)(spec);
        uint32_t latency = bytesPerSec
            * WEBRTC_PA_PLAYBACK_LATENCY_MINIMUM_MSECS / WEBRTC_PA_MSECS_PER_SEC;

        // Set the play buffer attributes
        _playBufferAttr.maxlength = latency; // num bytes stored in the buffer
        _playBufferAttr.tlength = latency; // target fill level of play buffer
        // minimum free num bytes before server request more data
        _playBufferAttr.minreq = latency / WEBRTC_PA_PLAYBACK_REQUEST_FACTOR;
        _playBufferAttr.prebuf = _playBufferAttr.tlength
            - _playBufferAttr.minreq; // prebuffer tlength before starting playout

        _configuredLatencyPlay = latency;
    }

    // num samples in bytes * num channels
    _playbackBufferSize = sample_rate_hz_ / 100 * 2 * _playChannels;
    _playbackBufferUnused = _playbackBufferSize;
    _playBuffer = new int8_t[_playbackBufferSize];

    // Enable underflow callback
    LATE(pa_stream_set_underflow_callback)(_playStream,
                                           PaStreamUnderflowCallback, this);

    // Set the state callback function for the stream
    LATE(pa_stream_set_state_callback)(_playStream, PaStreamStateCallback, this);

    // Mark playout side as initialized
    _playIsInitialized = true;
    _sndCardPlayDelay = 0;
    _sndCardRecDelay = 0;

    return 0;
}

int32_t AudioDeviceLinuxPulse::InitRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    if (!_inputDeviceIsSpecified)
    {
        return -1;
    }

    if (_recIsInitialized)
    {
        return 0;
    }

    // Initialize the microphone (devices might have been added or removed)
    if (InitMicrophone() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitMicrophone() failed");
    }

    // Set the rec sample specification
    pa_sample_spec recSampleSpec;
    recSampleSpec.channels = _recChannels;
    recSampleSpec.format = PA_SAMPLE_S16LE;
    recSampleSpec.rate = sample_rate_hz_;

    // Create a new rec stream
    _recStream = LATE(pa_stream_new)(_paContext, "recStream", &recSampleSpec,
                                     NULL);
    if (!_recStream)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to create rec stream, err=%d",
                     LATE(pa_context_errno)(_paContext));
        return -1;
    }

    // Provide the recStream to the mixer
    _mixerManager.SetRecStream(_recStream);

    if (_ptrAudioBuffer)
    {
        // Update audio buffer with the selected parameters
        _ptrAudioBuffer->SetRecordingSampleRate(sample_rate_hz_);
        _ptrAudioBuffer->SetRecordingChannels((uint8_t) _recChannels);
    }

    if (_configuredLatencyRec != WEBRTC_PA_NO_LATENCY_REQUIREMENTS)
    {
        _recStreamFlags = (pa_stream_flags_t) (PA_STREAM_AUTO_TIMING_UPDATE
            | PA_STREAM_INTERPOLATE_TIMING);

        // If configuring a specific latency then we want to specify
        // PA_STREAM_ADJUST_LATENCY to make the server adjust parameters
        // automatically to reach that target latency. However, that flag doesn't
        // exist in Ubuntu 8.04 and many people still use that, so we have to check
        // the protocol version of libpulse.
        if (LATE(pa_context_get_protocol_version)(_paContext)
            >= WEBRTC_PA_ADJUST_LATENCY_PROTOCOL_VERSION)
        {
            _recStreamFlags |= PA_STREAM_ADJUST_LATENCY;
        }

        const pa_sample_spec *spec =
            LATE(pa_stream_get_sample_spec)(_recStream);
        if (!spec)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  pa_stream_get_sample_spec(rec)");
            return -1;
        }

        size_t bytesPerSec = LATE(pa_bytes_per_second)(spec);
        uint32_t latency = bytesPerSec
            * WEBRTC_PA_LOW_CAPTURE_LATENCY_MSECS / WEBRTC_PA_MSECS_PER_SEC;

        // Set the rec buffer attributes
        // Note: fragsize specifies a maximum transfer size, not a minimum, so
        // it is not possible to force a high latency setting, only a low one.
        _recBufferAttr.fragsize = latency; // size of fragment
        _recBufferAttr.maxlength = latency + bytesPerSec
            * WEBRTC_PA_CAPTURE_BUFFER_EXTRA_MSECS / WEBRTC_PA_MSECS_PER_SEC;

        _configuredLatencyRec = latency;
    }

    _recordBufferSize = sample_rate_hz_ / 100 * 2 * _recChannels;
    _recordBufferUsed = 0;
    _recBuffer = new int8_t[_recordBufferSize];

    // Enable overflow callback
    LATE(pa_stream_set_overflow_callback)(_recStream, PaStreamOverflowCallback,
                                          this);

    // Set the state callback function for the stream
    LATE(pa_stream_set_state_callback)(_recStream, PaStreamStateCallback, this);

    // Mark recording side as initialized
    _recIsInitialized = true;

    return 0;
}

int32_t AudioDeviceLinuxPulse::StartRecording()
{

    if (!_recIsInitialized)
    {
        return -1;
    }

    if (_recording)
    {
        return 0;
    }

    // set state to ensure that the recording starts from the audio thread
    _startRec = true;

    // the audio thread will signal when recording has started
    _timeEventRec.Set();
    if (kEventTimeout == _recStartEvent.Wait(10000))
    {
        {
            CriticalSectionScoped lock(&_critSect);
            _startRec = false;
        }
        StopRecording();
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to activate recording");
        return -1;
    }

    {
        CriticalSectionScoped lock(&_critSect);
        if (_recording)
        {
            // the recording state is set by the audio thread after recording has started
        } else
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to activate recording");
            return -1;
        }
    }

    return 0;
}

int32_t AudioDeviceLinuxPulse::StopRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized)
    {
        return 0;
    }

    if (_recStream == NULL)
    {
        return -1;
    }

    _recIsInitialized = false;
    _recording = false;

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  stopping recording");

    // Stop Recording
    PaLock();

    DisableReadCallback();
    LATE(pa_stream_set_overflow_callback)(_recStream, NULL, NULL);

    // Unset this here so that we don't get a TERMINATED callback
    LATE(pa_stream_set_state_callback)(_recStream, NULL, NULL);

    if (LATE(pa_stream_get_state)(_recStream) != PA_STREAM_UNCONNECTED)
    {
        // Disconnect the stream
        if (LATE(pa_stream_disconnect)(_recStream) != PA_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to disconnect rec stream, err=%d\n",
                         LATE(pa_context_errno)(_paContext));
            PaUnLock();
            return -1;
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  disconnected recording");
    }

    LATE(pa_stream_unref)(_recStream);
    _recStream = NULL;

    PaUnLock();

    // Provide the recStream to the mixer
    _mixerManager.SetRecStream(_recStream);

    if (_recBuffer)
    {
        delete [] _recBuffer;
        _recBuffer = NULL;
    }

    return 0;
}

bool AudioDeviceLinuxPulse::RecordingIsInitialized() const
{
    return (_recIsInitialized);
}

bool AudioDeviceLinuxPulse::Recording() const
{
    CriticalSectionScoped lock(&_critSect);
    return (_recording);
}

bool AudioDeviceLinuxPulse::PlayoutIsInitialized() const
{
    return (_playIsInitialized);
}

int32_t AudioDeviceLinuxPulse::StartPlayout()
{
    if (!_playIsInitialized)
    {
        return -1;
    }

    if (_playing)
    {
        return 0;
    }

    // set state to ensure that playout starts from the audio thread
    _startPlay = true;

    // Both |_startPlay| and |_playing| needs protction since they are also
    // accessed on the playout thread.

    // the audio thread will signal when playout has started
    _timeEventPlay.Set();
    if (kEventTimeout == _playStartEvent.Wait(10000))
    {
        {
            CriticalSectionScoped lock(&_critSect);
            _startPlay = false;
        }
        StopPlayout();
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to activate playout");
        return -1;
    }

    {
        CriticalSectionScoped lock(&_critSect);
        if (_playing)
        {
            // the playing state is set by the audio thread after playout has started
        } else
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to activate playing");
            return -1;
        }
    }

    return 0;
}

int32_t AudioDeviceLinuxPulse::StopPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized)
    {
        return 0;
    }

    if (_playStream == NULL)
    {
        return -1;
    }

    _playIsInitialized = false;
    _playing = false;
    _sndCardPlayDelay = 0;
    _sndCardRecDelay = 0;

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  stopping playback");

    // Stop Playout
    PaLock();

    DisableWriteCallback();
    LATE(pa_stream_set_underflow_callback)(_playStream, NULL, NULL);

    // Unset this here so that we don't get a TERMINATED callback
    LATE(pa_stream_set_state_callback)(_playStream, NULL, NULL);

    if (LATE(pa_stream_get_state)(_playStream) != PA_STREAM_UNCONNECTED)
    {
        // Disconnect the stream
        if (LATE(pa_stream_disconnect)(_playStream) != PA_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to disconnect play stream, err=%d",
                         LATE(pa_context_errno)(_paContext));
            PaUnLock();
            return -1;
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  disconnected playback");
    }

    LATE(pa_stream_unref)(_playStream);
    _playStream = NULL;

    PaUnLock();

    // Provide the playStream to the mixer
    _mixerManager.SetPlayStream(_playStream);

    if (_playBuffer)
    {
        delete [] _playBuffer;
        _playBuffer = NULL;
    }

    return 0;
}

int32_t AudioDeviceLinuxPulse::PlayoutDelay(uint16_t& delayMS) const
{
    CriticalSectionScoped lock(&_critSect);
    delayMS = (uint16_t) _sndCardPlayDelay;
    return 0;
}

int32_t AudioDeviceLinuxPulse::RecordingDelay(uint16_t& delayMS) const
{
    CriticalSectionScoped lock(&_critSect);
    delayMS = (uint16_t) _sndCardRecDelay;
    return 0;
}

bool AudioDeviceLinuxPulse::Playing() const
{
    CriticalSectionScoped lock(&_critSect);
    return (_playing);
}

int32_t AudioDeviceLinuxPulse::SetPlayoutBuffer(
    const AudioDeviceModule::BufferType type,
    uint16_t sizeMS)
{

    if (type != AudioDeviceModule::kFixedBufferSize)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " Adaptive buffer size not supported on this platform");
        return -1;
    }

    _playBufType = type;
    _playBufDelayFixed = sizeMS;

    return 0;
}

int32_t AudioDeviceLinuxPulse::PlayoutBuffer(
    AudioDeviceModule::BufferType& type,
    uint16_t& sizeMS) const
{

    type = _playBufType;
    sizeMS = _playBufDelayFixed;

    return 0;
}

int32_t AudioDeviceLinuxPulse::CPULoad(uint16_t& /*load*/) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

bool AudioDeviceLinuxPulse::PlayoutWarning() const
{
  CriticalSectionScoped lock(&_critSect);
  return (_playWarning > 0);
}

bool AudioDeviceLinuxPulse::PlayoutError() const
{
  CriticalSectionScoped lock(&_critSect);
  return (_playError > 0);
}

bool AudioDeviceLinuxPulse::RecordingWarning() const
{
  CriticalSectionScoped lock(&_critSect);
  return (_recWarning > 0);
}

bool AudioDeviceLinuxPulse::RecordingError() const
{
  CriticalSectionScoped lock(&_critSect);
  return (_recError > 0);
}

void AudioDeviceLinuxPulse::ClearPlayoutWarning()
{
  CriticalSectionScoped lock(&_critSect);
  _playWarning = 0;
}

void AudioDeviceLinuxPulse::ClearPlayoutError()
{
  CriticalSectionScoped lock(&_critSect);
  _playError = 0;
}

void AudioDeviceLinuxPulse::ClearRecordingWarning()
{
  CriticalSectionScoped lock(&_critSect);
  _recWarning = 0;
}

void AudioDeviceLinuxPulse::ClearRecordingError()
{
  CriticalSectionScoped lock(&_critSect);
  _recError = 0;
}

// ============================================================================
//                                 Private Methods
// ============================================================================

void AudioDeviceLinuxPulse::PaContextStateCallback(pa_context *c, void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaContextStateCallbackHandler(
        c);
}

// ----------------------------------------------------------------------------
//  PaSinkInfoCallback
// ----------------------------------------------------------------------------

void AudioDeviceLinuxPulse::PaSinkInfoCallback(pa_context */*c*/,
                                               const pa_sink_info *i, int eol,
                                               void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaSinkInfoCallbackHandler(
        i, eol);
}

void AudioDeviceLinuxPulse::PaSourceInfoCallback(pa_context */*c*/,
                                                 const pa_source_info *i,
                                                 int eol, void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaSourceInfoCallbackHandler(
        i, eol);
}

void AudioDeviceLinuxPulse::PaServerInfoCallback(pa_context */*c*/,
                                                 const pa_server_info *i,
                                                 void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaServerInfoCallbackHandler(i);
}

void AudioDeviceLinuxPulse::PaStreamStateCallback(pa_stream *p, void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaStreamStateCallbackHandler(p);
}

void AudioDeviceLinuxPulse::PaContextStateCallbackHandler(pa_context *c)
{
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  context state cb");

    pa_context_state_t state = LATE(pa_context_get_state)(c);
    switch (state)
    {
        case PA_CONTEXT_UNCONNECTED:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  unconnected");
            break;
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  no state");
            break;
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  failed");
            _paStateChanged = true;
            LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
            break;
        case PA_CONTEXT_READY:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  ready");
            _paStateChanged = true;
            LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
            break;
    }
}

void AudioDeviceLinuxPulse::PaSinkInfoCallbackHandler(const pa_sink_info *i,
                                                      int eol)
{
    if (eol)
    {
        // Signal that we are done
        LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
        return;
    }

    if (_numPlayDevices == _deviceIndex)
    {
        // Convert the device index to the one of the sink
        _paDeviceIndex = i->index;

        if (_playDeviceName)
        {
            // Copy the sink name
            strncpy(_playDeviceName, i->name, kAdmMaxDeviceNameSize);
            _playDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
        }
        if (_playDisplayDeviceName)
        {
            // Copy the sink display name
            strncpy(_playDisplayDeviceName, i->description,
                    kAdmMaxDeviceNameSize);
            _playDisplayDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
        }
    }

    _numPlayDevices++;
}

void AudioDeviceLinuxPulse::PaSourceInfoCallbackHandler(
    const pa_source_info *i,
    int eol)
{
    if (eol)
    {
        // Signal that we are done
        LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
        return;
    }

    // We don't want to list output devices
     if (i->monitor_of_sink == PA_INVALID_INDEX)
    {
        if (_numRecDevices == _deviceIndex)
        {
            // Convert the device index to the one of the source
            _paDeviceIndex = i->index;

            if (_recDeviceName)
            {
                // copy the source name
                strncpy(_recDeviceName, i->name, kAdmMaxDeviceNameSize);
                _recDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
            }
            if (_recDisplayDeviceName)
            {
                // Copy the source display name
                strncpy(_recDisplayDeviceName, i->description,
                        kAdmMaxDeviceNameSize);
                _recDisplayDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
            }
        }

        _numRecDevices++;
    }
}

void AudioDeviceLinuxPulse::PaServerInfoCallbackHandler(const pa_server_info *i)
{
    // Use PA native sampling rate
    sample_rate_hz_ = i->sample_spec.rate;

    // Copy the PA server version
    strncpy(_paServerVersion, i->server_version, 31);
    _paServerVersion[31] = '\0';

    if (_recDisplayDeviceName)
    {
        // Copy the source name
        strncpy(_recDisplayDeviceName, i->default_source_name,
                kAdmMaxDeviceNameSize);
        _recDisplayDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
    }

    if (_playDisplayDeviceName)
    {
        // Copy the sink name
        strncpy(_playDisplayDeviceName, i->default_sink_name,
                kAdmMaxDeviceNameSize);
        _playDisplayDeviceName[kAdmMaxDeviceNameSize - 1] = '\0';
    }

    LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
}

void AudioDeviceLinuxPulse::PaStreamStateCallbackHandler(pa_stream *p)
{
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  stream state cb");

    pa_stream_state_t state = LATE(pa_stream_get_state)(p);
    switch (state)
    {
        case PA_STREAM_UNCONNECTED:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  unconnected");
            break;
        case PA_STREAM_CREATING:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  creating");
            break;
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  failed");
            break;
        case PA_STREAM_READY:
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  ready");
            break;
    }

    LATE(pa_threaded_mainloop_signal)(_paMainloop, 0);
}

int32_t AudioDeviceLinuxPulse::CheckPulseAudioVersion()
{
    PaLock();

    pa_operation* paOperation = NULL;

    // get the server info and update deviceName
    paOperation = LATE(pa_context_get_server_info)(_paContext,
                                                   PaServerInfoCallback, this);

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioDevice, -1,
                 "  checking PulseAudio version: %s", _paServerVersion);

    return 0;
}

int32_t AudioDeviceLinuxPulse::InitSamplingFrequency()
{
    PaLock();

    pa_operation* paOperation = NULL;

    // Get the server info and update sample_rate_hz_
    paOperation = LATE(pa_context_get_server_info)(_paContext,
                                                   PaServerInfoCallback, this);

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    return 0;
}

int32_t AudioDeviceLinuxPulse::GetDefaultDeviceInfo(bool recDevice,
                                                    char* name,
                                                    uint16_t& index)
{
    char tmpName[kAdmMaxDeviceNameSize] = {0};
    // subtract length of "default: "
    uint16_t nameLen = kAdmMaxDeviceNameSize - 9;
    char* pName = NULL;

    if (name)
    {
        // Add "default: "
        strcpy(name, "default: ");
        pName = &name[9];
    }

    // Tell the callback that we want
    // the name for this device
    if (recDevice)
    {
        _recDisplayDeviceName = tmpName;
    } else
    {
        _playDisplayDeviceName = tmpName;
    }

    // Set members
    _paDeviceIndex = -1;
    _deviceIndex = 0;
    _numPlayDevices = 0;
    _numRecDevices = 0;

    PaLock();

    pa_operation* paOperation = NULL;

    // Get the server info and update deviceName
    paOperation = LATE(pa_context_get_server_info)(_paContext,
                                                   PaServerInfoCallback, this);

    WaitForOperationCompletion(paOperation);

    // Get the device index
    if (recDevice)
    {
        paOperation
            = LATE(pa_context_get_source_info_by_name)(_paContext,
                                                       (char *) tmpName,
                                                       PaSourceInfoCallback,
                                                       this);
    } else
    {
        paOperation
            = LATE(pa_context_get_sink_info_by_name)(_paContext,
                                                     (char *) tmpName,
                                                     PaSinkInfoCallback, this);
    }

    WaitForOperationCompletion(paOperation);

    PaUnLock();

    // Set the index
    index = _paDeviceIndex;

    if (name)
    {
        // Copy to name string
        strncpy(pName, tmpName, nameLen);
    }

    // Clear members
    _playDisplayDeviceName = NULL;
    _recDisplayDeviceName = NULL;
    _paDeviceIndex = -1;
    _deviceIndex = -1;
    _numPlayDevices = 0;
    _numRecDevices = 0;

    return 0;
}

int32_t AudioDeviceLinuxPulse::InitPulseAudio()
{
    int retVal = 0;

    // Load libpulse
    if (!PaSymbolTable.Load())
    {
        // Most likely the Pulse library and sound server are not installed on
        // this system
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to load symbol table");
        return -1;
    }

    // Create a mainloop API and connection to the default server
    // the mainloop is the internal asynchronous API event loop
    if (_paMainloop) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  PA mainloop has already existed");
        return -1;
    }
    _paMainloop = LATE(pa_threaded_mainloop_new)();
    if (!_paMainloop)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  could not create mainloop");
        return -1;
    }

    // Start the threaded main loop
    retVal = LATE(pa_threaded_mainloop_start)(_paMainloop);
    if (retVal != PA_OK)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to start main loop, error=%d", retVal);
        return -1;
    }

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  mainloop running!");

    PaLock();

    _paMainloopApi = LATE(pa_threaded_mainloop_get_api)(_paMainloop);
    if (!_paMainloopApi)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  could not create mainloop API");
        PaUnLock();
        return -1;
    }

    // Create a new PulseAudio context
    if (_paContext){
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  PA context has already existed");
        PaUnLock();
        return -1;
    }
    _paContext = LATE(pa_context_new)(_paMainloopApi, "WEBRTC VoiceEngine");

    if (!_paContext)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  could not create context");
        PaUnLock();
        return -1;
    }

    // Set state callback function
    LATE(pa_context_set_state_callback)(_paContext, PaContextStateCallback,
                                        this);

    // Connect the context to a server (default)
    _paStateChanged = false;
    retVal = LATE(pa_context_connect)(_paContext, NULL, PA_CONTEXT_NOAUTOSPAWN,
                                      NULL);

    if (retVal != PA_OK)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to connect context, error=%d", retVal);
        PaUnLock();
        return -1;
    }

    // Wait for state change
    while (!_paStateChanged)
    {
        LATE(pa_threaded_mainloop_wait)(_paMainloop);
    }

    // Now check to see what final state we reached.
    pa_context_state_t state = LATE(pa_context_get_state)(_paContext);

    if (state != PA_CONTEXT_READY)
    {
        if (state == PA_CONTEXT_FAILED)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to connect to PulseAudio sound server");
        } else if (state == PA_CONTEXT_TERMINATED)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  PulseAudio connection terminated early");
        } else
        {
            // Shouldn't happen, because we only signal on one of those three
            // states
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  unknown problem connecting to PulseAudio");
        }
        PaUnLock();
        return -1;
    }

    PaUnLock();

    // Give the objects to the mixer manager
    _mixerManager.SetPulseAudioObjects(_paMainloop, _paContext);

    // Check the version
    if (CheckPulseAudioVersion() < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  PulseAudio version %s not supported", _paServerVersion);
        return -1;
    }

    // Initialize sampling frequency
    if (InitSamplingFrequency() < 0 || sample_rate_hz_ == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to initialize sampling frequency, set to %d Hz",
                     sample_rate_hz_);
        return -1;
    }

    return 0;
}

int32_t AudioDeviceLinuxPulse::TerminatePulseAudio()
{
    // Do nothing if the instance doesn't exist
    // likely PaSymbolTable.Load() fails
    if (!_paMainloop) {
        return 0;
    }

    PaLock();

    // Disconnect the context
    if (_paContext)
    {
        LATE(pa_context_disconnect)(_paContext);
    }

    // Unreference the context
    if (_paContext)
    {
        LATE(pa_context_unref)(_paContext);
    }

    PaUnLock();
    _paContext = NULL;

    // Stop the threaded main loop
    if (_paMainloop)
    {
        LATE(pa_threaded_mainloop_stop)(_paMainloop);
    }

    // Free the mainloop
    if (_paMainloop)
    {
        LATE(pa_threaded_mainloop_free)(_paMainloop);
    }

    _paMainloop = NULL;

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "  PulseAudio terminated");

    return 0;
}

void AudioDeviceLinuxPulse::PaLock()
{
    LATE(pa_threaded_mainloop_lock)(_paMainloop);
}

void AudioDeviceLinuxPulse::PaUnLock()
{
    LATE(pa_threaded_mainloop_unlock)(_paMainloop);
}

void AudioDeviceLinuxPulse::WaitForOperationCompletion(
    pa_operation* paOperation) const
{
    if (!paOperation)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "paOperation NULL in WaitForOperationCompletion");
        return;
    }

    while (LATE(pa_operation_get_state)(paOperation) == PA_OPERATION_RUNNING)
    {
        LATE(pa_threaded_mainloop_wait)(_paMainloop);
    }

    LATE(pa_operation_unref)(paOperation);
}

// ============================================================================
//                                  Thread Methods
// ============================================================================

void AudioDeviceLinuxPulse::EnableWriteCallback()
{
    if (LATE(pa_stream_get_state)(_playStream) == PA_STREAM_READY)
    {
        // May already have available space. Must check.
        _tempBufferSpace = LATE(pa_stream_writable_size)(_playStream);
        if (_tempBufferSpace > 0)
        {
            // Yup, there is already space available, so if we register a write
            // callback then it will not receive any event. So dispatch one ourself
            // instead
            _timeEventPlay.Set();
            return;
        }
    }

    LATE(pa_stream_set_write_callback)(_playStream, &PaStreamWriteCallback,
                                       this);
}

void AudioDeviceLinuxPulse::DisableWriteCallback()
{
    LATE(pa_stream_set_write_callback)(_playStream, NULL, NULL);
}

void AudioDeviceLinuxPulse::PaStreamWriteCallback(pa_stream */*unused*/,
                                                  size_t buffer_space,
                                                  void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaStreamWriteCallbackHandler(
        buffer_space);
}

void AudioDeviceLinuxPulse::PaStreamWriteCallbackHandler(size_t bufferSpace)
{
    _tempBufferSpace = bufferSpace;

    // Since we write the data asynchronously on a different thread, we have
    // to temporarily disable the write callback or else Pulse will call it
    // continuously until we write the data. We re-enable it below.
    DisableWriteCallback();
    _timeEventPlay.Set();
}

void AudioDeviceLinuxPulse::PaStreamUnderflowCallback(pa_stream */*unused*/,
                                                      void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaStreamUnderflowCallbackHandler();
}

void AudioDeviceLinuxPulse::PaStreamUnderflowCallbackHandler()
{
    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  Playout underflow");

    if (_configuredLatencyPlay == WEBRTC_PA_NO_LATENCY_REQUIREMENTS)
    {
        // We didn't configure a pa_buffer_attr before, so switching to one now
        // would be questionable.
        return;
    }

    // Otherwise reconfigure the stream with a higher target latency.

    const pa_sample_spec *spec = LATE(pa_stream_get_sample_spec)(_playStream);
    if (!spec)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  pa_stream_get_sample_spec()");
        return;
    }

    size_t bytesPerSec = LATE(pa_bytes_per_second)(spec);
    uint32_t newLatency = _configuredLatencyPlay + bytesPerSec
        * WEBRTC_PA_PLAYBACK_LATENCY_INCREMENT_MSECS / WEBRTC_PA_MSECS_PER_SEC;

    // Set the play buffer attributes
    _playBufferAttr.maxlength = newLatency;
    _playBufferAttr.tlength = newLatency;
    _playBufferAttr.minreq = newLatency / WEBRTC_PA_PLAYBACK_REQUEST_FACTOR;
    _playBufferAttr.prebuf = _playBufferAttr.tlength - _playBufferAttr.minreq;

    pa_operation *op = LATE(pa_stream_set_buffer_attr)(_playStream,
                                                       &_playBufferAttr, NULL,
                                                       NULL);
    if (!op)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  pa_stream_set_buffer_attr()");
        return;
    }

    // Don't need to wait for this to complete.
    LATE(pa_operation_unref)(op);

    // Save the new latency in case we underflow again.
    _configuredLatencyPlay = newLatency;
}

void AudioDeviceLinuxPulse::EnableReadCallback()
{
    LATE(pa_stream_set_read_callback)(_recStream, &PaStreamReadCallback, this);
}

void AudioDeviceLinuxPulse::DisableReadCallback()
{
    LATE(pa_stream_set_read_callback)(_recStream, NULL, NULL);
}

void AudioDeviceLinuxPulse::PaStreamReadCallback(pa_stream */*unused1*/,
                                                 size_t /*unused2*/,
                                                 void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaStreamReadCallbackHandler();
}

void AudioDeviceLinuxPulse::PaStreamReadCallbackHandler()
{
    // We get the data pointer and size now in order to save one Lock/Unlock
    // in the worker thread
    if (LATE(pa_stream_peek)(_recStream, &_tempSampleData, &_tempSampleDataSize)
        != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Can't read data!");
        return;
    }

    // Since we consume the data asynchronously on a different thread, we have
    // to temporarily disable the read callback or else Pulse will call it
    // continuously until we consume the data. We re-enable it below
    DisableReadCallback();
    _timeEventRec.Set();
}

void AudioDeviceLinuxPulse::PaStreamOverflowCallback(pa_stream */*unused*/,
                                                     void *pThis)
{
    static_cast<AudioDeviceLinuxPulse*> (pThis)->PaStreamOverflowCallbackHandler();
}

void AudioDeviceLinuxPulse::PaStreamOverflowCallbackHandler()
{
    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  Recording overflow");
}

int32_t AudioDeviceLinuxPulse::LatencyUsecs(pa_stream *stream)
{
    if (!WEBRTC_PA_REPORT_LATENCY)
    {
        return 0;
    }

    if (!stream)
    {
        return 0;
    }

    pa_usec_t latency;
    int negative;
    if (LATE(pa_stream_get_latency)(stream, &latency, &negative) != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Can't query latency");
        // We'd rather continue playout/capture with an incorrect delay than stop
        // it altogether, so return a valid value.
        return 0;
    }

    if (negative)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  warning: pa_stream_get_latency reported negative delay");

        // The delay can be negative for monitoring streams if the captured
        // samples haven't been played yet. In such a case, "latency" contains the
        // magnitude, so we must negate it to get the real value.
        int32_t tmpLatency = (int32_t) -latency;
        if (tmpLatency < 0)
        {
            // Make sure that we don't use a negative delay
            tmpLatency = 0;
        }

        return tmpLatency;
    } else
    {
        return (int32_t) latency;
    }
}

int32_t AudioDeviceLinuxPulse::ReadRecordedData(const void* bufferData,
                                                size_t bufferSize)
{
    size_t size = bufferSize;
    uint32_t numRecSamples = _recordBufferSize / (2 * _recChannels);

    // Account for the peeked data and the used data
    uint32_t recDelay = (uint32_t) ((LatencyUsecs(_recStream)
        / 1000) + 10 * ((size + _recordBufferUsed) / _recordBufferSize));

    _sndCardRecDelay = recDelay;

    if (_playStream)
    {
        // Get the playout delay
        _sndCardPlayDelay = (uint32_t) (LatencyUsecs(_playStream) / 1000);
    }

    if (_recordBufferUsed > 0)
    {
        // Have to copy to the buffer until it is full
        size_t copy = _recordBufferSize - _recordBufferUsed;
        if (size < copy)
        {
            copy = size;
        }

        memcpy(&_recBuffer[_recordBufferUsed], bufferData, copy);
        _recordBufferUsed += copy;
        bufferData = static_cast<const char *> (bufferData) + copy;
        size -= copy;

        if (_recordBufferUsed != _recordBufferSize)
        {
            // Not enough data yet to pass to VoE
            return 0;
        }

        // Provide data to VoiceEngine
        if (ProcessRecordedData(_recBuffer, numRecSamples, recDelay) == -1)
        {
            // We have stopped recording
            return -1;
        }

        _recordBufferUsed = 0;
    }

    // Now process full 10ms sample sets directly from the input
    while (size >= _recordBufferSize)
    {
        // Provide data to VoiceEngine
        if (ProcessRecordedData(
            static_cast<int8_t *> (const_cast<void *> (bufferData)),
            numRecSamples, recDelay) == -1)
        {
            // We have stopped recording
            return -1;
        }

        bufferData = static_cast<const char *> (bufferData) + _recordBufferSize;
        size -= _recordBufferSize;

        // We have consumed 10ms of data
        recDelay -= 10;
    }

    // Now save any leftovers for later.
    if (size > 0)
    {
        memcpy(_recBuffer, bufferData, size);
        _recordBufferUsed = size;
    }

    return 0;
}

int32_t AudioDeviceLinuxPulse::ProcessRecordedData(
    int8_t *bufferData,
    uint32_t bufferSizeInSamples,
    uint32_t recDelay) EXCLUSIVE_LOCKS_REQUIRED(_critSect)
{
    uint32_t currentMicLevel(0);
    uint32_t newMicLevel(0);

    _ptrAudioBuffer->SetRecordedBuffer(bufferData, bufferSizeInSamples);

    if (AGC())
    {
        // Store current mic level in the audio buffer if AGC is enabled
        if (MicrophoneVolume(currentMicLevel) == 0)
        {
            // This call does not affect the actual microphone volume
            _ptrAudioBuffer->SetCurrentMicLevel(currentMicLevel);
        }
    }

    const uint32_t clockDrift(0);
    // TODO(andrew): this is a temporary hack, to avoid non-causal far- and
    // near-end signals at the AEC for PulseAudio. I think the system delay is
    // being correctly calculated here, but for legacy reasons we add +10 ms to
    // the value in the AEC. The real fix will be part of a larger investigation
    // into managing system delay in the AEC.
    if (recDelay > 10)
        recDelay -= 10;
    else
        recDelay = 0;
    _ptrAudioBuffer->SetVQEData(_sndCardPlayDelay, recDelay, clockDrift);
    _ptrAudioBuffer->SetTypingStatus(KeyPressed());
    // Deliver recorded samples at specified sample rate,
    // mic level etc. to the observer using callback
    UnLock();
    _ptrAudioBuffer->DeliverRecordedData();
    Lock();

    // We have been unlocked - check the flag again
    if (!_recording)
    {
        return -1;
    }

    if (AGC())
    {
        newMicLevel = _ptrAudioBuffer->NewMicLevel();
        if (newMicLevel != 0)
        {
            // The VQE will only deliver non-zero microphone levels when a
            // change is needed.
            // Set this new mic level (received from the observer as return
            // value in the callback).
            WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                         "  AGC change of volume: old=%u => new=%u",
                         currentMicLevel, newMicLevel);
            if (SetMicrophoneVolume(newMicLevel) == -1)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                             _id,
                             "  the required modification of the microphone "
                             "volume failed");
            }
        }
    }

    return 0;
}

bool AudioDeviceLinuxPulse::PlayThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceLinuxPulse*> (pThis)->PlayThreadProcess());
}

bool AudioDeviceLinuxPulse::RecThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceLinuxPulse*> (pThis)->RecThreadProcess());
}

bool AudioDeviceLinuxPulse::PlayThreadProcess()
{
    switch (_timeEventPlay.Wait(1000))
    {
        case kEventSignaled:
            _timeEventPlay.Reset();
            break;
        case kEventError:
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "EventWrapper::Wait() failed");
            return true;
        case kEventTimeout:
            return true;
    }

    Lock();

    if (_startPlay)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "_startPlay true, performing initial actions");

        _startPlay = false;
        _playDeviceName = NULL;

        // Set if not default device
        if (_outputDeviceIndex > 0)
        {
            // Get the playout device name
            _playDeviceName = new char[kAdmMaxDeviceNameSize];
            _deviceIndex = _outputDeviceIndex;
            PlayoutDevices();
        }

        // Start muted only supported on 0.9.11 and up
        if (LATE(pa_context_get_protocol_version)(_paContext)
            >= WEBRTC_PA_ADJUST_LATENCY_PROTOCOL_VERSION)
        {
            // Get the currently saved speaker mute status
            // and set the initial mute status accordingly
            bool enabled(false);
            _mixerManager.SpeakerMute(enabled);
            if (enabled)
            {
                _playStreamFlags |= PA_STREAM_START_MUTED;
            }
        }

        // Get the currently saved speaker volume
        uint32_t volume = 0;
        if (update_speaker_volume_at_startup_)
          _mixerManager.SpeakerVolume(volume);

        PaLock();

        // NULL gives PA the choice of startup volume.
        pa_cvolume* ptr_cvolume = NULL;
        if (update_speaker_volume_at_startup_) {
          pa_cvolume cVolumes;
          ptr_cvolume = &cVolumes;

          // Set the same volume for all channels
          const pa_sample_spec *spec =
              LATE(pa_stream_get_sample_spec)(_playStream);
          LATE(pa_cvolume_set)(&cVolumes, spec->channels, volume);
          update_speaker_volume_at_startup_ = false;
        }

        // Connect the stream to a sink
        if (LATE(pa_stream_connect_playback)(
            _playStream,
            _playDeviceName,
            &_playBufferAttr,
            (pa_stream_flags_t) _playStreamFlags,
            ptr_cvolume, NULL) != PA_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to connect play stream, err=%d",
                         LATE(pa_context_errno)(_paContext));
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  play stream connected");

        // Wait for state change
        while (LATE(pa_stream_get_state)(_playStream) != PA_STREAM_READY)
        {
            LATE(pa_threaded_mainloop_wait)(_paMainloop);
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  play stream ready");

        // We can now handle write callbacks
        EnableWriteCallback();

        PaUnLock();

        // Clear device name
        if (_playDeviceName)
        {
            delete [] _playDeviceName;
            _playDeviceName = NULL;
        }

        _playing = true;
        _playStartEvent.Set();

        UnLock();
        return true;
    }

    if (_playing)
    {
        if (!_recording)
        {
            // Update the playout delay
            _sndCardPlayDelay = (uint32_t) (LatencyUsecs(_playStream)
                / 1000);
        }

        if (_playbackBufferUnused < _playbackBufferSize)
        {

            size_t write = _playbackBufferSize - _playbackBufferUnused;
            if (_tempBufferSpace < write)
            {
                write = _tempBufferSpace;
            }

            PaLock();
            if (LATE(pa_stream_write)(
                                      _playStream,
                                      (void *) &_playBuffer[_playbackBufferUnused],
                                      write, NULL, (int64_t) 0,
                                      PA_SEEK_RELATIVE) != PA_OK)
            {
                _writeErrors++;
                if (_writeErrors > 10)
                {
                    if (_playError == 1)
                    {
                        WEBRTC_TRACE(kTraceWarning,
                                     kTraceUtility, _id,
                                     "  pending playout error exists");
                    }
                    _playError = 1; // Triggers callback from module process thread
                    WEBRTC_TRACE(
                                 kTraceError,
                                 kTraceUtility,
                                 _id,
                                 "  kPlayoutError message posted: "
                                 "_writeErrors=%u, error=%d",
                                 _writeErrors,
                                 LATE(pa_context_errno)(_paContext));
                    _writeErrors = 0;
                }
            }
            PaUnLock();

            _playbackBufferUnused += write;
            _tempBufferSpace -= write;
        }

        uint32_t numPlaySamples = _playbackBufferSize / (2 * _playChannels);
        if (_tempBufferSpace > 0) // Might have been reduced to zero by the above
        {
            // Ask for new PCM data to be played out using the AudioDeviceBuffer
            // ensure that this callback is executed without taking the
            // audio-thread lock
            UnLock();
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  requesting data");
            uint32_t nSamples =
                _ptrAudioBuffer->RequestPlayoutData(numPlaySamples);
            Lock();

            // We have been unlocked - check the flag again
            if (!_playing)
            {
                UnLock();
                return true;
            }

            nSamples = _ptrAudioBuffer->GetPlayoutData(_playBuffer);
            if (nSamples != numPlaySamples)
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                             _id, "  invalid number of output samples(%d)",
                             nSamples);
            }

            size_t write = _playbackBufferSize;
            if (_tempBufferSpace < write)
            {
                write = _tempBufferSpace;
            }

            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         "  will write");
            PaLock();
            if (LATE(pa_stream_write)(_playStream, (void *) &_playBuffer[0],
                                      write, NULL, (int64_t) 0,
                                      PA_SEEK_RELATIVE) != PA_OK)
            {
                _writeErrors++;
                if (_writeErrors > 10)
                {
                    if (_playError == 1)
                    {
                        WEBRTC_TRACE(kTraceWarning,
                                     kTraceUtility, _id,
                                     "  pending playout error exists");
                    }
                    _playError = 1; // triggers callback from module process thread
                    WEBRTC_TRACE(
                                 kTraceError,
                                 kTraceUtility,
                                 _id,
                                 "  kPlayoutError message posted: "
                                 "_writeErrors=%u, error=%d",
                                 _writeErrors,
                                 LATE(pa_context_errno)(_paContext));
                    _writeErrors = 0;
                }
            }
            PaUnLock();

            _playbackBufferUnused = write;
        }

        _tempBufferSpace = 0;
        PaLock();
        EnableWriteCallback();
        PaUnLock();

    }  // _playing

    UnLock();
    return true;
}

bool AudioDeviceLinuxPulse::RecThreadProcess()
{
    switch (_timeEventRec.Wait(1000))
    {
        case kEventSignaled:
            _timeEventRec.Reset();
            break;
        case kEventError:
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "EventWrapper::Wait() failed");
            return true;
        case kEventTimeout:
            return true;
    }

    Lock();

    if (_startRec)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "_startRec true, performing initial actions");

        _recDeviceName = NULL;

        // Set if not default device
        if (_inputDeviceIndex > 0)
        {
            // Get the recording device name
            _recDeviceName = new char[kAdmMaxDeviceNameSize];
            _deviceIndex = _inputDeviceIndex;
            RecordingDevices();
        }

        PaLock();

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  connecting stream");

        // Connect the stream to a source
        if (LATE(pa_stream_connect_record)(_recStream, _recDeviceName,
                                           &_recBufferAttr,
                                           (pa_stream_flags_t) _recStreamFlags)
            != PA_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  failed to connect rec stream, err=%d",
                         LATE(pa_context_errno)(_paContext));
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  connected");

        // Wait for state change
        while (LATE(pa_stream_get_state)(_recStream) != PA_STREAM_READY)
        {
            LATE(pa_threaded_mainloop_wait)(_paMainloop);
        }

        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "  done");

        // We can now handle read callbacks
        EnableReadCallback();

        PaUnLock();

        // Clear device name
        if (_recDeviceName)
        {
            delete [] _recDeviceName;
            _recDeviceName = NULL;
        }

        _startRec = false;
        _recording = true;
        _recStartEvent.Set();

        UnLock();
        return true;
    }

    if (_recording)
    {
        // Read data and provide it to VoiceEngine
        if (ReadRecordedData(_tempSampleData, _tempSampleDataSize) == -1)
        {
            UnLock();
            return true;
        }

        _tempSampleData = NULL;
        _tempSampleDataSize = 0;

        PaLock();
        while (true)
        {
            // Ack the last thing we read
            if (LATE(pa_stream_drop)(_recStream) != 0)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                             _id, "  failed to drop, err=%d\n",
                             LATE(pa_context_errno)(_paContext));
            }

            if (LATE(pa_stream_readable_size)(_recStream) <= 0)
            {
                // Then that was all the data
                break;
            }

            // Else more data.
            const void *sampleData;
            size_t sampleDataSize;

            if (LATE(pa_stream_peek)(_recStream, &sampleData, &sampleDataSize)
                != 0)
            {
                _recError = 1; // triggers callback from module process thread
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                             _id, "  RECORD_ERROR message posted, error = %d",
                             LATE(pa_context_errno)(_paContext));
                break;
            }

            _sndCardRecDelay = (uint32_t) (LatencyUsecs(_recStream)
                / 1000);

            // Drop lock for sigslot dispatch, which could take a while.
            PaUnLock();
            // Read data and provide it to VoiceEngine
            if (ReadRecordedData(sampleData, sampleDataSize) == -1)
            {
                UnLock();
                return true;
            }
            PaLock();

            // Return to top of loop for the ack and the check for more data.
        }

        EnableReadCallback();
        PaUnLock();

    }  // _recording

    UnLock();
    return true;
}

bool AudioDeviceLinuxPulse::KeyPressed() const{

  char szKey[32];
  unsigned int i = 0;
  char state = 0;

  if (!_XDisplay)
    return false;

  // Check key map status
  XQueryKeymap(_XDisplay, szKey);

  // A bit change in keymap means a key is pressed
  for (i = 0; i < sizeof(szKey); i++)
    state |= (szKey[i] ^ _oldKeyState[i]) & szKey[i];

  // Save old state
  memcpy((char*)_oldKeyState, (char*)szKey, sizeof(_oldKeyState));
  return (state != 0);
}
}
