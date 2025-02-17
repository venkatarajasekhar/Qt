// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/filter/filter.h"

#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "net/base/filename_util_unsafe.h"
#include "net/base/io_buffer.h"
#include "net/base/mime_util.h"
#include "net/filter/gzip_filter.h"
#include "net/filter/sdch_filter.h"
#include "net/url_request/url_request_context.h"
#include "url/gurl.h"

namespace {

// Filter types (using canonical lower case only):
const char kDeflate[]      = "deflate";
const char kGZip[]         = "gzip";
const char kXGZip[]        = "x-gzip";
const char kSdch[]         = "sdch";
// compress and x-compress are currently not supported.  If we decide to support
// them, we'll need the same mime type compatibility hack we have for gzip.  For
// more information, see Firefox's nsHttpChannel::ProcessNormal.

// Mime types:
const char kApplicationXGzip[]     = "application/x-gzip";
const char kApplicationGzip[]      = "application/gzip";
const char kApplicationXGunzip[]   = "application/x-gunzip";
const char kTextHtml[]             = "text/html";

// Buffer size allocated when de-compressing data.
const int kFilterBufSize = 32 * 1024;

}  // namespace

namespace net {

FilterContext::~FilterContext() {
}

Filter::~Filter() {}

// static
Filter* Filter::Factory(const std::vector<FilterType>& filter_types,
                        const FilterContext& filter_context) {
  if (filter_types.empty())
    return NULL;

  Filter* filter_list = NULL;  // Linked list of filters.
  for (size_t i = 0; i < filter_types.size(); i++) {
    filter_list = PrependNewFilter(filter_types[i], filter_context,
                                   kFilterBufSize, filter_list);
    if (!filter_list)
      return NULL;
  }
  return filter_list;
}

// static
Filter* Filter::GZipFactory() {
  return InitGZipFilter(FILTER_TYPE_GZIP, kFilterBufSize);
}

// static
Filter* Filter::FactoryForTests(const std::vector<FilterType>& filter_types,
                                const FilterContext& filter_context,
                                int buffer_size) {
  if (filter_types.empty())
    return NULL;

  Filter* filter_list = NULL;  // Linked list of filters.
  for (size_t i = 0; i < filter_types.size(); i++) {
    filter_list = PrependNewFilter(filter_types[i], filter_context,
                                   buffer_size, filter_list);
    if (!filter_list)
      return NULL;
  }
  return filter_list;
}

Filter::FilterStatus Filter::ReadData(char* dest_buffer, int* dest_len) {
  const int dest_buffer_capacity = *dest_len;
  if (last_status_ == FILTER_ERROR)
    return last_status_;
  if (!next_filter_.get())
    return last_status_ = ReadFilteredData(dest_buffer, dest_len);
  if (last_status_ == FILTER_NEED_MORE_DATA && !stream_data_len())
    return next_filter_->ReadData(dest_buffer, dest_len);

  do {
    if (next_filter_->last_status() == FILTER_NEED_MORE_DATA) {
      PushDataIntoNextFilter();
      if (FILTER_ERROR == last_status_)
        return FILTER_ERROR;
    }
    *dest_len = dest_buffer_capacity;  // Reset the input/output parameter.
    next_filter_->ReadData(dest_buffer, dest_len);
    if (FILTER_NEED_MORE_DATA == last_status_)
        return next_filter_->last_status();

    // In the case where this filter has data internally, and is indicating such
    // with a last_status_ of FILTER_OK, but at the same time the next filter in
    // the chain indicated it FILTER_NEED_MORE_DATA, we have to be cautious
    // about confusing the caller.  The API confusion can appear if we return
    // FILTER_OK (suggesting we have more data in aggregate), but yet we don't
    // populate our output buffer.  When that is the case, we need to
    // alternately call our filter element, and the next_filter element until we
    // get out of this state (by pumping data into the next filter until it
    // outputs data, or it runs out of data and reports that it NEED_MORE_DATA.)
  } while (FILTER_OK == last_status_ &&
           FILTER_NEED_MORE_DATA == next_filter_->last_status() &&
           0 == *dest_len);

  if (next_filter_->last_status() == FILTER_ERROR)
    return FILTER_ERROR;
  return FILTER_OK;
}

bool Filter::FlushStreamBuffer(int stream_data_len) {
  DCHECK_LE(stream_data_len, stream_buffer_size_);
  if (stream_data_len <= 0 || stream_data_len > stream_buffer_size_)
    return false;

  DCHECK(stream_buffer());
  // Bail out if there is more data in the stream buffer to be filtered.
  if (!stream_buffer() || stream_data_len_)
    return false;

  next_stream_data_ = stream_buffer()->data();
  stream_data_len_ = stream_data_len;
  return true;
}

// static
Filter::FilterType Filter::ConvertEncodingToType(
    const std::string& filter_type) {
  FilterType type_id;
  if (LowerCaseEqualsASCII(filter_type, kDeflate)) {
    type_id = FILTER_TYPE_DEFLATE;
  } else if (LowerCaseEqualsASCII(filter_type, kGZip) ||
             LowerCaseEqualsASCII(filter_type, kXGZip)) {
    type_id = FILTER_TYPE_GZIP;
  } else if (LowerCaseEqualsASCII(filter_type, kSdch)) {
    type_id = FILTER_TYPE_SDCH;
  } else {
    // Note we also consider "identity" and "uncompressed" UNSUPPORTED as
    // filter should be disabled in such cases.
    type_id = FILTER_TYPE_UNSUPPORTED;
  }
  return type_id;
}

// static
void Filter::FixupEncodingTypes(
    const FilterContext& filter_context,
    std::vector<FilterType>* encoding_types) {
  std::string mime_type;
  bool success = filter_context.GetMimeType(&mime_type);
  DCHECK(success || mime_type.empty());

  if ((1 == encoding_types->size()) &&
      (FILTER_TYPE_GZIP == encoding_types->front())) {
    if (LowerCaseEqualsASCII(mime_type, kApplicationXGzip) ||
        LowerCaseEqualsASCII(mime_type, kApplicationGzip) ||
        LowerCaseEqualsASCII(mime_type, kApplicationXGunzip))
      // The server has told us that it sent us gziped content with a gzip
      // content encoding.  Sadly, Apache mistakenly sets these headers for all
      // .gz files.  We match Firefox's nsHttpChannel::ProcessNormal and ignore
      // the Content-Encoding here.
      encoding_types->clear();

    GURL url;
    std::string disposition;
    success = filter_context.GetURL(&url);
    DCHECK(success);
    filter_context.GetContentDisposition(&disposition);
    // Don't supply a MIME type here, since that may cause disk IO.
    base::FilePath::StringType extension =
        GenerateFileExtensionUnsafe(url, disposition, "UTF-8", "", "", "");

    if (filter_context.IsDownload()) {
      // We don't want to decompress gzipped files when the user explicitly
      // asks to download them.
      // For the case of svgz files, we use the extension to distinguish
      // between svgz files and svg files compressed with gzip by the server.
      // When viewing a .svgz file, we need to uncompress it, but we don't
      // want to do that when downloading.
      // See Firefox's nonDecodableExtensions in nsExternalHelperAppService.cpp
      if (EndsWith(extension, FILE_PATH_LITERAL(".gz"), false) ||
          LowerCaseEqualsASCII(extension, ".tgz") ||
          LowerCaseEqualsASCII(extension, ".svgz"))
        encoding_types->clear();
    } else {
      // When the user does not explicitly ask to download a file, if we get a
      // supported mime type, then we attempt to decompress in order to view it.
      // However, if it's not a supported mime type, then we will attempt to
      // download it, and in that case, don't decompress .gz/.tgz files.
      if ((EndsWith(extension, FILE_PATH_LITERAL(".gz"), false) ||
           LowerCaseEqualsASCII(extension, ".tgz")) &&
          !IsSupportedMimeType(mime_type))
        encoding_types->clear();
    }
  }

  // If the request was for SDCH content, then we might need additional fixups.
  if (!filter_context.IsSdchResponse()) {
    // It was not an SDCH request, so we'll just record stats.
    if (1 < encoding_types->size()) {
      // Multiple filters were intended to only be used for SDCH (thus far!)
      SdchManager::SdchErrorRecovery(
          SdchManager::MULTIENCODING_FOR_NON_SDCH_REQUEST);
    }
    if ((1 == encoding_types->size()) &&
        (FILTER_TYPE_SDCH == encoding_types->front())) {
        SdchManager::SdchErrorRecovery(
            SdchManager::SDCH_CONTENT_ENCODE_FOR_NON_SDCH_REQUEST);
    }
    return;
  }

  // The request was tagged as an SDCH request, which means the server supplied
  // a dictionary, and we advertised it in the request.  Some proxies will do
  // very strange things to the request, or the response, so we have to handle
  // them gracefully.

  // If content encoding included SDCH, then everything is "relatively" fine.
  if (!encoding_types->empty() &&
      (FILTER_TYPE_SDCH == encoding_types->front())) {
    // Some proxies (found currently in Argentina) strip the Content-Encoding
    // text from "sdch,gzip" to a mere "sdch" without modifying the compressed
    // payload.   To handle this gracefully, we simulate the "probably" deleted
    // ",gzip" by appending a tentative gzip decode, which will default to a
    // no-op pass through filter if it doesn't get gzip headers where expected.
    if (1 == encoding_types->size()) {
      encoding_types->push_back(FILTER_TYPE_GZIP_HELPING_SDCH);
      SdchManager::SdchErrorRecovery(
          SdchManager::OPTIONAL_GUNZIP_ENCODING_ADDED);
    }
    return;
  }

  // There are now several cases to handle for an SDCH request.  Foremost, if
  // the outbound request was stripped so as not to advertise support for
  // encodings, we might get back content with no encoding, or (for example)
  // just gzip.  We have to be sure that any changes we make allow for such
  // minimal coding to work.  That issue is why we use TENTATIVE filters if we
  // add any, as those filters sniff the content, and act as pass-through
  // filters if headers are not found.

  // If the outbound GET is not modified, then the server will generally try to
  // send us SDCH encoded content.  As that content returns, there are several
  // corruptions of the header "content-encoding" that proxies may perform (and
  // have been detected in the wild).  We already dealt with the a honest
  // content encoding of "sdch,gzip" being corrupted into "sdch" with on change
  // of the actual content.  Another common corruption is to either disscard
  // the accurate content encoding, or to replace it with gzip only (again, with
  // no change in actual content). The last observed corruption it to actually
  // change the content, such as by re-gzipping it, and that may happen along
  // with corruption of the stated content encoding (wow!).

  // The one unresolved failure mode comes when we advertise a dictionary, and
  // the server tries to *send* a gzipped file (not gzip encode content), and
  // then we could do a gzip decode :-(. Since SDCH is only (currently)
  // supported server side on paths that only send HTML content, this mode has
  // never surfaced in the wild (and is unlikely to).
  // We will gather a lot of stats as we perform the fixups
  if (StartsWithASCII(mime_type, kTextHtml, false)) {
    // Suspicious case: Advertised dictionary, but server didn't use sdch, and
    // we're HTML tagged.
    if (encoding_types->empty()) {
      SdchManager::SdchErrorRecovery(
          SdchManager::ADDED_CONTENT_ENCODING);
    } else if (1 == encoding_types->size()) {
      SdchManager::SdchErrorRecovery(
          SdchManager::FIXED_CONTENT_ENCODING);
    } else {
      SdchManager::SdchErrorRecovery(
          SdchManager::FIXED_CONTENT_ENCODINGS);
    }
  } else {
    // Remarkable case!?!  We advertised an SDCH dictionary, content-encoding
    // was not marked for SDCH processing: Why did the server suggest an SDCH
    // dictionary in the first place??.  Also, the content isn't
    // tagged as HTML, despite the fact that SDCH encoding is mostly likely for
    // HTML: Did some anti-virus system strip this tag (sometimes they strip
    // accept-encoding headers on the request)??  Does the content encoding not
    // start with "text/html" for some other reason??  We'll report this as a
    // fixup to a binary file, but it probably really is text/html (some how).
    if (encoding_types->empty()) {
      SdchManager::SdchErrorRecovery(
          SdchManager::BINARY_ADDED_CONTENT_ENCODING);
    } else if (1 == encoding_types->size()) {
      SdchManager::SdchErrorRecovery(
          SdchManager::BINARY_FIXED_CONTENT_ENCODING);
    } else {
      SdchManager::SdchErrorRecovery(
          SdchManager::BINARY_FIXED_CONTENT_ENCODINGS);
    }
  }

  // Leave the existing encoding type to be processed first, and add our
  // tentative decodings to be done afterwards.  Vodaphone UK reportedyl will
  // perform a second layer of gzip encoding atop the server's sdch,gzip
  // encoding, and then claim that the content encoding is a mere gzip.  As a
  // result we'll need (in that case) to do the gunzip, plus our tentative
  // gunzip and tentative SDCH decoding.
  // This approach nicely handles the empty() list as well, and should work with
  // other (as yet undiscovered) proxies the choose to re-compressed with some
  // other encoding (such as bzip2, etc.).
  encoding_types->insert(encoding_types->begin(),
                         FILTER_TYPE_GZIP_HELPING_SDCH);
  encoding_types->insert(encoding_types->begin(), FILTER_TYPE_SDCH_POSSIBLE);
  return;
}

Filter::Filter()
    : stream_buffer_(NULL),
      stream_buffer_size_(0),
      next_stream_data_(NULL),
      stream_data_len_(0),
      last_status_(FILTER_NEED_MORE_DATA) {}

Filter::FilterStatus Filter::CopyOut(char* dest_buffer, int* dest_len) {
  int out_len;
  int input_len = *dest_len;
  *dest_len = 0;

  if (0 == stream_data_len_)
    return Filter::FILTER_NEED_MORE_DATA;

  out_len = std::min(input_len, stream_data_len_);
  memcpy(dest_buffer, next_stream_data_, out_len);
  *dest_len += out_len;
  stream_data_len_ -= out_len;
  if (0 == stream_data_len_) {
    next_stream_data_ = NULL;
    return Filter::FILTER_NEED_MORE_DATA;
  } else {
    next_stream_data_ += out_len;
    return Filter::FILTER_OK;
  }
}

// static
Filter* Filter::InitGZipFilter(FilterType type_id, int buffer_size) {
  scoped_ptr<GZipFilter> gz_filter(new GZipFilter());
  gz_filter->InitBuffer(buffer_size);
  return gz_filter->InitDecoding(type_id) ? gz_filter.release() : NULL;
}

// static
Filter* Filter::InitSdchFilter(FilterType type_id,
                               const FilterContext& filter_context,
                               int buffer_size) {
  scoped_ptr<SdchFilter> sdch_filter(new SdchFilter(filter_context));
  sdch_filter->InitBuffer(buffer_size);
  return sdch_filter->InitDecoding(type_id) ? sdch_filter.release() : NULL;
}

// static
Filter* Filter::PrependNewFilter(FilterType type_id,
                                 const FilterContext& filter_context,
                                 int buffer_size,
                                 Filter* filter_list) {
  scoped_ptr<Filter> first_filter;  // Soon to be start of chain.
  switch (type_id) {
    case FILTER_TYPE_GZIP_HELPING_SDCH:
    case FILTER_TYPE_DEFLATE:
    case FILTER_TYPE_GZIP:
      first_filter.reset(InitGZipFilter(type_id, buffer_size));
      break;
    case FILTER_TYPE_SDCH:
    case FILTER_TYPE_SDCH_POSSIBLE:
      if (filter_context.GetURLRequestContext()->sdch_manager() &&
          SdchManager::sdch_enabled()) {
        first_filter.reset(
            InitSdchFilter(type_id, filter_context, buffer_size));
      }
      break;
    default:
      break;
  }

  if (!first_filter.get())
    return NULL;

  first_filter->next_filter_.reset(filter_list);
  return first_filter.release();
}

void Filter::InitBuffer(int buffer_size) {
  DCHECK(!stream_buffer());
  DCHECK_GT(buffer_size, 0);
  stream_buffer_ = new IOBuffer(buffer_size);
  stream_buffer_size_ = buffer_size;
}

void Filter::PushDataIntoNextFilter() {
  IOBuffer* next_buffer = next_filter_->stream_buffer();
  int next_size = next_filter_->stream_buffer_size();
  last_status_ = ReadFilteredData(next_buffer->data(), &next_size);
  if (FILTER_ERROR != last_status_)
    next_filter_->FlushStreamBuffer(next_size);
}

}  // namespace net
