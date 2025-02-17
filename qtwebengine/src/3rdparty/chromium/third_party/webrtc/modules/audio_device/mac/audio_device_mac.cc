/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_device/audio_device_config.h"
#include "webrtc/modules/audio_device/audio_device_utility.h"
#include "webrtc/modules/audio_device/mac/audio_device_mac.h"

#include "webrtc/modules/audio_device/mac/portaudio/pa_ringbuffer.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

#include <ApplicationServices/ApplicationServices.h>
#include <assert.h>
#include <libkern/OSAtomic.h>   // OSAtomicCompareAndSwap()
#include <mach/mach.h>          // mach_task_self()
#include <sys/sysctl.h>         // sysctlbyname()



namespace webrtc
{

#define WEBRTC_CA_RETURN_ON_ERR(expr)                                   \
    do {                                                                \
        err = expr;                                                     \
        if (err != noErr) {                                             \
            logCAMsg(kTraceError, kTraceAudioDevice, _id,               \
                "Error in " #expr, (const char *)&err);                 \
            return -1;                                                  \
        }                                                               \
    } while(0)

#define WEBRTC_CA_LOG_ERR(expr)                                         \
    do {                                                                \
        err = expr;                                                     \
        if (err != noErr) {                                             \
            logCAMsg(kTraceError, kTraceAudioDevice, _id,               \
                "Error in " #expr, (const char *)&err);                 \
        }                                                               \
    } while(0)

#define WEBRTC_CA_LOG_WARN(expr)                                        \
    do {                                                                \
        err = expr;                                                     \
        if (err != noErr) {                                             \
            logCAMsg(kTraceWarning, kTraceAudioDevice, _id,             \
                "Error in " #expr, (const char *)&err);                 \
        }                                                               \
    } while(0)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

enum
{
    MaxNumberDevices = 64
};

void AudioDeviceMac::AtomicSet32(int32_t* theValue, int32_t newValue)
{
    while (1)
    {
        int32_t oldValue = *theValue;
        if (OSAtomicCompareAndSwap32Barrier(oldValue, newValue, theValue)
            == true)
        {
            return;
        }
    }
}

int32_t AudioDeviceMac::AtomicGet32(int32_t* theValue)
{
    while (1)
    {
        int32_t value = *theValue;
        if (OSAtomicCompareAndSwap32Barrier(value, value, theValue) == true)
        {
            return value;
        }
    }
}

// CoreAudio errors are best interpreted as four character strings.
void AudioDeviceMac::logCAMsg(const TraceLevel level,
                              const TraceModule module,
                              const int32_t id, const char *msg,
                              const char *err)
{
    assert(msg != NULL);
    assert(err != NULL);

#ifdef WEBRTC_ARCH_BIG_ENDIAN
    WEBRTC_TRACE(level, module, id, "%s: %.4s", msg, err);
#else
    // We need to flip the characters in this case.
    WEBRTC_TRACE(level, module, id, "%s: %.1s%.1s%.1s%.1s", msg, err + 3, err
        + 2, err + 1, err);
#endif
}

AudioDeviceMac::AudioDeviceMac(const int32_t id) :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _stopEventRec(*EventWrapper::Create()),
    _stopEvent(*EventWrapper::Create()),
    _captureWorkerThread(NULL),
    _renderWorkerThread(NULL),
    _captureWorkerThreadId(0),
    _renderWorkerThreadId(0),
    _id(id),
    _mixerManager(id),
    _inputDeviceIndex(0),
    _outputDeviceIndex(0),
    _inputDeviceID(kAudioObjectUnknown),
    _outputDeviceID(kAudioObjectUnknown),
    _inputDeviceIsSpecified(false),
    _outputDeviceIsSpecified(false),
    _recChannels(N_REC_CHANNELS),
    _playChannels(N_PLAY_CHANNELS),
    _captureBufData(NULL),
    _renderBufData(NULL),
    _playBufType(AudioDeviceModule::kFixedBufferSize),
    _initialized(false),
    _isShutDown(false),
    _recording(false),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _AGC(false),
    _renderDeviceIsAlive(1),
    _captureDeviceIsAlive(1),
    _twoDevices(true),
    _doStop(false),
    _doStopRec(false),
    _macBookPro(false),
    _macBookProPanRight(false),
    _captureLatencyUs(0),
    _renderLatencyUs(0),
    _captureDelayUs(0),
    _renderDelayUs(0),
    _renderDelayOffsetSamples(0),
    _playBufDelayFixed(20),
    _playWarning(0),
    _playError(0),
    _recWarning(0),
    _recError(0),
    _paCaptureBuffer(NULL),
    _paRenderBuffer(NULL),
    _captureBufSizeSamples(0),
    _renderBufSizeSamples(0),
    prev_key_state_()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);

    assert(&_stopEvent != NULL);
    assert(&_stopEventRec != NULL);

    memset(_renderConvertData, 0, sizeof(_renderConvertData));
    memset(&_outStreamFormat, 0, sizeof(AudioStreamBasicDescription));
    memset(&_outDesiredFormat, 0, sizeof(AudioStreamBasicDescription));
    memset(&_inStreamFormat, 0, sizeof(AudioStreamBasicDescription));
    memset(&_inDesiredFormat, 0, sizeof(AudioStreamBasicDescription));
}


AudioDeviceMac::~AudioDeviceMac()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);

    if (!_isShutDown)
    {
        Terminate();
    }

    if (_captureWorkerThread)
    {
        delete _captureWorkerThread;
        _captureWorkerThread = NULL;
    }

    if (_renderWorkerThread)
    {
        delete _renderWorkerThread;
        _renderWorkerThread = NULL;
    }

    if (_paRenderBuffer)
    {
        delete _paRenderBuffer;
        _paRenderBuffer = NULL;
    }

    if (_paCaptureBuffer)
    {
        delete _paCaptureBuffer;
        _paCaptureBuffer = NULL;
    }

    if (_renderBufData)
    {
        delete[] _renderBufData;
        _renderBufData = NULL;
    }

    if (_captureBufData)
    {
        delete[] _captureBufData;
        _captureBufData = NULL;
    }

    kern_return_t kernErr = KERN_SUCCESS;
    kernErr = semaphore_destroy(mach_task_self(), _renderSemaphore);
    if (kernErr != KERN_SUCCESS)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " semaphore_destroy() error: %d", kernErr);
    }

    kernErr = semaphore_destroy(mach_task_self(), _captureSemaphore);
    if (kernErr != KERN_SUCCESS)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " semaphore_destroy() error: %d", kernErr);
    }

    delete &_stopEvent;
    delete &_stopEventRec;
    delete &_critSect;
}

// ============================================================================
//                                     API
// ============================================================================

void AudioDeviceMac::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{

    CriticalSectionScoped lock(&_critSect);

    _ptrAudioBuffer = audioBuffer;

    // inform the AudioBuffer about default settings for this implementation
    _ptrAudioBuffer->SetRecordingSampleRate(N_REC_SAMPLES_PER_SEC);
    _ptrAudioBuffer->SetPlayoutSampleRate(N_PLAY_SAMPLES_PER_SEC);
    _ptrAudioBuffer->SetRecordingChannels(N_REC_CHANNELS);
    _ptrAudioBuffer->SetPlayoutChannels(N_PLAY_CHANNELS);
}

int32_t AudioDeviceMac::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = AudioDeviceModule::kPlatformDefaultAudio;
    return 0;
}

int32_t AudioDeviceMac::Init()
{

    CriticalSectionScoped lock(&_critSect);

    if (_initialized)
    {
        return 0;
    }

    OSStatus err = noErr;

    _isShutDown = false;

    // PortAudio ring buffers require an elementCount which is a power of two.
    if (_renderBufData == NULL)
    {
        UInt32 powerOfTwo = 1;
        while (powerOfTwo < PLAY_BUF_SIZE_IN_SAMPLES)
        {
            powerOfTwo <<= 1;
        }
        _renderBufSizeSamples = powerOfTwo;
        _renderBufData = new SInt16[_renderBufSizeSamples];
    }

    if (_paRenderBuffer == NULL)
    {
        _paRenderBuffer = new PaUtilRingBuffer;
        ring_buffer_size_t bufSize = -1;
        bufSize = PaUtil_InitializeRingBuffer(_paRenderBuffer, sizeof(SInt16),
                                              _renderBufSizeSamples,
                                              _renderBufData);
        if (bufSize == -1)
        {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice,
                         _id, " PaUtil_InitializeRingBuffer() error");
            return -1;
        }
    }

    if (_captureBufData == NULL)
    {
        UInt32 powerOfTwo = 1;
        while (powerOfTwo < REC_BUF_SIZE_IN_SAMPLES)
        {
            powerOfTwo <<= 1;
        }
        _captureBufSizeSamples = powerOfTwo;
        _captureBufData = new Float32[_captureBufSizeSamples];
    }

    if (_paCaptureBuffer == NULL)
    {
        _paCaptureBuffer = new PaUtilRingBuffer;
        ring_buffer_size_t bufSize = -1;
        bufSize = PaUtil_InitializeRingBuffer(_paCaptureBuffer,
                                              sizeof(Float32),
                                              _captureBufSizeSamples,
                                              _captureBufData);
        if (bufSize == -1)
        {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice,
                         _id, " PaUtil_InitializeRingBuffer() error");
            return -1;
        }
    }

    if (_renderWorkerThread == NULL)
    {
        _renderWorkerThread
            = ThreadWrapper::CreateThread(RunRender, this, kRealtimePriority,
                                          "RenderWorkerThread");
        if (_renderWorkerThread == NULL)
        {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice,
                         _id, " Render CreateThread() error");
            return -1;
        }
    }

    if (_captureWorkerThread == NULL)
    {
        _captureWorkerThread
            = ThreadWrapper::CreateThread(RunCapture, this, kRealtimePriority,
                                          "CaptureWorkerThread");
        if (_captureWorkerThread == NULL)
        {
            WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice,
                         _id, " Capture CreateThread() error");
            return -1;
        }
    }

    kern_return_t kernErr = KERN_SUCCESS;
    kernErr = semaphore_create(mach_task_self(), &_renderSemaphore,
                               SYNC_POLICY_FIFO, 0);
    if (kernErr != KERN_SUCCESS)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     " semaphore_create() error: %d", kernErr);
        return -1;
    }

    kernErr = semaphore_create(mach_task_self(), &_captureSemaphore,
                               SYNC_POLICY_FIFO, 0);
    if (kernErr != KERN_SUCCESS)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     " semaphore_create() error: %d", kernErr);
        return -1;
    }

    // Setting RunLoop to NULL here instructs HAL to manage its own thread for
    // notifications. This was the default behaviour on OS X 10.5 and earlier,
    // but now must be explicitly specified. HAL would otherwise try to use the
    // main thread to issue notifications.
    AudioObjectPropertyAddress propertyAddress = {
            kAudioHardwarePropertyRunLoop,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMaster };
    CFRunLoopRef runLoop = NULL;
    UInt32 size = sizeof(CFRunLoopRef);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectSetPropertyData(kAudioObjectSystemObject,
            &propertyAddress, 0, NULL, size, &runLoop));

    // Listen for any device changes.
    propertyAddress.mSelector = kAudioHardwarePropertyDevices;
    WEBRTC_CA_LOG_ERR(AudioObjectAddPropertyListener(kAudioObjectSystemObject,
            &propertyAddress, &objectListenerProc, this));

    // Determine if this is a MacBook Pro
    _macBookPro = false;
    _macBookProPanRight = false;
    char buf[128];
    size_t length = sizeof(buf);
    memset(buf, 0, length);

    int intErr = sysctlbyname("hw.model", buf, &length, NULL, 0);
    if (intErr != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " Error in sysctlbyname(): %d", err);
    } else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     " Hardware model: %s", buf);
        if (strncmp(buf, "MacBookPro", 10) == 0)
        {
            _macBookPro = true;
        }
    }

    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    _initialized = true;

    return 0;
}

int32_t AudioDeviceMac::Terminate()
{

    if (!_initialized)
    {
        return 0;
    }

    if (_recording)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " Recording must be stopped");
        return -1;
    }

    if (_playing)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " Playback must be stopped");
        return -1;
    }

    _critSect.Enter();

    _mixerManager.Close();

    OSStatus err = noErr;
    int retVal = 0;

    AudioObjectPropertyAddress propertyAddress = {
            kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMaster };
    WEBRTC_CA_LOG_WARN(AudioObjectRemovePropertyListener(kAudioObjectSystemObject,
            &propertyAddress, &objectListenerProc, this));

    err = AudioHardwareUnload();
    if (err != noErr)
    {
        logCAMsg(kTraceError, kTraceAudioDevice, _id,
                 "Error in AudioHardwareUnload()", (const char*) &err);
        retVal = -1;
    }

    _critSect.Leave();

    _isShutDown = true;
    _initialized = false;
    _outputDeviceIsSpecified = false;
    _inputDeviceIsSpecified = false;

    return retVal;
}

bool AudioDeviceMac::Initialized() const
{
    return (_initialized);
}

int32_t AudioDeviceMac::SpeakerIsAvailable(bool& available)
{

    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    // Make an attempt to open up the
    // output mixer corresponding to the currently selected output device.
    //
    if (!wasInitialized && InitSpeaker() == -1)
    {
        available = false;
        return 0;
    }

    // Given that InitSpeaker was successful, we know that a valid speaker
    // exists.
    available = true;

    // Close the initialized output mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

int32_t AudioDeviceMac::InitSpeaker()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        return -1;
    }

    if (InitDevice(_outputDeviceIndex, _outputDeviceID, false) == -1)
    {
        return -1;
    }

    if (_inputDeviceID == _outputDeviceID)
    {
        _twoDevices = false;
    } else
    {
        _twoDevices = true;
    }

    if (_mixerManager.OpenSpeaker(_outputDeviceID) == -1)
    {
        return -1;
    }

    return 0;
}

int32_t AudioDeviceMac::MicrophoneIsAvailable(bool& available)
{

    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    // Make an attempt to open up the
    // input mixer corresponding to the currently selected output device.
    //
    if (!wasInitialized && InitMicrophone() == -1)
    {
        available = false;
        return 0;
    }

    // Given that InitMicrophone was successful, we know that a valid microphone
    // exists.
    available = true;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceMac::InitMicrophone()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    if (InitDevice(_inputDeviceIndex, _inputDeviceID, true) == -1)
    {
        return -1;
    }

    if (_inputDeviceID == _outputDeviceID)
    {
        _twoDevices = false;
    } else
    {
        _twoDevices = true;
    }

    if (_mixerManager.OpenMicrophone(_inputDeviceID) == -1)
    {
        return -1;
    }

    return 0;
}

bool AudioDeviceMac::SpeakerIsInitialized() const
{
    return (_mixerManager.SpeakerIsInitialized());
}

bool AudioDeviceMac::MicrophoneIsInitialized() const
{
    return (_mixerManager.MicrophoneIsInitialized());
}

int32_t AudioDeviceMac::SpeakerVolumeIsAvailable(bool& available)
{

    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    // Make an attempt to open up the
    // output mixer corresponding to the currently selected output device.
    //
    if (!wasInitialized && InitSpeaker() == -1)
    {
        // If we end up here it means that the selected speaker has no volume
        // control.
        available = false;
        return 0;
    }

    // Given that InitSpeaker was successful, we know that a volume control exists
    //
    available = true;

    // Close the initialized output mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

int32_t AudioDeviceMac::SetSpeakerVolume(uint32_t volume)
{

    return (_mixerManager.SetSpeakerVolume(volume));
}

int32_t AudioDeviceMac::SpeakerVolume(uint32_t& volume) const
{

    uint32_t level(0);

    if (_mixerManager.SpeakerVolume(level) == -1)
    {
        return -1;
    }

    volume = level;
    return 0;
}

int32_t AudioDeviceMac::SetWaveOutVolume(uint16_t volumeLeft,
                                         uint16_t volumeRight)
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t
AudioDeviceMac::WaveOutVolume(uint16_t& /*volumeLeft*/,
                              uint16_t& /*volumeRight*/) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

int32_t AudioDeviceMac::MaxSpeakerVolume(uint32_t& maxVolume) const
{

    uint32_t maxVol(0);

    if (_mixerManager.MaxSpeakerVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;
    return 0;
}

int32_t AudioDeviceMac::MinSpeakerVolume(uint32_t& minVolume) const
{

    uint32_t minVol(0);

    if (_mixerManager.MinSpeakerVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;
    return 0;
}

int32_t
AudioDeviceMac::SpeakerVolumeStepSize(uint16_t& stepSize) const
{

    uint16_t delta(0);

    if (_mixerManager.SpeakerVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;
    return 0;
}

int32_t AudioDeviceMac::SpeakerMuteIsAvailable(bool& available)
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
    //
    _mixerManager.SpeakerMuteIsAvailable(isAvailable);

    available = isAvailable;

    // Close the initialized output mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

int32_t AudioDeviceMac::SetSpeakerMute(bool enable)
{
    return (_mixerManager.SetSpeakerMute(enable));
}

int32_t AudioDeviceMac::SpeakerMute(bool& enabled) const
{

    bool muted(0);

    if (_mixerManager.SpeakerMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}

int32_t AudioDeviceMac::MicrophoneMuteIsAvailable(bool& available)
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

int32_t AudioDeviceMac::SetMicrophoneMute(bool enable)
{
    return (_mixerManager.SetMicrophoneMute(enable));
}

int32_t AudioDeviceMac::MicrophoneMute(bool& enabled) const
{

    bool muted(0);

    if (_mixerManager.MicrophoneMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}

int32_t AudioDeviceMac::MicrophoneBoostIsAvailable(bool& available)
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
    //
    _mixerManager.MicrophoneBoostIsAvailable(isAvailable);
    available = isAvailable;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceMac::SetMicrophoneBoost(bool enable)
{

    return (_mixerManager.SetMicrophoneBoost(enable));
}

int32_t AudioDeviceMac::MicrophoneBoost(bool& enabled) const
{

    bool onOff(0);

    if (_mixerManager.MicrophoneBoost(onOff) == -1)
    {
        return -1;
    }

    enabled = onOff;
    return 0;
}

int32_t AudioDeviceMac::StereoRecordingIsAvailable(bool& available)
{

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    if (!wasInitialized && InitMicrophone() == -1)
    {
        // Cannot open the specified device
        available = false;
        return 0;
    }

    // Check if the selected microphone can record stereo
    //
    _mixerManager.StereoRecordingIsAvailable(isAvailable);
    available = isAvailable;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceMac::SetStereoRecording(bool enable)
{

    if (enable)
        _recChannels = 2;
    else
        _recChannels = 1;

    return 0;
}

int32_t AudioDeviceMac::StereoRecording(bool& enabled) const
{

    if (_recChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

int32_t AudioDeviceMac::StereoPlayoutIsAvailable(bool& available)
{

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    if (!wasInitialized && InitSpeaker() == -1)
    {
        // Cannot open the specified device
        available = false;
        return 0;
    }

    // Check if the selected microphone can record stereo
    //
    _mixerManager.StereoPlayoutIsAvailable(isAvailable);
    available = isAvailable;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

int32_t AudioDeviceMac::SetStereoPlayout(bool enable)
{

    if (enable)
        _playChannels = 2;
    else
        _playChannels = 1;

    return 0;
}

int32_t AudioDeviceMac::StereoPlayout(bool& enabled) const
{

    if (_playChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

int32_t AudioDeviceMac::SetAGC(bool enable)
{

    _AGC = enable;

    return 0;
}

bool AudioDeviceMac::AGC() const
{

    return _AGC;
}

int32_t AudioDeviceMac::MicrophoneVolumeIsAvailable(bool& available)
{

    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    // Make an attempt to open up the
    // input mixer corresponding to the currently selected output device.
    //
    if (!wasInitialized && InitMicrophone() == -1)
    {
        // If we end up here it means that the selected microphone has no volume
        // control.
        available = false;
        return 0;
    }

    // Given that InitMicrophone was successful, we know that a volume control
    // exists
    //
    available = true;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

int32_t AudioDeviceMac::SetMicrophoneVolume(uint32_t volume)
{

    return (_mixerManager.SetMicrophoneVolume(volume));
}

int32_t AudioDeviceMac::MicrophoneVolume(uint32_t& volume) const
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

int32_t
AudioDeviceMac::MaxMicrophoneVolume(uint32_t& maxVolume) const
{

    uint32_t maxVol(0);

    if (_mixerManager.MaxMicrophoneVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;
    return 0;
}

int32_t
AudioDeviceMac::MinMicrophoneVolume(uint32_t& minVolume) const
{

    uint32_t minVol(0);

    if (_mixerManager.MinMicrophoneVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;
    return 0;
}

int32_t
AudioDeviceMac::MicrophoneVolumeStepSize(uint16_t& stepSize) const
{

    uint16_t delta(0);

    if (_mixerManager.MicrophoneVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;
    return 0;
}

int16_t AudioDeviceMac::PlayoutDevices()
{

    AudioDeviceID playDevices[MaxNumberDevices];
    return GetNumberDevices(kAudioDevicePropertyScopeOutput, playDevices,
                            MaxNumberDevices);
}

int32_t AudioDeviceMac::SetPlayoutDevice(uint16_t index)
{

    if (_playIsInitialized)
    {
        return -1;
    }

    AudioDeviceID playDevices[MaxNumberDevices];
    uint32_t nDevices = GetNumberDevices(kAudioDevicePropertyScopeOutput,
                                         playDevices, MaxNumberDevices);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  number of availiable waveform-audio output devices is %u",
                 nDevices);

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

int32_t AudioDeviceMac::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType /*device*/)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

int32_t AudioDeviceMac::PlayoutDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    const uint16_t nDevices(PlayoutDevices());

    if ((index > (nDevices - 1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return GetDeviceName(kAudioDevicePropertyScopeOutput, index, name);
}

int32_t AudioDeviceMac::RecordingDeviceName(
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

    return GetDeviceName(kAudioDevicePropertyScopeInput, index, name);
}

int16_t AudioDeviceMac::RecordingDevices()
{

    AudioDeviceID recDevices[MaxNumberDevices];
    return GetNumberDevices(kAudioDevicePropertyScopeInput, recDevices,
                            MaxNumberDevices);
}

int32_t AudioDeviceMac::SetRecordingDevice(uint16_t index)
{

    if (_recIsInitialized)
    {
        return -1;
    }

    AudioDeviceID recDevices[MaxNumberDevices];
    uint32_t nDevices = GetNumberDevices(kAudioDevicePropertyScopeInput,
                                         recDevices, MaxNumberDevices);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  number of availiable waveform-audio input devices is %u",
                 nDevices);

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


int32_t
AudioDeviceMac::SetRecordingDevice(AudioDeviceModule::WindowsDeviceType /*device*/)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

int32_t AudioDeviceMac::PlayoutIsAvailable(bool& available)
{

    available = true;

    // Try to initialize the playout side
    if (InitPlayout() == -1)
    {
        available = false;
    }

    // We destroy the IOProc created by InitPlayout() in implDeviceIOProc().
    // We must actually start playout here in order to have the IOProc
    // deleted by calling StopPlayout().
    if (StartPlayout() == -1)
    {
        available = false;
    }

    // Cancel effect of initialization
    if (StopPlayout() == -1)
    {
        available = false;
    }

    return 0;
}

int32_t AudioDeviceMac::RecordingIsAvailable(bool& available)
{

    available = true;

    // Try to initialize the recording side
    if (InitRecording() == -1)
    {
        available = false;
    }

    // We destroy the IOProc created by InitRecording() in implInDeviceIOProc().
    // We must actually start recording here in order to have the IOProc
    // deleted by calling StopRecording().
    if (StartRecording() == -1)
    {
        available = false;
    }

    // Cancel effect of initialization
    if (StopRecording() == -1)
    {
        available = false;
    }

    return 0;
}

int32_t AudioDeviceMac::InitPlayout()
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

    if (!MicrophoneIsInitialized())
    {
        // Make this call to check if we are using
        // one or two devices (_twoDevices)
        bool available = false;
        if (MicrophoneIsAvailable(available) == -1)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  MicrophoneIsAvailable() failed");
        }
    }

    PaUtil_FlushRingBuffer(_paRenderBuffer);

    OSStatus err = noErr;
    UInt32 size = 0;
    _renderDelayOffsetSamples = 0;
    _renderDelayUs = 0;
    _renderLatencyUs = 0;
    _renderDeviceIsAlive = 1;
    _doStop = false;

    // The internal microphone of a MacBook Pro is located under the left speaker
    // grille. When the internal speakers are in use, we want to fully stereo
    // pan to the right.
    AudioObjectPropertyAddress
        propertyAddress = { kAudioDevicePropertyDataSource,
                kAudioDevicePropertyScopeOutput, 0 };
    if (_macBookPro)
    {
        _macBookProPanRight = false;
        Boolean hasProperty = AudioObjectHasProperty(_outputDeviceID,
                                                     &propertyAddress);
        if (hasProperty)
        {
            UInt32 dataSource = 0;
            size = sizeof(dataSource);
            WEBRTC_CA_LOG_WARN(AudioObjectGetPropertyData(_outputDeviceID,
                    &propertyAddress, 0, NULL, &size, &dataSource));

            if (dataSource == 'ispk')
            {
                _macBookProPanRight = true;
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice,
                             _id,
                             "MacBook Pro using internal speakers; stereo"
                             " panning right");
            } else
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice,
                             _id, "MacBook Pro not using internal speakers");
            }

            // Add a listener to determine if the status changes.
            WEBRTC_CA_LOG_WARN(AudioObjectAddPropertyListener(_outputDeviceID,
                    &propertyAddress, &objectListenerProc, this));
        }
    }

    // Get current stream description
    propertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
    memset(&_outStreamFormat, 0, sizeof(_outStreamFormat));
    size = sizeof(_outStreamFormat);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(_outputDeviceID,
            &propertyAddress, 0, NULL, &size, &_outStreamFormat));

    if (_outStreamFormat.mFormatID != kAudioFormatLinearPCM)
    {
        logCAMsg(kTraceError, kTraceAudioDevice, _id,
                 "Unacceptable output stream format -> mFormatID",
                 (const char *) &_outStreamFormat.mFormatID);
        return -1;
    }

    if (_outStreamFormat.mChannelsPerFrame > N_DEVICE_CHANNELS)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "Too many channels on output device (mChannelsPerFrame = %d)",
            _outStreamFormat.mChannelsPerFrame);
        return -1;
    }

    if (_outStreamFormat.mFormatFlags & kAudioFormatFlagIsNonInterleaved)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "Non-interleaved audio data is not supported.",
                     "AudioHardware streams should not have this format.");
        return -1;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "Ouput stream format:");
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mSampleRate = %f, mChannelsPerFrame = %u",
                 _outStreamFormat.mSampleRate,
                 _outStreamFormat.mChannelsPerFrame);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mBytesPerPacket = %u, mFramesPerPacket = %u",
                 _outStreamFormat.mBytesPerPacket,
                 _outStreamFormat.mFramesPerPacket);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mBytesPerFrame = %u, mBitsPerChannel = %u",
                 _outStreamFormat.mBytesPerFrame,
                 _outStreamFormat.mBitsPerChannel);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mFormatFlags = %u",
                 _outStreamFormat.mFormatFlags);
    logCAMsg(kTraceInfo, kTraceAudioDevice, _id, "mFormatID",
             (const char *) &_outStreamFormat.mFormatID);

    // Our preferred format to work with
    _outDesiredFormat.mSampleRate = N_PLAY_SAMPLES_PER_SEC;
    if (_outStreamFormat.mChannelsPerFrame >= 2 && (_playChannels == 2))
    {
        _outDesiredFormat.mChannelsPerFrame = 2;
    } else
    {
        // Disable stereo playout when we only have one channel on the device.
        _outDesiredFormat.mChannelsPerFrame = 1;
        _playChannels = 1;
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "Stereo playout unavailable on this device");
    }

    if (_ptrAudioBuffer)
    {
        // Update audio buffer with the selected parameters
        _ptrAudioBuffer->SetPlayoutSampleRate(N_PLAY_SAMPLES_PER_SEC);
        _ptrAudioBuffer->SetPlayoutChannels((uint8_t) _playChannels);
    }

    _renderDelayOffsetSamples = _renderBufSizeSamples - N_BUFFERS_OUT
        * ENGINE_PLAY_BUF_SIZE_IN_SAMPLES * _outDesiredFormat.mChannelsPerFrame;

    _outDesiredFormat.mBytesPerPacket = _outDesiredFormat.mChannelsPerFrame
        * sizeof(SInt16);
    _outDesiredFormat.mFramesPerPacket = 1; // In uncompressed audio,
    // a packet is one frame.
    _outDesiredFormat.mBytesPerFrame = _outDesiredFormat.mChannelsPerFrame
        * sizeof(SInt16);
    _outDesiredFormat.mBitsPerChannel = sizeof(SInt16) * 8;

    _outDesiredFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger
        | kLinearPCMFormatFlagIsPacked;
#ifdef WEBRTC_ARCH_BIG_ENDIAN
    _outDesiredFormat.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
    _outDesiredFormat.mFormatID = kAudioFormatLinearPCM;

    WEBRTC_CA_RETURN_ON_ERR(AudioConverterNew(&_outDesiredFormat, &_outStreamFormat,
            &_renderConverter));

    // First try to set buffer size to desired value (_playBufDelayFixed)
    UInt32 bufByteCount = (UInt32)((_outStreamFormat.mSampleRate / 1000.0)
        * _playBufDelayFixed * _outStreamFormat.mChannelsPerFrame
        * sizeof(Float32));
    if (_outStreamFormat.mFramesPerPacket != 0)
    {
        if (bufByteCount % _outStreamFormat.mFramesPerPacket != 0)
        {
            bufByteCount = ((UInt32)(bufByteCount
                / _outStreamFormat.mFramesPerPacket) + 1)
                * _outStreamFormat.mFramesPerPacket;
        }
    }

    // Ensure the buffer size is within the acceptable range provided by the device.
    propertyAddress.mSelector = kAudioDevicePropertyBufferSizeRange;
    AudioValueRange range;
    size = sizeof(range);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(_outputDeviceID,
            &propertyAddress, 0, NULL, &size, &range));
    if (range.mMinimum > bufByteCount)
    {
        bufByteCount = range.mMinimum;
    } else if (range.mMaximum < bufByteCount)
    {
        bufByteCount = range.mMaximum;
    }

    propertyAddress.mSelector = kAudioDevicePropertyBufferSize;
    size = sizeof(bufByteCount);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectSetPropertyData(_outputDeviceID,
            &propertyAddress, 0, NULL, size, &bufByteCount));

    // Get render device latency
    propertyAddress.mSelector = kAudioDevicePropertyLatency;
    UInt32 latency = 0;
    size = sizeof(UInt32);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(_outputDeviceID,
            &propertyAddress, 0, NULL, &size, &latency));
    _renderLatencyUs = (uint32_t) ((1.0e6 * latency)
        / _outStreamFormat.mSampleRate);

    // Get render stream latency
    propertyAddress.mSelector = kAudioDevicePropertyStreams;
    AudioStreamID stream = 0;
    size = sizeof(AudioStreamID);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(_outputDeviceID,
            &propertyAddress, 0, NULL, &size, &stream));
    propertyAddress.mSelector = kAudioStreamPropertyLatency;
    size = sizeof(UInt32);
    latency = 0;
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(_outputDeviceID,
            &propertyAddress, 0, NULL, &size, &latency));
    _renderLatencyUs += (uint32_t) ((1.0e6 * latency)
        / _outStreamFormat.mSampleRate);

    // Listen for format changes
    propertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectAddPropertyListener(_outputDeviceID,
            &propertyAddress, &objectListenerProc, this));

    // Listen for processor overloads
    propertyAddress.mSelector = kAudioDeviceProcessorOverload;
    WEBRTC_CA_LOG_WARN(AudioObjectAddPropertyListener(_outputDeviceID,
            &propertyAddress, &objectListenerProc, this));

    if (_twoDevices || !_recIsInitialized)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioDeviceCreateIOProcID(_outputDeviceID,
                deviceIOProc, this, &_deviceIOProcID));
    }

    // Mark playout side as initialized
    _playIsInitialized = true;

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  initial playout status: _renderDelayOffsetSamples=%d,"
                 " _renderDelayUs=%d, _renderLatencyUs=%d",
                 _renderDelayOffsetSamples, _renderDelayUs, _renderLatencyUs);

    return 0;
}

int32_t AudioDeviceMac::InitRecording()
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

    if (!SpeakerIsInitialized())
    {
        // Make this call to check if we are using
        // one or two devices (_twoDevices)
        bool available = false;
        if (SpeakerIsAvailable(available) == -1)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  SpeakerIsAvailable() failed");
        }
    }

    OSStatus err = noErr;
    UInt32 size = 0;

    PaUtil_FlushRingBuffer(_paCaptureBuffer);

    _captureDelayUs = 0;
    _captureLatencyUs = 0;
    _captureDeviceIsAlive = 1;
    _doStopRec = false;

    // Get current stream description
    AudioObjectPropertyAddress
        propertyAddress = { kAudioDevicePropertyStreamFormat,
                kAudioDevicePropertyScopeInput, 0 };
    memset(&_inStreamFormat, 0, sizeof(_inStreamFormat));
    size = sizeof(_inStreamFormat);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(_inputDeviceID,
            &propertyAddress, 0, NULL, &size, &_inStreamFormat));

    if (_inStreamFormat.mFormatID != kAudioFormatLinearPCM)
    {
        logCAMsg(kTraceError, kTraceAudioDevice, _id,
                 "Unacceptable input stream format -> mFormatID",
                 (const char *) &_inStreamFormat.mFormatID);
        return -1;
    }

    if (_inStreamFormat.mChannelsPerFrame > N_DEVICE_CHANNELS)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "Too many channels on input device (mChannelsPerFrame = %d)",
            _inStreamFormat.mChannelsPerFrame);
        return -1;
    }

    const int io_block_size_samples = _inStreamFormat.mChannelsPerFrame *
        _inStreamFormat.mSampleRate / 100 * N_BLOCKS_IO;
    if (io_block_size_samples > _captureBufSizeSamples)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
            "Input IO block size (%d) is larger than ring buffer (%u)",
            io_block_size_samples, _captureBufSizeSamples);
        return -1;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 " Input stream format:");
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 " mSampleRate = %f, mChannelsPerFrame = %u",
                 _inStreamFormat.mSampleRate, _inStreamFormat.mChannelsPerFrame);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 " mBytesPerPacket = %u, mFramesPerPacket = %u",
                 _inStreamFormat.mBytesPerPacket,
                 _inStreamFormat.mFramesPerPacket);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 " mBytesPerFrame = %u, mBitsPerChannel = %u",
                 _inStreamFormat.mBytesPerFrame,
                 _inStreamFormat.mBitsPerChannel);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 " mFormatFlags = %u",
                 _inStreamFormat.mFormatFlags);
    logCAMsg(kTraceInfo, kTraceAudioDevice, _id, "mFormatID",
             (const char *) &_inStreamFormat.mFormatID);

    // Our preferred format to work with
    if (_inStreamFormat.mChannelsPerFrame >= 2 && (_recChannels == 2))
    {
        _inDesiredFormat.mChannelsPerFrame = 2;
    } else
    {
        // Disable stereo recording when we only have one channel on the device.
        _inDesiredFormat.mChannelsPerFrame = 1;
        _recChannels = 1;
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "Stereo recording unavailable on this device");
    }

    if (_ptrAudioBuffer)
    {
        // Update audio buffer with the selected parameters
        _ptrAudioBuffer->SetRecordingSampleRate(N_REC_SAMPLES_PER_SEC);
        _ptrAudioBuffer->SetRecordingChannels((uint8_t) _recChannels);
    }

    _inDesiredFormat.mSampleRate = N_REC_SAMPLES_PER_SEC;
    _inDesiredFormat.mBytesPerPacket = _inDesiredFormat.mChannelsPerFrame
        * sizeof(SInt16);
    _inDesiredFormat.mFramesPerPacket = 1;
    _inDesiredFormat.mBytesPerFrame = _inDesiredFormat.mChannelsPerFrame
        * sizeof(SInt16);
    _inDesiredFormat.mBitsPerChannel = sizeof(SInt16) * 8;

    _inDesiredFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger
        | kLinearPCMFormatFlagIsPacked;
#ifdef WEBRTC_ARCH_BIG_ENDIAN
    _inDesiredFormat.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
    _inDesiredFormat.mFormatID = kAudioFormatLinearPCM;

    WEBRTC_CA_RETURN_ON_ERR(AudioConverterNew(&_inStreamFormat, &_inDesiredFormat,
            &_captureConverter));

    // First try to set buffer size to desired value (10 ms * N_BLOCKS_IO)
    // TODO(xians): investigate this block.
    UInt32 bufByteCount = (UInt32)((_inStreamFormat.mSampleRate / 1000.0)
        * 10.0 * N_BLOCKS_IO * _inStreamFormat.mChannelsPerFrame
        * sizeof(Float32));
    if (_inStreamFormat.mFramesPerPacket != 0)
    {
        if (bufByteCount % _inStreamFormat.mFramesPerPacket != 0)
        {
            bufByteCount = ((UInt32)(bufByteCount
                / _inStreamFormat.mFramesPerPacket) + 1)
                * _inStreamFormat.mFramesPerPacket;
        }
    }

    // Ensure the buffer size is within the acceptable range provided by the device.
    propertyAddress.mSelector = kAudioDevicePropertyBufferSizeRange;
    AudioValueRange range;
    size = sizeof(range);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(_inputDeviceID,
            &propertyAddress, 0, NULL, &size, &range));
    if (range.mMinimum > bufByteCount)
    {
        bufByteCount = range.mMinimum;
    } else if (range.mMaximum < bufByteCount)
    {
        bufByteCount = range.mMaximum;
    }

    propertyAddress.mSelector = kAudioDevicePropertyBufferSize;
    size = sizeof(bufByteCount);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectSetPropertyData(_inputDeviceID,
            &propertyAddress, 0, NULL, size, &bufByteCount));

    // Get capture device latency
    propertyAddress.mSelector = kAudioDevicePropertyLatency;
    UInt32 latency = 0;
    size = sizeof(UInt32);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(_inputDeviceID,
            &propertyAddress, 0, NULL, &size, &latency));
    _captureLatencyUs = (UInt32)((1.0e6 * latency)
        / _inStreamFormat.mSampleRate);

    // Get capture stream latency
    propertyAddress.mSelector = kAudioDevicePropertyStreams;
    AudioStreamID stream = 0;
    size = sizeof(AudioStreamID);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(_inputDeviceID,
            &propertyAddress, 0, NULL, &size, &stream));
    propertyAddress.mSelector = kAudioStreamPropertyLatency;
    size = sizeof(UInt32);
    latency = 0;
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(_inputDeviceID,
            &propertyAddress, 0, NULL, &size, &latency));
    _captureLatencyUs += (UInt32)((1.0e6 * latency)
        / _inStreamFormat.mSampleRate);

    // Listen for format changes
    // TODO(xians): should we be using kAudioDevicePropertyDeviceHasChanged?
    propertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectAddPropertyListener(_inputDeviceID,
            &propertyAddress, &objectListenerProc, this));

    // Listen for processor overloads
    propertyAddress.mSelector = kAudioDeviceProcessorOverload;
    WEBRTC_CA_LOG_WARN(AudioObjectAddPropertyListener(_inputDeviceID,
            &propertyAddress, &objectListenerProc, this));

    if (_twoDevices)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioDeviceCreateIOProcID(_inputDeviceID,
                inDeviceIOProc, this, &_inDeviceIOProcID));
    } else if (!_playIsInitialized)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioDeviceCreateIOProcID(_inputDeviceID,
                deviceIOProc, this, &_deviceIOProcID));
    }

    // Mark recording side as initialized
    _recIsInitialized = true;

    return 0;
}

int32_t AudioDeviceMac::StartRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized)
    {
        return -1;
    }

    if (_recording)
    {
        return 0;
    }

    if (!_initialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " Recording worker thread has not been started");
        return -1;
    }

    OSStatus err = noErr;

    unsigned int threadID(0);
    if (_captureWorkerThread != NULL)
    {
        _captureWorkerThread->Start(threadID);
    }
    _captureWorkerThreadId = threadID;

    if (_twoDevices)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioDeviceStart(_inputDeviceID, _inDeviceIOProcID));
    } else if (!_playing)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioDeviceStart(_inputDeviceID, _deviceIOProcID));
    }

    _recording = true;

    return 0;
}

int32_t AudioDeviceMac::StopRecording()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_recIsInitialized)
    {
        return 0;
    }

    OSStatus err = noErr;

    // Stop device
    int32_t captureDeviceIsAlive = AtomicGet32(&_captureDeviceIsAlive);
    if (_twoDevices)
    {
        if (_recording && captureDeviceIsAlive == 1)
        {
            _recording = false;
            _doStopRec = true; // Signal to io proc to stop audio device
            _critSect.Leave(); // Cannot be under lock, risk of deadlock
            if (kEventTimeout == _stopEventRec.Wait(2000))
            {
                CriticalSectionScoped critScoped(&_critSect);
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                             " Timed out stopping the capture IOProc. "
                             "We may have failed to detect a device removal.");

                WEBRTC_CA_LOG_WARN(AudioDeviceStop(_inputDeviceID,
                                                   _inDeviceIOProcID));
                WEBRTC_CA_LOG_WARN(
                    AudioDeviceDestroyIOProcID(_inputDeviceID,
                                               _inDeviceIOProcID));
            }
            _critSect.Enter();
            _doStopRec = false;
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         " Recording stopped");
        }
    }
    else
    {
        // We signal a stop for a shared device even when rendering has
        // not yet ended. This is to ensure the IOProc will return early as
        // intended (by checking |_recording|) before accessing
        // resources we free below (e.g. the capture converter).
        //
        // In the case of a shared devcie, the IOProc will verify
        // rendering has ended before stopping itself.
        if (_recording && captureDeviceIsAlive == 1)
        {
            _recording = false;
            _doStop = true; // Signal to io proc to stop audio device
            _critSect.Leave(); // Cannot be under lock, risk of deadlock
            if (kEventTimeout == _stopEvent.Wait(2000))
            {
                CriticalSectionScoped critScoped(&_critSect);
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                             " Timed out stopping the shared IOProc. "
                             "We may have failed to detect a device removal.");

                // We assume rendering on a shared device has stopped as well if
                // the IOProc times out.
                WEBRTC_CA_LOG_WARN(AudioDeviceStop(_outputDeviceID,
                                                   _deviceIOProcID));
                WEBRTC_CA_LOG_WARN(AudioDeviceDestroyIOProcID(_outputDeviceID,
                                                              _deviceIOProcID));
            }
            _critSect.Enter();
            _doStop = false;
            WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                         " Recording stopped (shared)");
        }
    }

    // Setting this signal will allow the worker thread to be stopped.
    AtomicSet32(&_captureDeviceIsAlive, 0);
    _critSect.Leave();
    if (_captureWorkerThread != NULL)
    {
        if (!_captureWorkerThread->Stop())
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         " Timed out waiting for the render worker thread to "
                             "stop.");
        }
    }
    _critSect.Enter();

    WEBRTC_CA_LOG_WARN(AudioConverterDispose(_captureConverter));

    // Remove listeners.
    AudioObjectPropertyAddress
        propertyAddress = { kAudioDevicePropertyStreamFormat,
                kAudioDevicePropertyScopeInput, 0 };
    WEBRTC_CA_LOG_WARN(AudioObjectRemovePropertyListener(_inputDeviceID,
            &propertyAddress, &objectListenerProc, this));

    propertyAddress.mSelector = kAudioDeviceProcessorOverload;
    WEBRTC_CA_LOG_WARN(AudioObjectRemovePropertyListener(_inputDeviceID,
            &propertyAddress, &objectListenerProc, this));

    _recIsInitialized = false;
    _recording = false;

    return 0;
}

bool AudioDeviceMac::RecordingIsInitialized() const
{
    return (_recIsInitialized);
}

bool AudioDeviceMac::Recording() const
{
    return (_recording);
}

bool AudioDeviceMac::PlayoutIsInitialized() const
{
    return (_playIsInitialized);
}

int32_t AudioDeviceMac::StartPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized)
    {
        return -1;
    }

    if (_playing)
    {
        return 0;
    }

    OSStatus err = noErr;

    unsigned int threadID(0);
    if (_renderWorkerThread != NULL)
    {
        _renderWorkerThread->Start(threadID);
    }
    _renderWorkerThreadId = threadID;

    if (_twoDevices || !_recording)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioDeviceStart(_outputDeviceID, _deviceIOProcID));
    }
    _playing = true;

    return 0;
}

int32_t AudioDeviceMac::StopPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized)
    {
        return 0;
    }

    OSStatus err = noErr;

    int32_t renderDeviceIsAlive = AtomicGet32(&_renderDeviceIsAlive);
    if (_playing && renderDeviceIsAlive == 1)
    {
        // We signal a stop for a shared device even when capturing has not
        // yet ended. This is to ensure the IOProc will return early as
        // intended (by checking |_playing|) before accessing resources we
        // free below (e.g. the render converter).
        //
        // In the case of a shared device, the IOProc will verify capturing
        // has ended before stopping itself.
        _playing = false;
        _doStop = true; // Signal to io proc to stop audio device
        _critSect.Leave(); // Cannot be under lock, risk of deadlock
        if (kEventTimeout == _stopEvent.Wait(2000))
        {
            CriticalSectionScoped critScoped(&_critSect);
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         " Timed out stopping the render IOProc. "
                         "We may have failed to detect a device removal.");

            // We assume capturing on a shared device has stopped as well if the
            // IOProc times out.
            WEBRTC_CA_LOG_WARN(AudioDeviceStop(_outputDeviceID,
                                               _deviceIOProcID));
            WEBRTC_CA_LOG_WARN(AudioDeviceDestroyIOProcID(_outputDeviceID,
                                                          _deviceIOProcID));
        }
        _critSect.Enter();
        _doStop = false;
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "Playout stopped");
    }

    // Setting this signal will allow the worker thread to be stopped.
    AtomicSet32(&_renderDeviceIsAlive, 0);
    _critSect.Leave();
    if (_renderWorkerThread != NULL)
    {
        if (!_renderWorkerThread->Stop())
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         " Timed out waiting for the render worker thread to "
                         "stop.");
        }
    }
    _critSect.Enter();

    WEBRTC_CA_LOG_WARN(AudioConverterDispose(_renderConverter));

    // Remove listeners.
    AudioObjectPropertyAddress propertyAddress = {
            kAudioDevicePropertyStreamFormat, kAudioDevicePropertyScopeOutput,
            0 };
    WEBRTC_CA_LOG_WARN(AudioObjectRemovePropertyListener(_outputDeviceID,
            &propertyAddress, &objectListenerProc, this));

    propertyAddress.mSelector = kAudioDeviceProcessorOverload;
    WEBRTC_CA_LOG_WARN(AudioObjectRemovePropertyListener(_outputDeviceID,
            &propertyAddress, &objectListenerProc, this));

    if (_macBookPro)
    {
        Boolean hasProperty = AudioObjectHasProperty(_outputDeviceID,
                                                     &propertyAddress);
        if (hasProperty)
        {
            propertyAddress.mSelector = kAudioDevicePropertyDataSource;
            WEBRTC_CA_LOG_WARN(AudioObjectRemovePropertyListener(_outputDeviceID,
                    &propertyAddress, &objectListenerProc, this));
        }
    }

    _playIsInitialized = false;
    _playing = false;

    return 0;
}

int32_t AudioDeviceMac::PlayoutDelay(uint16_t& delayMS) const
{
    int32_t renderDelayUs = AtomicGet32(&_renderDelayUs);
    delayMS = static_cast<uint16_t> (1e-3 * (renderDelayUs + _renderLatencyUs) +
                                     0.5);
    return 0;
}

int32_t AudioDeviceMac::RecordingDelay(uint16_t& delayMS) const
{
    int32_t captureDelayUs = AtomicGet32(&_captureDelayUs);
    delayMS = static_cast<uint16_t> (1e-3 * (captureDelayUs +
                                             _captureLatencyUs) + 0.5);
    return 0;
}

bool AudioDeviceMac::Playing() const
{
    return (_playing);
}

int32_t AudioDeviceMac::SetPlayoutBuffer(
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

int32_t AudioDeviceMac::PlayoutBuffer(
    AudioDeviceModule::BufferType& type,
    uint16_t& sizeMS) const
{

    type = _playBufType;
    sizeMS = _playBufDelayFixed;

    return 0;
}

// Not implemented for Mac.
int32_t AudioDeviceMac::CPULoad(uint16_t& /*load*/) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");

    return -1;
}

bool AudioDeviceMac::PlayoutWarning() const
{
    return (_playWarning > 0);
}

bool AudioDeviceMac::PlayoutError() const
{
    return (_playError > 0);
}

bool AudioDeviceMac::RecordingWarning() const
{
    return (_recWarning > 0);
}

bool AudioDeviceMac::RecordingError() const
{
    return (_recError > 0);
}

void AudioDeviceMac::ClearPlayoutWarning()
{
    _playWarning = 0;
}

void AudioDeviceMac::ClearPlayoutError()
{
    _playError = 0;
}

void AudioDeviceMac::ClearRecordingWarning()
{
    _recWarning = 0;
}

void AudioDeviceMac::ClearRecordingError()
{
    _recError = 0;
}

// ============================================================================
//                                 Private Methods
// ============================================================================

int32_t
AudioDeviceMac::GetNumberDevices(const AudioObjectPropertyScope scope,
                                 AudioDeviceID scopedDeviceIds[],
                                 const uint32_t deviceListLength)
{
    OSStatus err = noErr;

    AudioObjectPropertyAddress propertyAddress = {
            kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMaster };
    UInt32 size = 0;
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
            &propertyAddress, 0, NULL, &size));
    if (size == 0)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "No devices");
        return 0;
    }

    AudioDeviceID* deviceIds = (AudioDeviceID*) malloc(size);
    UInt32 numberDevices = size / sizeof(AudioDeviceID);
    AudioBufferList* bufferList = NULL;
    UInt32 numberScopedDevices = 0;

    // First check if there is a default device and list it
    UInt32 hardwareProperty = 0;
    if (scope == kAudioDevicePropertyScopeOutput)
    {
        hardwareProperty = kAudioHardwarePropertyDefaultOutputDevice;
    } else
    {
        hardwareProperty = kAudioHardwarePropertyDefaultInputDevice;
    }

    AudioObjectPropertyAddress
        propertyAddressDefault = { hardwareProperty,
                kAudioObjectPropertyScopeGlobal,
                kAudioObjectPropertyElementMaster };

    AudioDeviceID usedID;
    UInt32 uintSize = sizeof(UInt32);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(kAudioObjectSystemObject,
            &propertyAddressDefault, 0, NULL, &uintSize, &usedID));
    if (usedID != kAudioDeviceUnknown)
    {
        scopedDeviceIds[numberScopedDevices] = usedID;
        numberScopedDevices++;
    } else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "GetNumberDevices(): Default device unknown");
    }

    // Then list the rest of the devices
    bool listOK = true;

    WEBRTC_CA_LOG_ERR(AudioObjectGetPropertyData(kAudioObjectSystemObject,
            &propertyAddress, 0, NULL, &size, deviceIds));
    if (err != noErr)
    {
        listOK = false;
    } else
    {
        propertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
        propertyAddress.mScope = scope;
        propertyAddress.mElement = 0;
        for (UInt32 i = 0; i < numberDevices; i++)
        {
            // Check for input channels
            WEBRTC_CA_LOG_ERR(AudioObjectGetPropertyDataSize(deviceIds[i],
                    &propertyAddress, 0, NULL, &size));
            if (err == kAudioHardwareBadDeviceError)
            {
                // This device doesn't actually exist; continue iterating.
                continue;
            } else if (err != noErr)
            {
                listOK = false;
                break;
            }

            bufferList = (AudioBufferList*) malloc(size);
            WEBRTC_CA_LOG_ERR(AudioObjectGetPropertyData(deviceIds[i],
                    &propertyAddress, 0, NULL, &size, bufferList));
            if (err != noErr)
            {
                listOK = false;
                break;
            }

            if (bufferList->mNumberBuffers > 0)
            {
                if (numberScopedDevices >= deviceListLength)
                {
                    WEBRTC_TRACE(kTraceError,
                                 kTraceAudioDevice, _id,
                                 "Device list is not long enough");
                    listOK = false;
                    break;
                }

                scopedDeviceIds[numberScopedDevices] = deviceIds[i];
                numberScopedDevices++;
            }

            free(bufferList);
            bufferList = NULL;
        }  // for
    }

    if (!listOK)
    {
        if (deviceIds)
        {
            free(deviceIds);
            deviceIds = NULL;
        }

        if (bufferList)
        {
            free(bufferList);
            bufferList = NULL;
        }

        return -1;
    }

    // Happy ending
    if (deviceIds)
    {
        free(deviceIds);
        deviceIds = NULL;
    }

    return numberScopedDevices;
}

int32_t
AudioDeviceMac::GetDeviceName(const AudioObjectPropertyScope scope,
                              const uint16_t index,
                              char* name)
{
    OSStatus err = noErr;
    UInt32 len = kAdmMaxDeviceNameSize;
    AudioDeviceID deviceIds[MaxNumberDevices];

    int numberDevices = GetNumberDevices(scope, deviceIds, MaxNumberDevices);
    if (numberDevices < 0)
    {
        return -1;
    } else if (numberDevices == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "No devices");
        return -1;
    }

    // If the number is below the number of devices, assume it's "WEBRTC ID"
    // otherwise assume it's a CoreAudio ID
    AudioDeviceID usedID;

    // Check if there is a default device
    bool isDefaultDevice = false;
    if (index == 0)
    {
        UInt32 hardwareProperty = 0;
        if (scope == kAudioDevicePropertyScopeOutput)
        {
            hardwareProperty = kAudioHardwarePropertyDefaultOutputDevice;
        } else
        {
            hardwareProperty = kAudioHardwarePropertyDefaultInputDevice;
        }
        AudioObjectPropertyAddress propertyAddress = { hardwareProperty,
                kAudioObjectPropertyScopeGlobal,
                kAudioObjectPropertyElementMaster };
        UInt32 size = sizeof(UInt32);
        WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(kAudioObjectSystemObject,
                &propertyAddress, 0, NULL, &size, &usedID));
        if (usedID == kAudioDeviceUnknown)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "GetDeviceName(): Default device unknown");
        } else
        {
            isDefaultDevice = true;
        }
    }

    AudioObjectPropertyAddress propertyAddress = {
            kAudioDevicePropertyDeviceName, scope, 0 };

    if (isDefaultDevice)
    {
        char devName[len];

        WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(usedID,
                &propertyAddress, 0, NULL, &len, devName));

        sprintf(name, "default (%s)", devName);
    } else
    {
        if (index < numberDevices)
        {
            usedID = deviceIds[index];
        } else
        {
            usedID = index;
        }

        WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(usedID,
                &propertyAddress, 0, NULL, &len, name));
    }

    return 0;
}

int32_t AudioDeviceMac::InitDevice(const uint16_t userDeviceIndex,
                                   AudioDeviceID& deviceId,
                                   const bool isInput)
{
    OSStatus err = noErr;
    UInt32 size = 0;
    AudioObjectPropertyScope deviceScope;
    AudioObjectPropertySelector defaultDeviceSelector;
    AudioDeviceID deviceIds[MaxNumberDevices];

    if (isInput)
    {
        deviceScope = kAudioDevicePropertyScopeInput;
        defaultDeviceSelector = kAudioHardwarePropertyDefaultInputDevice;
    } else
    {
        deviceScope = kAudioDevicePropertyScopeOutput;
        defaultDeviceSelector = kAudioHardwarePropertyDefaultOutputDevice;
    }

    AudioObjectPropertyAddress
        propertyAddress = { defaultDeviceSelector,
                kAudioObjectPropertyScopeGlobal,
                kAudioObjectPropertyElementMaster };

    // Get the actual device IDs
    int numberDevices = GetNumberDevices(deviceScope, deviceIds,
                                         MaxNumberDevices);
    if (numberDevices < 0)
    {
        return -1;
    } else if (numberDevices == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "InitDevice(): No devices");
        return -1;
    }

    bool isDefaultDevice = false;
    deviceId = kAudioDeviceUnknown;
    if (userDeviceIndex == 0)
    {
        // Try to use default system device
        size = sizeof(AudioDeviceID);
        WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(kAudioObjectSystemObject,
                &propertyAddress, 0, NULL, &size, &deviceId));
        if (deviceId == kAudioDeviceUnknown)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         " No default device exists");
        } else
        {
            isDefaultDevice = true;
        }
    }

    if (!isDefaultDevice)
    {
        deviceId = deviceIds[userDeviceIndex];
    }

    // Obtain device name and manufacturer for logging.
    // Also use this as a test to ensure a user-set device ID is valid.
    char devName[128];
    char devManf[128];
    memset(devName, 0, sizeof(devName));
    memset(devManf, 0, sizeof(devManf));

    propertyAddress.mSelector = kAudioDevicePropertyDeviceName;
    propertyAddress.mScope = deviceScope;
    propertyAddress.mElement = 0;
    size = sizeof(devName);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(deviceId,
            &propertyAddress, 0, NULL, &size, devName));

    propertyAddress.mSelector = kAudioDevicePropertyDeviceManufacturer;
    size = sizeof(devManf);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(deviceId,
            &propertyAddress, 0, NULL, &size, devManf));

    if (isInput)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     " Input device: %s %s", devManf, devName);
    } else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     " Output device: %s %s", devManf, devName);
    }

    return 0;
}

OSStatus AudioDeviceMac::objectListenerProc(
    AudioObjectID objectId,
    UInt32 numberAddresses,
    const AudioObjectPropertyAddress addresses[],
    void* clientData)
{
    AudioDeviceMac *ptrThis = (AudioDeviceMac *) clientData;
    assert(ptrThis != NULL);

    ptrThis->implObjectListenerProc(objectId, numberAddresses, addresses);

    // AudioObjectPropertyListenerProc functions are supposed to return 0
    return 0;
}

OSStatus AudioDeviceMac::implObjectListenerProc(
    const AudioObjectID objectId,
    const UInt32 numberAddresses,
    const AudioObjectPropertyAddress addresses[])
{
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "AudioDeviceMac::implObjectListenerProc()");

    for (UInt32 i = 0; i < numberAddresses; i++)
    {
        if (addresses[i].mSelector == kAudioHardwarePropertyDevices)
        {
            HandleDeviceChange();
        } else if (addresses[i].mSelector == kAudioDevicePropertyStreamFormat)
        {
            HandleStreamFormatChange(objectId, addresses[i]);
        } else if (addresses[i].mSelector == kAudioDevicePropertyDataSource)
        {
            HandleDataSourceChange(objectId, addresses[i]);
        } else if (addresses[i].mSelector == kAudioDeviceProcessorOverload)
        {
            HandleProcessorOverload(addresses[i]);
        }
    }

    return 0;
}

int32_t AudioDeviceMac::HandleDeviceChange()
{
    OSStatus err = noErr;

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "kAudioHardwarePropertyDevices");

    // A device has changed. Check if our registered devices have been removed.
    // Ensure the devices have been initialized, meaning the IDs are valid.
    if (MicrophoneIsInitialized())
    {
        AudioObjectPropertyAddress propertyAddress = {
                kAudioDevicePropertyDeviceIsAlive,
                kAudioDevicePropertyScopeInput, 0 };
        UInt32 deviceIsAlive = 1;
        UInt32 size = sizeof(UInt32);
        err = AudioObjectGetPropertyData(_inputDeviceID, &propertyAddress, 0,
                                         NULL, &size, &deviceIsAlive);

        if (err == kAudioHardwareBadDeviceError || deviceIsAlive == 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "Capture device is not alive (probably removed)");
            AtomicSet32(&_captureDeviceIsAlive, 0);
            _mixerManager.CloseMicrophone();
            if (_recError == 1)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                             _id, "  pending recording error exists");
            }
            _recError = 1; // triggers callback from module process thread
        } else if (err != noErr)
        {
            logCAMsg(kTraceError, kTraceAudioDevice, _id,
                     "Error in AudioDeviceGetProperty()", (const char*) &err);
            return -1;
        }
    }

    if (SpeakerIsInitialized())
    {
        AudioObjectPropertyAddress propertyAddress = {
                kAudioDevicePropertyDeviceIsAlive,
                kAudioDevicePropertyScopeOutput, 0 };
        UInt32 deviceIsAlive = 1;
        UInt32 size = sizeof(UInt32);
        err = AudioObjectGetPropertyData(_outputDeviceID, &propertyAddress, 0,
                                         NULL, &size, &deviceIsAlive);

        if (err == kAudioHardwareBadDeviceError || deviceIsAlive == 0)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "Render device is not alive (probably removed)");
            AtomicSet32(&_renderDeviceIsAlive, 0);
            _mixerManager.CloseSpeaker();
            if (_playError == 1)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice,
                             _id, "  pending playout error exists");
            }
            _playError = 1; // triggers callback from module process thread
        } else if (err != noErr)
        {
            logCAMsg(kTraceError, kTraceAudioDevice, _id,
                     "Error in AudioDeviceGetProperty()", (const char*) &err);
            return -1;
        }
    }

    return 0;
}

int32_t AudioDeviceMac::HandleStreamFormatChange(
    const AudioObjectID objectId,
    const AudioObjectPropertyAddress propertyAddress)
{
    OSStatus err = noErr;

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "Stream format changed");

    if (objectId != _inputDeviceID && objectId != _outputDeviceID)
    {
        return 0;
    }

    // Get the new device format
    AudioStreamBasicDescription streamFormat;
    UInt32 size = sizeof(streamFormat);
    WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(objectId,
            &propertyAddress, 0, NULL, &size, &streamFormat));

    if (streamFormat.mFormatID != kAudioFormatLinearPCM)
    {
        logCAMsg(kTraceError, kTraceAudioDevice, _id,
                 "Unacceptable input stream format -> mFormatID",
                 (const char *) &streamFormat.mFormatID);
        return -1;
    }

    if (streamFormat.mChannelsPerFrame > N_DEVICE_CHANNELS)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "Too many channels on device (mChannelsPerFrame = %d)",
                     streamFormat.mChannelsPerFrame);
        return -1;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "Stream format:");
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mSampleRate = %f, mChannelsPerFrame = %u",
                 streamFormat.mSampleRate, streamFormat.mChannelsPerFrame);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mBytesPerPacket = %u, mFramesPerPacket = %u",
                 streamFormat.mBytesPerPacket, streamFormat.mFramesPerPacket);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mBytesPerFrame = %u, mBitsPerChannel = %u",
                 streamFormat.mBytesPerFrame, streamFormat.mBitsPerChannel);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mFormatFlags = %u",
                 streamFormat.mFormatFlags);
    logCAMsg(kTraceInfo, kTraceAudioDevice, _id, "mFormatID",
             (const char *) &streamFormat.mFormatID);

    if (propertyAddress.mScope == kAudioDevicePropertyScopeInput)
    {
        const int io_block_size_samples = streamFormat.mChannelsPerFrame *
            streamFormat.mSampleRate / 100 * N_BLOCKS_IO;
        if (io_block_size_samples > _captureBufSizeSamples)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                "Input IO block size (%d) is larger than ring buffer (%u)",
                io_block_size_samples, _captureBufSizeSamples);
            return -1;

        }

        memcpy(&_inStreamFormat, &streamFormat, sizeof(streamFormat));

        if (_inStreamFormat.mChannelsPerFrame >= 2 && (_recChannels == 2))
        {
            _inDesiredFormat.mChannelsPerFrame = 2;
        } else
        {
            // Disable stereo recording when we only have one channel on the device.
            _inDesiredFormat.mChannelsPerFrame = 1;
            _recChannels = 1;
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                         "Stereo recording unavailable on this device");
        }

        if (_ptrAudioBuffer)
        {
            // Update audio buffer with the selected parameters
            _ptrAudioBuffer->SetRecordingSampleRate(N_REC_SAMPLES_PER_SEC);
            _ptrAudioBuffer->SetRecordingChannels((uint8_t) _recChannels);
        }

        // Recreate the converter with the new format
        // TODO(xians): make this thread safe
        WEBRTC_CA_RETURN_ON_ERR(AudioConverterDispose(_captureConverter));

        WEBRTC_CA_RETURN_ON_ERR(AudioConverterNew(&streamFormat, &_inDesiredFormat,
                &_captureConverter));
    } else
    {
        memcpy(&_outStreamFormat, &streamFormat, sizeof(streamFormat));

        if (_outStreamFormat.mChannelsPerFrame >= 2 && (_playChannels == 2))
        {
            _outDesiredFormat.mChannelsPerFrame = 2;
        } else
        {
            // Disable stereo playout when we only have one channel on the device.
            _outDesiredFormat.mChannelsPerFrame = 1;
            _playChannels = 1;
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                         "Stereo playout unavailable on this device");
        }

        if (_ptrAudioBuffer)
        {
            // Update audio buffer with the selected parameters
            _ptrAudioBuffer->SetPlayoutSampleRate(N_PLAY_SAMPLES_PER_SEC);
            _ptrAudioBuffer->SetPlayoutChannels((uint8_t) _playChannels);
        }

        _renderDelayOffsetSamples = _renderBufSizeSamples - N_BUFFERS_OUT
            * ENGINE_PLAY_BUF_SIZE_IN_SAMPLES
            * _outDesiredFormat.mChannelsPerFrame;

        // Recreate the converter with the new format
        // TODO(xians): make this thread safe
        WEBRTC_CA_RETURN_ON_ERR(AudioConverterDispose(_renderConverter));

        WEBRTC_CA_RETURN_ON_ERR(AudioConverterNew(&_outDesiredFormat, &streamFormat,
                &_renderConverter));
    }

    return 0;
}

int32_t AudioDeviceMac::HandleDataSourceChange(
    const AudioObjectID objectId,
    const AudioObjectPropertyAddress propertyAddress)
{
    OSStatus err = noErr;

    if (_macBookPro && propertyAddress.mScope
        == kAudioDevicePropertyScopeOutput)
    {
        WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                     "Data source changed");

        _macBookProPanRight = false;
        UInt32 dataSource = 0;
        UInt32 size = sizeof(UInt32);
        WEBRTC_CA_RETURN_ON_ERR(AudioObjectGetPropertyData(objectId,
                &propertyAddress, 0, NULL, &size, &dataSource));
        if (dataSource == 'ispk')
        {
            _macBookProPanRight = true;
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                         "MacBook Pro using internal speakers; stereo panning right");
        } else
        {
            WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                         "MacBook Pro not using internal speakers");
        }
    }

    return 0;
}
int32_t AudioDeviceMac::HandleProcessorOverload(
    const AudioObjectPropertyAddress propertyAddress)
{
    // TODO(xians): we probably want to notify the user in some way of the
    // overload. However, the Windows interpretations of these errors seem to
    // be more severe than what ProcessorOverload is thrown for.
    //
    // We don't log the notification, as it's sent from the HAL's IO thread. We
    // don't want to slow it down even further.
    if (propertyAddress.mScope == kAudioDevicePropertyScopeInput)
    {
        //WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "Capture processor
        // overload");
        //_callback->ProblemIsReported(
        // SndCardStreamObserver::ERecordingProblem);
    } else
    {
        //WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
        // "Render processor overload");
        //_callback->ProblemIsReported(
        // SndCardStreamObserver::EPlaybackProblem);
    }

    return 0;
}

// ============================================================================
//                                  Thread Methods
// ============================================================================

OSStatus AudioDeviceMac::deviceIOProc(AudioDeviceID, const AudioTimeStamp*,
                                      const AudioBufferList* inputData,
                                      const AudioTimeStamp* inputTime,
                                      AudioBufferList* outputData,
                                      const AudioTimeStamp* outputTime,
                                      void *clientData)
{
    AudioDeviceMac *ptrThis = (AudioDeviceMac *) clientData;
    assert(ptrThis != NULL);

    ptrThis->implDeviceIOProc(inputData, inputTime, outputData, outputTime);

    // AudioDeviceIOProc functions are supposed to return 0
    return 0;
}

OSStatus AudioDeviceMac::outConverterProc(AudioConverterRef,
                                          UInt32 *numberDataPackets,
                                          AudioBufferList *data,
                                          AudioStreamPacketDescription **,
                                          void *userData)
{
    AudioDeviceMac *ptrThis = (AudioDeviceMac *) userData;
    assert(ptrThis != NULL);

    return ptrThis->implOutConverterProc(numberDataPackets, data);
}

OSStatus AudioDeviceMac::inDeviceIOProc(AudioDeviceID, const AudioTimeStamp*,
                                        const AudioBufferList* inputData,
                                        const AudioTimeStamp* inputTime,
                                        AudioBufferList*,
                                        const AudioTimeStamp*, void* clientData)
{
    AudioDeviceMac *ptrThis = (AudioDeviceMac *) clientData;
    assert(ptrThis != NULL);

    ptrThis->implInDeviceIOProc(inputData, inputTime);

    // AudioDeviceIOProc functions are supposed to return 0
    return 0;
}

OSStatus AudioDeviceMac::inConverterProc(
    AudioConverterRef,
    UInt32 *numberDataPackets,
    AudioBufferList *data,
    AudioStreamPacketDescription ** /*dataPacketDescription*/,
    void *userData)
{
    AudioDeviceMac *ptrThis = static_cast<AudioDeviceMac*> (userData);
    assert(ptrThis != NULL);

    return ptrThis->implInConverterProc(numberDataPackets, data);
}

OSStatus AudioDeviceMac::implDeviceIOProc(const AudioBufferList *inputData,
                                          const AudioTimeStamp *inputTime,
                                          AudioBufferList *outputData,
                                          const AudioTimeStamp *outputTime)
{
    OSStatus err = noErr;
    UInt64 outputTimeNs = AudioConvertHostTimeToNanos(outputTime->mHostTime);
    UInt64 nowNs = AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());

    if (!_twoDevices && _recording)
    {
        implInDeviceIOProc(inputData, inputTime);
    }

    // Check if we should close down audio device
    // Double-checked locking optimization to remove locking overhead
    if (_doStop)
    {
        _critSect.Enter();
        if (_doStop)
        {
            if (_twoDevices || (!_recording && !_playing))
            {
               // In the case of a shared device, the single driving ioProc
               // is stopped here
               WEBRTC_CA_LOG_ERR(AudioDeviceStop(_outputDeviceID,
                                                 _deviceIOProcID));
               WEBRTC_CA_LOG_WARN(AudioDeviceDestroyIOProcID(_outputDeviceID,
                                                             _deviceIOProcID));
               if (err == noErr)
               {
                  WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                               _id, " Playout or shared device stopped");
               }
            }

            _doStop = false;
            _stopEvent.Set();
            _critSect.Leave();
            return 0;
        }
        _critSect.Leave();
    }

    if (!_playing)
    {
        // This can be the case when a shared device is capturing but not
        // rendering. We allow the checks above before returning to avoid a
        // timeout when capturing is stopped.
        return 0;
    }

    assert(_outStreamFormat.mBytesPerFrame != 0);
    UInt32 size = outputData->mBuffers->mDataByteSize
        / _outStreamFormat.mBytesPerFrame;

    // TODO(xians): signal an error somehow?
    err = AudioConverterFillComplexBuffer(_renderConverter, outConverterProc,
                                          this, &size, outputData, NULL);
    if (err != noErr)
    {
        if (err == 1)
        {
            // This is our own error.
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         " Error in AudioConverterFillComplexBuffer()");
            return 1;
        } else
        {
            logCAMsg(kTraceError, kTraceAudioDevice, _id,
                     "Error in AudioConverterFillComplexBuffer()",
                     (const char *) &err);
            return 1;
        }
    }

    ring_buffer_size_t bufSizeSamples =
        PaUtil_GetRingBufferReadAvailable(_paRenderBuffer);

    int32_t renderDelayUs = static_cast<int32_t> (1e-3 * (outputTimeNs - nowNs)
        + 0.5);
    renderDelayUs += static_cast<int32_t> ((1.0e6 * bufSizeSamples)
        / _outDesiredFormat.mChannelsPerFrame / _outDesiredFormat.mSampleRate
        + 0.5);

    AtomicSet32(&_renderDelayUs, renderDelayUs);

    return 0;
}

OSStatus AudioDeviceMac::implOutConverterProc(UInt32 *numberDataPackets,
                                              AudioBufferList *data)
{
    assert(data->mNumberBuffers == 1);
    ring_buffer_size_t numSamples = *numberDataPackets
        * _outDesiredFormat.mChannelsPerFrame;

    data->mBuffers->mNumberChannels = _outDesiredFormat.mChannelsPerFrame;
    // Always give the converter as much as it wants, zero padding as required.
    data->mBuffers->mDataByteSize = *numberDataPackets
        * _outDesiredFormat.mBytesPerPacket;
    data->mBuffers->mData = _renderConvertData;
    memset(_renderConvertData, 0, sizeof(_renderConvertData));

    PaUtil_ReadRingBuffer(_paRenderBuffer, _renderConvertData, numSamples);

    kern_return_t kernErr = semaphore_signal_all(_renderSemaphore);
    if (kernErr != KERN_SUCCESS)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " semaphore_signal_all() error: %d", kernErr);
        return 1;
    }

    return 0;
}

OSStatus AudioDeviceMac::implInDeviceIOProc(const AudioBufferList *inputData,
                                            const AudioTimeStamp *inputTime)
{
    OSStatus err = noErr;
    UInt64 inputTimeNs = AudioConvertHostTimeToNanos(inputTime->mHostTime);
    UInt64 nowNs = AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());

    // Check if we should close down audio device
    // Double-checked locking optimization to remove locking overhead
    if (_doStopRec)
    {
        _critSect.Enter();
        if (_doStopRec)
        {
            // This will be signalled only when a shared device is not in use.
            WEBRTC_CA_LOG_ERR(AudioDeviceStop(_inputDeviceID, _inDeviceIOProcID));
            WEBRTC_CA_LOG_WARN(AudioDeviceDestroyIOProcID(_inputDeviceID,
                                                          _inDeviceIOProcID));
            if (err == noErr)
            {
                WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice,
                             _id, " Recording device stopped");
            }

            _doStopRec = false;
            _stopEventRec.Set();
            _critSect.Leave();
            return 0;
        }
        _critSect.Leave();
    }

    if (!_recording)
    {
        // Allow above checks to avoid a timeout on stopping capture.
        return 0;
    }

    ring_buffer_size_t bufSizeSamples =
        PaUtil_GetRingBufferReadAvailable(_paCaptureBuffer);

    int32_t captureDelayUs = static_cast<int32_t> (1e-3 * (nowNs - inputTimeNs)
        + 0.5);
    captureDelayUs
        += static_cast<int32_t> ((1.0e6 * bufSizeSamples)
            / _inStreamFormat.mChannelsPerFrame / _inStreamFormat.mSampleRate
            + 0.5);

    AtomicSet32(&_captureDelayUs, captureDelayUs);

    assert(inputData->mNumberBuffers == 1);
    ring_buffer_size_t numSamples = inputData->mBuffers->mDataByteSize
        * _inStreamFormat.mChannelsPerFrame / _inStreamFormat.mBytesPerPacket;
    PaUtil_WriteRingBuffer(_paCaptureBuffer, inputData->mBuffers->mData,
                           numSamples);

    kern_return_t kernErr = semaphore_signal_all(_captureSemaphore);
    if (kernErr != KERN_SUCCESS)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " semaphore_signal_all() error: %d", kernErr);
    }

    return err;
}

OSStatus AudioDeviceMac::implInConverterProc(UInt32 *numberDataPackets,
                                             AudioBufferList *data)
{
    assert(data->mNumberBuffers == 1);
    ring_buffer_size_t numSamples = *numberDataPackets
        * _inStreamFormat.mChannelsPerFrame;

    while (PaUtil_GetRingBufferReadAvailable(_paCaptureBuffer) < numSamples)
    {
        mach_timespec_t timeout;
        timeout.tv_sec = 0;
        timeout.tv_nsec = TIMER_PERIOD_MS;

        kern_return_t kernErr = semaphore_timedwait(_captureSemaphore, timeout);
        if (kernErr == KERN_OPERATION_TIMED_OUT)
        {
            int32_t signal = AtomicGet32(&_captureDeviceIsAlive);
            if (signal == 0)
            {
                // The capture device is no longer alive; stop the worker thread.
                *numberDataPackets = 0;
                return 1;
            }
        } else if (kernErr != KERN_SUCCESS)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         " semaphore_wait() error: %d", kernErr);
        }
    }

    // Pass the read pointer directly to the converter to avoid a memcpy.
    void* dummyPtr;
    ring_buffer_size_t dummySize;
    PaUtil_GetRingBufferReadRegions(_paCaptureBuffer, numSamples,
                                    &data->mBuffers->mData, &numSamples,
                                    &dummyPtr, &dummySize);
    PaUtil_AdvanceRingBufferReadIndex(_paCaptureBuffer, numSamples);

    data->mBuffers->mNumberChannels = _inStreamFormat.mChannelsPerFrame;
    *numberDataPackets = numSamples / _inStreamFormat.mChannelsPerFrame;
    data->mBuffers->mDataByteSize = *numberDataPackets
        * _inStreamFormat.mBytesPerPacket;

    return 0;
}

bool AudioDeviceMac::RunRender(void* ptrThis)
{
    return static_cast<AudioDeviceMac*> (ptrThis)->RenderWorkerThread();
}

bool AudioDeviceMac::RenderWorkerThread()
{
    ring_buffer_size_t numSamples = ENGINE_PLAY_BUF_SIZE_IN_SAMPLES
        * _outDesiredFormat.mChannelsPerFrame;
    while (PaUtil_GetRingBufferWriteAvailable(_paRenderBuffer)
        - _renderDelayOffsetSamples < numSamples)
    {
        mach_timespec_t timeout;
        timeout.tv_sec = 0;
        timeout.tv_nsec = TIMER_PERIOD_MS;

        kern_return_t kernErr = semaphore_timedwait(_renderSemaphore, timeout);
        if (kernErr == KERN_OPERATION_TIMED_OUT)
        {
            int32_t signal = AtomicGet32(&_renderDeviceIsAlive);
            if (signal == 0)
            {
                // The render device is no longer alive; stop the worker thread.
                return false;
            }
        } else if (kernErr != KERN_SUCCESS)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         " semaphore_timedwait() error: %d", kernErr);
        }
    }

    int8_t playBuffer[4 * ENGINE_PLAY_BUF_SIZE_IN_SAMPLES];

    if (!_ptrAudioBuffer)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  capture AudioBuffer is invalid");
        return false;
    }

    // Ask for new PCM data to be played out using the AudioDeviceBuffer.
    uint32_t nSamples =
        _ptrAudioBuffer->RequestPlayoutData(ENGINE_PLAY_BUF_SIZE_IN_SAMPLES);

    nSamples = _ptrAudioBuffer->GetPlayoutData(playBuffer);
    if (nSamples != ENGINE_PLAY_BUF_SIZE_IN_SAMPLES)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  invalid number of output samples(%d)", nSamples);
    }

    uint32_t nOutSamples = nSamples * _outDesiredFormat.mChannelsPerFrame;

    SInt16 *pPlayBuffer = (SInt16 *) &playBuffer;
    if (_macBookProPanRight && (_playChannels == 2))
    {
        // Mix entirely into the right channel and zero the left channel.
        SInt32 sampleInt32 = 0;
        for (uint32_t sampleIdx = 0; sampleIdx < nOutSamples; sampleIdx
            += 2)
        {
            sampleInt32 = pPlayBuffer[sampleIdx];
            sampleInt32 += pPlayBuffer[sampleIdx + 1];
            sampleInt32 /= 2;

            if (sampleInt32 > 32767)
            {
                sampleInt32 = 32767;
            } else if (sampleInt32 < -32768)
            {
                sampleInt32 = -32768;
            }

            pPlayBuffer[sampleIdx] = 0;
            pPlayBuffer[sampleIdx + 1] = static_cast<SInt16> (sampleInt32);
        }
    }

    PaUtil_WriteRingBuffer(_paRenderBuffer, pPlayBuffer, nOutSamples);

    return true;
}

bool AudioDeviceMac::RunCapture(void* ptrThis)
{
    return static_cast<AudioDeviceMac*> (ptrThis)->CaptureWorkerThread();
}

bool AudioDeviceMac::CaptureWorkerThread()
{
    OSStatus err = noErr;
    UInt32 noRecSamples = ENGINE_REC_BUF_SIZE_IN_SAMPLES
        * _inDesiredFormat.mChannelsPerFrame;
    SInt16 recordBuffer[noRecSamples];
    UInt32 size = ENGINE_REC_BUF_SIZE_IN_SAMPLES;

    AudioBufferList engineBuffer;
    engineBuffer.mNumberBuffers = 1; // Interleaved channels.
    engineBuffer.mBuffers->mNumberChannels = _inDesiredFormat.mChannelsPerFrame;
    engineBuffer.mBuffers->mDataByteSize = _inDesiredFormat.mBytesPerPacket
        * noRecSamples;
    engineBuffer.mBuffers->mData = recordBuffer;

    err = AudioConverterFillComplexBuffer(_captureConverter, inConverterProc,
                                          this, &size, &engineBuffer, NULL);
    if (err != noErr)
    {
        if (err == 1)
        {
            // This is our own error.
            return false;
        } else
        {
            logCAMsg(kTraceError, kTraceAudioDevice, _id,
                     "Error in AudioConverterFillComplexBuffer()",
                     (const char *) &err);
            return false;
        }
    }

    // TODO(xians): what if the returned size is incorrect?
    if (size == ENGINE_REC_BUF_SIZE_IN_SAMPLES)
    {
        uint32_t currentMicLevel(0);
        uint32_t newMicLevel(0);
        int32_t msecOnPlaySide;
        int32_t msecOnRecordSide;

        int32_t captureDelayUs = AtomicGet32(&_captureDelayUs);
        int32_t renderDelayUs = AtomicGet32(&_renderDelayUs);

        msecOnPlaySide = static_cast<int32_t> (1e-3 * (renderDelayUs +
                                                       _renderLatencyUs) + 0.5);
        msecOnRecordSide = static_cast<int32_t> (1e-3 * (captureDelayUs +
                                                         _captureLatencyUs) +
                                                 0.5);

        if (!_ptrAudioBuffer)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  capture AudioBuffer is invalid");
            return false;
        }

        // store the recorded buffer (no action will be taken if the
        // #recorded samples is not a full buffer)
        _ptrAudioBuffer->SetRecordedBuffer((int8_t*) &recordBuffer,
                                           (uint32_t) size);

        if (AGC())
        {
            // store current mic level in the audio buffer if AGC is enabled
            if (MicrophoneVolume(currentMicLevel) == 0)
            {
                // this call does not affect the actual microphone volume
                _ptrAudioBuffer->SetCurrentMicLevel(currentMicLevel);
            }
        }

        _ptrAudioBuffer->SetVQEData(msecOnPlaySide, msecOnRecordSide, 0);

        _ptrAudioBuffer->SetTypingStatus(KeyPressed());

        // deliver recorded samples at specified sample rate, mic level etc.
        // to the observer using callback
        _ptrAudioBuffer->DeliverRecordedData();

        if (AGC())
        {
            newMicLevel = _ptrAudioBuffer->NewMicLevel();
            if (newMicLevel != 0)
            {
                // The VQE will only deliver non-zero microphone levels when
                // a change is needed.
                // Set this new mic level (received from the observer as return
                // value in the callback).
                WEBRTC_TRACE(kTraceStream, kTraceAudioDevice,
                             _id, "  AGC change of volume: old=%u => new=%u",
                             currentMicLevel, newMicLevel);
                if (SetMicrophoneVolume(newMicLevel) == -1)
                {
                    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                                 "  the required modification of the microphone "
                                 "volume failed");
                }
            }
        }
    }

    return true;
}

bool AudioDeviceMac::KeyPressed() {
  bool key_down = false;
  // Loop through all Mac virtual key constant values.
  for (unsigned int key_index = 0;
                    key_index < ARRAY_SIZE(prev_key_state_);
                    ++key_index) {
    bool keyState = CGEventSourceKeyState(
                             kCGEventSourceStateHIDSystemState,
                             key_index);
    // A false -> true change in keymap means a key is pressed.
    key_down |= (keyState && !prev_key_state_[key_index]);
    // Save current state.
    prev_key_state_[key_index] = keyState;
  }
  return key_down;
}
}  // namespace webrtc
