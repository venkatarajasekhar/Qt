/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <iostream>
#include "webrtc/modules/audio_device/dummy/file_audio_device.h"
#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"

namespace webrtc {

int kRecordingFixedSampleRate = 48000;
int kRecordingNumChannels = 2;
int kPlayoutFixedSampleRate = 48000;
int kPlayoutNumChannels = 2;
int kPlayoutBufferSize = kPlayoutFixedSampleRate / 100
                         * kPlayoutNumChannels * 2;
int kRecordingBufferSize = kRecordingFixedSampleRate / 100
                           * kRecordingNumChannels * 2;

FileAudioDevice::FileAudioDevice(const int32_t id,
                                 const char* inputFilename,
                                 const char* outputFile):
    _ptrAudioBuffer(NULL),
    _recordingBuffer(NULL),
    _playoutBuffer(NULL),
    _recordingFramesLeft(0),
    _playoutFramesLeft(0),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _recordingBufferSizeIn10MS(0),
    _recordingFramesIn10MS(0),
    _playoutFramesIn10MS(0),
    _ptrThreadRec(NULL),
    _ptrThreadPlay(NULL),
    _recThreadID(0),
    _playThreadID(0),
    _playing(false),
    _recording(false),
    _lastCallPlayoutMillis(0),
    _lastCallRecordMillis(0),
    _outputFile(*FileWrapper::Create()),
    _inputFile(*FileWrapper::Create()),
    _outputFilename(outputFile),
    _inputFilename(inputFilename),
    _clock(Clock::GetRealTimeClock()) {
}

FileAudioDevice::~FileAudioDevice() {
  _outputFile.Flush();
  _outputFile.CloseFile();
  delete &_outputFile;
  _inputFile.Flush();
  _inputFile.CloseFile();
  delete &_inputFile;
}

int32_t FileAudioDevice::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const {
  return -1;
}

int32_t FileAudioDevice::Init() { return 0; }

int32_t FileAudioDevice::Terminate() { return 0; }

bool FileAudioDevice::Initialized() const { return true; }

int16_t FileAudioDevice::PlayoutDevices() {
  return 1;
}

int16_t FileAudioDevice::RecordingDevices() {
  return 1;
}

int32_t FileAudioDevice::PlayoutDeviceName(uint16_t index,
                                            char name[kAdmMaxDeviceNameSize],
                                            char guid[kAdmMaxGuidSize]) {
  const char* kName = "dummy_device";
  const char* kGuid = "dummy_device_unique_id";
  if (index < 1) {
    memset(name, 0, kAdmMaxDeviceNameSize);
    memset(guid, 0, kAdmMaxGuidSize);
    memcpy(name, kName, strlen(kName));
    memcpy(guid, kGuid, strlen(guid));
    return 0;
  }
  return -1;
}

int32_t FileAudioDevice::RecordingDeviceName(uint16_t index,
                                              char name[kAdmMaxDeviceNameSize],
                                              char guid[kAdmMaxGuidSize]) {
  const char* kName = "dummy_device";
  const char* kGuid = "dummy_device_unique_id";
  if (index < 1) {
    memset(name, 0, kAdmMaxDeviceNameSize);
    memset(guid, 0, kAdmMaxGuidSize);
    memcpy(name, kName, strlen(kName));
    memcpy(guid, kGuid, strlen(guid));
    return 0;
  }
  return -1;
}

int32_t FileAudioDevice::SetPlayoutDevice(uint16_t index) {
  if (index == 0) {
    _playout_index = index;
    return 0;
  }
  return -1;
}

int32_t FileAudioDevice::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType device) {
  return -1;
}

int32_t FileAudioDevice::SetRecordingDevice(uint16_t index) {
  if (index == 0) {
    _record_index = index;
    return _record_index;
  }
  return -1;
}

int32_t FileAudioDevice::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType device) {
  return -1;
}

int32_t FileAudioDevice::PlayoutIsAvailable(bool& available) {
  if (_playout_index == 0) {
    available = true;
    return _playout_index;
  }
  available = false;
  return -1;
}

int32_t FileAudioDevice::InitPlayout() {
  if (_ptrAudioBuffer)
  {
      // Update webrtc audio buffer with the selected parameters
      _ptrAudioBuffer->SetPlayoutSampleRate(kPlayoutFixedSampleRate);
      _ptrAudioBuffer->SetPlayoutChannels(kPlayoutNumChannels);
  }
  return 0;
}

bool FileAudioDevice::PlayoutIsInitialized() const {
  return true;
}

int32_t FileAudioDevice::RecordingIsAvailable(bool& available) {
  if (_record_index == 0) {
    available = true;
    return _record_index;
  }
  available = false;
  return -1;
}

int32_t FileAudioDevice::InitRecording() {
  CriticalSectionScoped lock(&_critSect);

  if (_recording) {
    return -1;
  }

  _recordingFramesIn10MS = kRecordingFixedSampleRate/100;

  if (_ptrAudioBuffer) {
    _ptrAudioBuffer->SetRecordingSampleRate(kRecordingFixedSampleRate);
    _ptrAudioBuffer->SetRecordingChannels(kRecordingNumChannels);
  }
  return 0;
}

bool FileAudioDevice::RecordingIsInitialized() const {
  return true;
}

int32_t FileAudioDevice::StartPlayout() {
  if (_playing)
  {
      return 0;
  }

  _playing = true;
  _playoutFramesLeft = 0;

  if (!_playoutBuffer)
      _playoutBuffer = new int8_t[2 *
                                  kPlayoutNumChannels *
                                  kPlayoutFixedSampleRate/100];
  if (!_playoutBuffer)
  {
    _playing = false;
    return -1;
  }

  // PLAYOUT
  const char* threadName = "webrtc_audio_module_play_thread";
  _ptrThreadPlay =  ThreadWrapper::CreateThread(PlayThreadFunc,
                                                this,
                                                kRealtimePriority,
                                                threadName);
  if (_ptrThreadPlay == NULL)
  {
      _playing = false;
      delete [] _playoutBuffer;
      _playoutBuffer = NULL;
      return -1;
  }

  if (_outputFile.OpenFile(_outputFilename.c_str(),
                           false, false, false) == -1) {
    printf("Failed to open playout file %s!", _outputFilename.c_str());
    _playing = false;
    delete [] _playoutBuffer;
    _playoutBuffer = NULL;
    return -1;
  }

  unsigned int threadID(0);
  if (!_ptrThreadPlay->Start(threadID))
  {
      _playing = false;
      delete _ptrThreadPlay;
      _ptrThreadPlay = NULL;
      delete [] _playoutBuffer;
      _playoutBuffer = NULL;
      return -1;
  }
  _playThreadID = threadID;

  return 0;
}

int32_t FileAudioDevice::StopPlayout() {
  {
      CriticalSectionScoped lock(&_critSect);
      _playing = false;
  }

  // stop playout thread first
  if (_ptrThreadPlay && !_ptrThreadPlay->Stop())
  {
      return -1;
  }
  else {
      delete _ptrThreadPlay;
      _ptrThreadPlay = NULL;
  }

  CriticalSectionScoped lock(&_critSect);

  _playoutFramesLeft = 0;
  delete [] _playoutBuffer;
  _playoutBuffer = NULL;
  _outputFile.Flush();
  _outputFile.CloseFile();
   return 0;
}

bool FileAudioDevice::Playing() const {
  return true;
}

int32_t FileAudioDevice::StartRecording() {
  _recording = true;

  // Make sure we only create the buffer once.
  _recordingBufferSizeIn10MS = _recordingFramesIn10MS *
                               kRecordingNumChannels *
                               2;
  if (!_recordingBuffer) {
      _recordingBuffer = new int8_t[_recordingBufferSizeIn10MS];
  }

  if (_inputFile.OpenFile(_inputFilename.c_str(), true,
                              true, false) == -1) {
    printf("Failed to open audio input file %s!\n",
           _inputFilename.c_str());
    _recording = false;
    delete[] _recordingBuffer;
    _recordingBuffer = NULL;
    return -1;
  }

  const char* threadName = "webrtc_audio_module_capture_thread";
  _ptrThreadRec = ThreadWrapper::CreateThread(RecThreadFunc,
                                              this,
                                              kRealtimePriority,
                                              threadName);
  if (_ptrThreadRec == NULL)
  {
      _recording = false;
      delete [] _recordingBuffer;
      _recordingBuffer = NULL;
      return -1;
  }

  unsigned int threadID(0);
  if (!_ptrThreadRec->Start(threadID))
  {
      _recording = false;
      delete _ptrThreadRec;
      _ptrThreadRec = NULL;
      delete [] _recordingBuffer;
      _recordingBuffer = NULL;
      return -1;
  }
  _recThreadID = threadID;

  return 0;
}


int32_t FileAudioDevice::StopRecording() {
  {
    CriticalSectionScoped lock(&_critSect);
    _recording = false;
  }

  if (_ptrThreadRec && !_ptrThreadRec->Stop())
  {
      return -1;
  }
  else {
      delete _ptrThreadRec;
      _ptrThreadRec = NULL;
  }

  CriticalSectionScoped lock(&_critSect);
  _recordingFramesLeft = 0;
  if (_recordingBuffer)
  {
      delete [] _recordingBuffer;
      _recordingBuffer = NULL;
  }
  return 0;
}

bool FileAudioDevice::Recording() const {
  return _recording;
}

int32_t FileAudioDevice::SetAGC(bool enable) { return -1; }

bool FileAudioDevice::AGC() const { return false; }

int32_t FileAudioDevice::SetWaveOutVolume(uint16_t volumeLeft,
                                           uint16_t volumeRight) {
  return -1;
}

int32_t FileAudioDevice::WaveOutVolume(uint16_t& volumeLeft,
                                        uint16_t& volumeRight) const {
  return -1;
}

int32_t FileAudioDevice::InitSpeaker() { return -1; }

bool FileAudioDevice::SpeakerIsInitialized() const { return false; }

int32_t FileAudioDevice::InitMicrophone() { return 0; }

bool FileAudioDevice::MicrophoneIsInitialized() const { return true; }

int32_t FileAudioDevice::SpeakerVolumeIsAvailable(bool& available) {
  return -1;
}

int32_t FileAudioDevice::SetSpeakerVolume(uint32_t volume) { return -1; }

int32_t FileAudioDevice::SpeakerVolume(uint32_t& volume) const { return -1; }

int32_t FileAudioDevice::MaxSpeakerVolume(uint32_t& maxVolume) const {
  return -1;
}

int32_t FileAudioDevice::MinSpeakerVolume(uint32_t& minVolume) const {
  return -1;
}

int32_t FileAudioDevice::SpeakerVolumeStepSize(uint16_t& stepSize) const {
  return -1;
}

int32_t FileAudioDevice::MicrophoneVolumeIsAvailable(bool& available) {
  return -1;
}

int32_t FileAudioDevice::SetMicrophoneVolume(uint32_t volume) { return -1; }

int32_t FileAudioDevice::MicrophoneVolume(uint32_t& volume) const {
  return -1;
}

int32_t FileAudioDevice::MaxMicrophoneVolume(uint32_t& maxVolume) const {
  return -1;
}

int32_t FileAudioDevice::MinMicrophoneVolume(uint32_t& minVolume) const {
  return -1;
}

int32_t FileAudioDevice::MicrophoneVolumeStepSize(uint16_t& stepSize) const {
  return -1;
}

int32_t FileAudioDevice::SpeakerMuteIsAvailable(bool& available) { return -1; }

int32_t FileAudioDevice::SetSpeakerMute(bool enable) { return -1; }

int32_t FileAudioDevice::SpeakerMute(bool& enabled) const { return -1; }

int32_t FileAudioDevice::MicrophoneMuteIsAvailable(bool& available) {
  return -1;
}

int32_t FileAudioDevice::SetMicrophoneMute(bool enable) { return -1; }

int32_t FileAudioDevice::MicrophoneMute(bool& enabled) const { return -1; }

int32_t FileAudioDevice::MicrophoneBoostIsAvailable(bool& available) {
  return -1;
}

int32_t FileAudioDevice::SetMicrophoneBoost(bool enable) { return -1; }

int32_t FileAudioDevice::MicrophoneBoost(bool& enabled) const { return -1; }

int32_t FileAudioDevice::StereoPlayoutIsAvailable(bool& available) {
  available = true;
  return 0;
}
int32_t FileAudioDevice::SetStereoPlayout(bool enable) {
  return 0;
}

int32_t FileAudioDevice::StereoPlayout(bool& enabled) const {
  enabled = true;
  return 0;
}

int32_t FileAudioDevice::StereoRecordingIsAvailable(bool& available) {
  available = true;
  return 0;
}

int32_t FileAudioDevice::SetStereoRecording(bool enable) {
  return 0;
}

int32_t FileAudioDevice::StereoRecording(bool& enabled) const {
  enabled = true;
  return 0;
}

int32_t FileAudioDevice::SetPlayoutBuffer(
    const AudioDeviceModule::BufferType type,
    uint16_t sizeMS) {
  return 0;
}

int32_t FileAudioDevice::PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                        uint16_t& sizeMS) const {
  type = _playBufType;
  return 0;
}

int32_t FileAudioDevice::PlayoutDelay(uint16_t& delayMS) const {
  return 0;
}

int32_t FileAudioDevice::RecordingDelay(uint16_t& delayMS) const { return -1; }

int32_t FileAudioDevice::CPULoad(uint16_t& load) const { return -1; }

bool FileAudioDevice::PlayoutWarning() const { return false; }

bool FileAudioDevice::PlayoutError() const { return false; }

bool FileAudioDevice::RecordingWarning() const { return false; }

bool FileAudioDevice::RecordingError() const { return false; }

void FileAudioDevice::ClearPlayoutWarning() {}

void FileAudioDevice::ClearPlayoutError() {}

void FileAudioDevice::ClearRecordingWarning() {}

void FileAudioDevice::ClearRecordingError() {}

void FileAudioDevice::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) {
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

bool FileAudioDevice::PlayThreadFunc(void* pThis)
{
    return (static_cast<FileAudioDevice*>(pThis)->PlayThreadProcess());
}

bool FileAudioDevice::RecThreadFunc(void* pThis)
{
    return (static_cast<FileAudioDevice*>(pThis)->RecThreadProcess());
}

bool FileAudioDevice::PlayThreadProcess()
{
    if(!_playing)
        return false;

    uint64_t currentTime = _clock->CurrentNtpInMilliseconds();
    _critSect.Enter();

    if (_lastCallPlayoutMillis == 0 ||
        currentTime - _lastCallPlayoutMillis >= 10)
    {
        _critSect.Leave();
        _ptrAudioBuffer->RequestPlayoutData(_playoutFramesIn10MS);
        _critSect.Enter();

        _playoutFramesLeft = _ptrAudioBuffer->GetPlayoutData(_playoutBuffer);
        assert(_playoutFramesLeft == _playoutFramesIn10MS);
        if (_outputFile.Open()) {
          _outputFile.Write(_playoutBuffer, kPlayoutBufferSize);
          _outputFile.Flush();
        }
        _lastCallPlayoutMillis = currentTime;
    }
    _playoutFramesLeft = 0;
    _critSect.Leave();
    SleepMs(10 - (_clock->CurrentNtpInMilliseconds() - currentTime));
    return true;
}

bool FileAudioDevice::RecThreadProcess()
{
    if (!_recording)
        return false;

    uint64_t currentTime = _clock->CurrentNtpInMilliseconds();
    _critSect.Enter();

    if (_lastCallRecordMillis == 0 ||
        currentTime - _lastCallRecordMillis >= 10) {
      if (_inputFile.Open()) {
        if (_inputFile.Read(_recordingBuffer, kRecordingBufferSize) > 0) {
          _ptrAudioBuffer->SetRecordedBuffer(_recordingBuffer,
                                             _recordingFramesIn10MS);
        } else {
          _inputFile.Rewind();
        }
        _lastCallRecordMillis = currentTime;
        _critSect.Leave();
        _ptrAudioBuffer->DeliverRecordedData();
        _critSect.Enter();
      }
    }

    _critSect.Leave();
    SleepMs(10 - (_clock->CurrentNtpInMilliseconds() - currentTime));
    return true;
}

}  // namespace webrtc
