//===-- AMDILNIDevice.cpp - Device Info for Northern Islands devices ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
#include "AMDILNIDevice.h"
#include "AMDILEvergreenDevice.h"
#include "AMDGPUSubtarget.h"

using namespace llvm;

AMDGPUNIDevice::AMDGPUNIDevice(AMDGPUSubtarget *ST)
  : AMDGPUEvergreenDevice(ST)
{
  std::string name = ST->getDeviceName();
  if (name == "caicos") {
    mDeviceFlag = OCL_DEVICE_CAICOS;
  } else if (name == "turks") {
    mDeviceFlag = OCL_DEVICE_TURKS;
  } else if (name == "cayman") {
    mDeviceFlag = OCL_DEVICE_CAYMAN;
  } else {
    mDeviceFlag = OCL_DEVICE_BARTS;
  }
}
AMDGPUNIDevice::~AMDGPUNIDevice()
{
}

size_t
AMDGPUNIDevice::getMaxLDSSize() const
{
  if (usesHardware(AMDGPUDeviceInfo::LocalMem)) {
    return MAX_LDS_SIZE_900;
  } else {
    return 0;
  }
}

uint32_t
AMDGPUNIDevice::getGeneration() const
{
  return AMDGPUDeviceInfo::HD6XXX;
}


AMDGPUCaymanDevice::AMDGPUCaymanDevice(AMDGPUSubtarget *ST)
  : AMDGPUNIDevice(ST)
{
  setCaps();
}

AMDGPUCaymanDevice::~AMDGPUCaymanDevice()
{
}

void
AMDGPUCaymanDevice::setCaps()
{
  if (mSTM->isOverride(AMDGPUDeviceInfo::DoubleOps)) {
    mHWBits.set(AMDGPUDeviceInfo::DoubleOps);
    mHWBits.set(AMDGPUDeviceInfo::FMA);
  }
  mHWBits.set(AMDGPUDeviceInfo::Signed24BitOps);
  mSWBits.reset(AMDGPUDeviceInfo::Signed24BitOps);
  mSWBits.set(AMDGPUDeviceInfo::ArenaSegment);
}

