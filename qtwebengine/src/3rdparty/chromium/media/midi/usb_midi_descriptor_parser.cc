// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/midi/usb_midi_descriptor_parser.h"

#include <algorithm>

#include "base/logging.h"

namespace media {

namespace {

// The constants below are specified in USB spec, USB audio spec
// and USB midi spec.

enum DescriptorType {
  TYPE_DEVICE = 1,
  TYPE_CONFIGURATION = 2,
  TYPE_STRING = 3,
  TYPE_INTERFACE = 4,
  TYPE_ENDPOINT = 5,
  TYPE_DEVICE_QUALIFIER = 6,
  TYPE_OTHER_SPEED_CONFIGURATION = 7,
  TYPE_INTERFACE_POWER = 8,

  TYPE_CS_INTERFACE = 36,
  TYPE_CS_ENDPOINT = 37,
};

enum DescriptorSubType {
  SUBTYPE_MS_DESCRIPTOR_UNDEFINED = 0,
  SUBTYPE_MS_HEADER = 1,
  SUBTYPE_MIDI_IN_JACK = 2,
  SUBTYPE_MIDI_OUT_JACK = 3,
  SUBTYPE_ELEMENT = 4,
};

enum JackType {
  JACK_TYPE_UNDEFINED = 0,
  JACK_TYPE_EMBEDDED = 1,
  JACK_TYPE_EXTERNAL = 2,
};

const uint8 kAudioInterfaceClass = 1;
const uint8 kAudioMidiInterfaceSubclass = 3;

class JackMatcher {
 public:
  explicit JackMatcher(uint8 id) : id_(id) {}

  bool operator() (const UsbMidiJack& jack) const {
    return jack.jack_id == id_;
  }

 private:
  uint8 id_;
};

}  // namespace

UsbMidiDescriptorParser::UsbMidiDescriptorParser()
    : is_parsing_usb_midi_interface_(false),
      current_endpoint_address_(0),
      current_cable_number_(0) {}

UsbMidiDescriptorParser::~UsbMidiDescriptorParser() {}

bool UsbMidiDescriptorParser::Parse(UsbMidiDevice* device,
                                    const uint8* data,
                                    size_t size,
                                    std::vector<UsbMidiJack>* jacks) {
  jacks->clear();
  bool result = ParseInternal(device, data, size, jacks);
  if (!result)
    jacks->clear();
  Clear();
  return result;
}

bool UsbMidiDescriptorParser::ParseInternal(UsbMidiDevice* device,
                                            const uint8* data,
                                            size_t size,
                                            std::vector<UsbMidiJack>* jacks) {
  for (const uint8* current = data;
       current < data + size;
       current += current[0]) {
    uint8 length = current[0];
    if (length < 2) {
      DVLOG(1) << "Descriptor Type is not accessible.";
      return false;
    }
    if (current + length > data + size) {
      DVLOG(1) << "The header size is incorrect.";
      return false;
    }
    DescriptorType descriptor_type = static_cast<DescriptorType>(current[1]);
    if (descriptor_type != TYPE_INTERFACE && !is_parsing_usb_midi_interface_)
      continue;

    switch (descriptor_type) {
      case TYPE_INTERFACE:
        if (!ParseInterface(current, length))
          return false;
        break;
      case TYPE_CS_INTERFACE:
        // We are assuming that the corresponding INTERFACE precedes
        // the CS_INTERFACE descriptor, as specified.
        if (!ParseCSInterface(device, current, length))
          return false;
        break;
      case TYPE_ENDPOINT:
        // We are assuming that endpoints are contained in an interface.
        if (!ParseEndpoint(current, length))
          return false;
        break;
      case TYPE_CS_ENDPOINT:
        // We are assuming that the corresponding ENDPOINT precedes
        // the CS_ENDPOINT descriptor, as specified.
        if (!ParseCSEndpoint(current, length, jacks))
          return false;
        break;
      default:
        // Ignore uninteresting types.
        break;
    }
  }
  return true;
}

bool UsbMidiDescriptorParser::ParseInterface(const uint8* data, size_t size) {
  if (size != 9) {
    DVLOG(1) << "INTERFACE header size is incorrect.";
    return false;
  }
  incomplete_jacks_.clear();

  uint8 interface_class = data[5];
  uint8 interface_subclass = data[6];

  // All descriptors of endpoints contained in this interface
  // precede the next INTERFACE descriptor.
  is_parsing_usb_midi_interface_ =
      interface_class == kAudioInterfaceClass &&
      interface_subclass == kAudioMidiInterfaceSubclass;
  return true;
}

bool UsbMidiDescriptorParser::ParseCSInterface(UsbMidiDevice* device,
                                               const uint8* data,
                                               size_t size) {
  // Descriptor Type and Descriptor Subtype should be accessible.
  if (size < 3) {
    DVLOG(1) << "CS_INTERFACE header size is incorrect.";
    return false;
  }

  DescriptorSubType subtype = static_cast<DescriptorSubType>(data[2]);

  if (subtype != SUBTYPE_MIDI_OUT_JACK &&
      subtype != SUBTYPE_MIDI_IN_JACK)
    return true;

  if (size < 6) {
    DVLOG(1) << "CS_INTERFACE (MIDI JACK) header size is incorrect.";
    return false;
  }
  uint8 jack_type = data[3];
  uint8 id = data[4];
  if (jack_type == JACK_TYPE_EMBEDDED) {
    // We can't determine the associated endpoint now.
    incomplete_jacks_.push_back(UsbMidiJack(device, id, 0, 0));
  }
  return true;
}

bool UsbMidiDescriptorParser::ParseEndpoint(const uint8* data, size_t size) {
  if (size < 4) {
    DVLOG(1) << "ENDPOINT header size is incorrect.";
    return false;
  }
  current_endpoint_address_ = data[2];
  current_cable_number_ = 0;
  return true;
}

bool UsbMidiDescriptorParser::ParseCSEndpoint(const uint8* data,
                                              size_t size,
                                              std::vector<UsbMidiJack>* jacks) {
  const size_t kSizeForEmptyJacks = 4;
  // CS_ENDPOINT must be of size 4 + n where n is the number of associated
  // jacks.
  if (size < kSizeForEmptyJacks) {
    DVLOG(1) << "CS_ENDPOINT header size is incorrect.";
    return false;
  }
  uint8 num_jacks = data[3];
  if (size != kSizeForEmptyJacks + num_jacks) {
    DVLOG(1) << "CS_ENDPOINT header size is incorrect.";
    return false;
  }

  for (size_t i = 0; i < num_jacks; ++i) {
    uint8 jack = data[kSizeForEmptyJacks + i];
    std::vector<UsbMidiJack>::iterator it =
        std::find_if(incomplete_jacks_.begin(),
                     incomplete_jacks_.end(),
                     JackMatcher(jack));
    if (it == incomplete_jacks_.end()) {
      DVLOG(1) << "A non-existing MIDI jack is associated.";
      return false;
    }
    if (current_cable_number_ > 0xf) {
      DVLOG(1) << "Cable number should range from 0x0 to 0xf.";
      return false;
    }
    // CS_ENDPOINT follows ENDPOINT and hence we can use the following
    // member variables.
    it->cable_number = current_cable_number_++;
    it->endpoint_address = current_endpoint_address_;
    jacks->push_back(*it);
    incomplete_jacks_.erase(it);
  }
  return true;
}

void UsbMidiDescriptorParser::Clear() {
  is_parsing_usb_midi_interface_ = false;
  current_endpoint_address_ = 0;
  current_cable_number_ = 0;
  incomplete_jacks_.clear();
}

}  // namespace media
