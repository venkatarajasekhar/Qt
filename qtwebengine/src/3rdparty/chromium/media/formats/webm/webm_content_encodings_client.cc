// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/formats/webm/webm_content_encodings_client.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "media/formats/webm/webm_constants.h"

namespace media {

WebMContentEncodingsClient::WebMContentEncodingsClient(const LogCB& log_cb)
    : log_cb_(log_cb),
      content_encryption_encountered_(false),
      content_encodings_ready_(false) {
}

WebMContentEncodingsClient::~WebMContentEncodingsClient() {
  STLDeleteElements(&content_encodings_);
}

const ContentEncodings& WebMContentEncodingsClient::content_encodings() const {
  DCHECK(content_encodings_ready_);
  return content_encodings_;
}

WebMParserClient* WebMContentEncodingsClient::OnListStart(int id) {
  if (id == kWebMIdContentEncodings) {
    DCHECK(!cur_content_encoding_.get());
    DCHECK(!content_encryption_encountered_);
    STLDeleteElements(&content_encodings_);
    content_encodings_ready_ = false;
    return this;
  }

  if (id == kWebMIdContentEncoding) {
    DCHECK(!cur_content_encoding_.get());
    DCHECK(!content_encryption_encountered_);
    cur_content_encoding_.reset(new ContentEncoding());
    return this;
  }

  if (id == kWebMIdContentEncryption) {
    DCHECK(cur_content_encoding_.get());
    if (content_encryption_encountered_) {
      MEDIA_LOG(log_cb_) << "Unexpected multiple ContentEncryption.";
      return NULL;
    }
    content_encryption_encountered_ = true;
    return this;
  }

  if (id == kWebMIdContentEncAESSettings) {
    DCHECK(cur_content_encoding_.get());
    return this;
  }

  // This should not happen if WebMListParser is working properly.
  DCHECK(false);
  return NULL;
}

// Mandatory occurrence restriction is checked in this function. Multiple
// occurrence restriction is checked in OnUInt and OnBinary.
bool WebMContentEncodingsClient::OnListEnd(int id) {
  if (id == kWebMIdContentEncodings) {
    // ContentEncoding element is mandatory. Check this!
    if (content_encodings_.empty()) {
      MEDIA_LOG(log_cb_) << "Missing ContentEncoding.";
      return false;
    }
    content_encodings_ready_ = true;
    return true;
  }

  if (id == kWebMIdContentEncoding) {
    DCHECK(cur_content_encoding_.get());

    //
    // Specify default values to missing mandatory elements.
    //

    if (cur_content_encoding_->order() == ContentEncoding::kOrderInvalid) {
      // Default value of encoding order is 0, which should only be used on the
      // first ContentEncoding.
      if (!content_encodings_.empty()) {
        MEDIA_LOG(log_cb_) << "Missing ContentEncodingOrder.";
        return false;
      }
      cur_content_encoding_->set_order(0);
    }

    if (cur_content_encoding_->scope() == ContentEncoding::kScopeInvalid)
      cur_content_encoding_->set_scope(ContentEncoding::kScopeAllFrameContents);

    if (cur_content_encoding_->type() == ContentEncoding::kTypeInvalid)
      cur_content_encoding_->set_type(ContentEncoding::kTypeCompression);

    // Check for elements valid in spec but not supported for now.
    if (cur_content_encoding_->type() == ContentEncoding::kTypeCompression) {
      MEDIA_LOG(log_cb_) << "ContentCompression not supported.";
      return false;
    }

    // Enforce mandatory elements without default values.
    DCHECK(cur_content_encoding_->type() == ContentEncoding::kTypeEncryption);
    if (!content_encryption_encountered_) {
      MEDIA_LOG(log_cb_) << "ContentEncodingType is encryption but"
                         << " ContentEncryption is missing.";
      return false;
    }

    content_encodings_.push_back(cur_content_encoding_.release());
    content_encryption_encountered_ = false;
    return true;
  }

  if (id == kWebMIdContentEncryption) {
    DCHECK(cur_content_encoding_.get());
    // Specify default value for elements that are not present.
    if (cur_content_encoding_->encryption_algo() ==
        ContentEncoding::kEncAlgoInvalid) {
      cur_content_encoding_->set_encryption_algo(
          ContentEncoding::kEncAlgoNotEncrypted);
    }
    return true;
  }

  if (id == kWebMIdContentEncAESSettings) {
    if (cur_content_encoding_->cipher_mode() ==
        ContentEncoding::kCipherModeInvalid)
      cur_content_encoding_->set_cipher_mode(ContentEncoding::kCipherModeCtr);
    return true;
  }

  // This should not happen if WebMListParser is working properly.
  DCHECK(false);
  return false;
}

// Multiple occurrence restriction and range are checked in this function.
// Mandatory occurrence restriction is checked in OnListEnd.
bool WebMContentEncodingsClient::OnUInt(int id, int64 val) {
  DCHECK(cur_content_encoding_.get());

  if (id == kWebMIdContentEncodingOrder) {
    if (cur_content_encoding_->order() != ContentEncoding::kOrderInvalid) {
      MEDIA_LOG(log_cb_) << "Unexpected multiple ContentEncodingOrder.";
      return false;
    }

    if (val != static_cast<int64>(content_encodings_.size())) {
      // According to the spec, encoding order starts with 0 and counts upwards.
      MEDIA_LOG(log_cb_) << "Unexpected ContentEncodingOrder.";
      return false;
    }

    cur_content_encoding_->set_order(val);
    return true;
  }

  if (id == kWebMIdContentEncodingScope) {
    if (cur_content_encoding_->scope() != ContentEncoding::kScopeInvalid) {
      MEDIA_LOG(log_cb_) << "Unexpected multiple ContentEncodingScope.";
      return false;
    }

    if (val == ContentEncoding::kScopeInvalid ||
        val > ContentEncoding::kScopeMax) {
      MEDIA_LOG(log_cb_) << "Unexpected ContentEncodingScope.";
      return false;
    }

    if (val & ContentEncoding::kScopeNextContentEncodingData) {
      MEDIA_LOG(log_cb_) << "Encoded next ContentEncoding is not supported.";
      return false;
    }

    cur_content_encoding_->set_scope(static_cast<ContentEncoding::Scope>(val));
    return true;
  }

  if (id == kWebMIdContentEncodingType) {
    if (cur_content_encoding_->type() != ContentEncoding::kTypeInvalid) {
      MEDIA_LOG(log_cb_) << "Unexpected multiple ContentEncodingType.";
      return false;
    }

    if (val == ContentEncoding::kTypeCompression) {
      MEDIA_LOG(log_cb_) << "ContentCompression not supported.";
      return false;
    }

    if (val != ContentEncoding::kTypeEncryption) {
      MEDIA_LOG(log_cb_) << "Unexpected ContentEncodingType " << val << ".";
      return false;
    }

    cur_content_encoding_->set_type(static_cast<ContentEncoding::Type>(val));
    return true;
  }

  if (id == kWebMIdContentEncAlgo) {
    if (cur_content_encoding_->encryption_algo() !=
        ContentEncoding::kEncAlgoInvalid) {
      MEDIA_LOG(log_cb_) << "Unexpected multiple ContentEncAlgo.";
      return false;
    }

    if (val < ContentEncoding::kEncAlgoNotEncrypted ||
        val > ContentEncoding::kEncAlgoAes) {
      MEDIA_LOG(log_cb_) << "Unexpected ContentEncAlgo " << val << ".";
      return false;
    }

    cur_content_encoding_->set_encryption_algo(
        static_cast<ContentEncoding::EncryptionAlgo>(val));
    return true;
  }

  if (id == kWebMIdAESSettingsCipherMode) {
    if (cur_content_encoding_->cipher_mode() !=
        ContentEncoding::kCipherModeInvalid) {
      MEDIA_LOG(log_cb_) << "Unexpected multiple AESSettingsCipherMode.";
      return false;
    }

    if (val != ContentEncoding::kCipherModeCtr) {
      MEDIA_LOG(log_cb_) << "Unexpected AESSettingsCipherMode " << val << ".";
      return false;
    }

    cur_content_encoding_->set_cipher_mode(
        static_cast<ContentEncoding::CipherMode>(val));
    return true;
  }

  // This should not happen if WebMListParser is working properly.
  DCHECK(false);
  return false;
}

// Multiple occurrence restriction is checked in this function.  Mandatory
// restriction is checked in OnListEnd.
bool WebMContentEncodingsClient::OnBinary(int id, const uint8* data, int size) {
  DCHECK(cur_content_encoding_.get());
  DCHECK(data);
  DCHECK_GT(size, 0);

  if (id == kWebMIdContentEncKeyID) {
    if (!cur_content_encoding_->encryption_key_id().empty()) {
      MEDIA_LOG(log_cb_) << "Unexpected multiple ContentEncKeyID";
      return false;
    }
    cur_content_encoding_->SetEncryptionKeyId(data, size);
    return true;
  }

  // This should not happen if WebMListParser is working properly.
  DCHECK(false);
  return false;
}

}  // namespace media
