// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits.h>

#include <algorithm>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "net/base/io_buffer.h"
#include "net/filter/mock_filter_context.h"
#include "net/filter/sdch_filter.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_http_job.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/zlib/zlib.h"

namespace net {

//------------------------------------------------------------------------------
// Provide sample data and compression results with a sample VCDIFF dictionary.
// Note an SDCH dictionary has extra meta-data before the VCDIFF dictionary.
static const char kTestVcdiffDictionary[] = "DictionaryFor"
    "SdchCompression1SdchCompression2SdchCompression3SdchCompression\n";
// Pre-compression test data.  Note that we pad with a lot of highly gzip
// compressible content to help to exercise the chaining pipeline.  That is why
// there are a PILE of zeros at the start and end.
// This will ensure that gzip compressed data can be fed to the chain in one
// gulp, but (with careful selection of intermediate buffers) that it takes
// several sdch buffers worth of data to satisfy the sdch filter.  See detailed
// CHECK() calls in FilterChaining test for specifics.
static const char kTestData[] = "0000000000000000000000000000000000000000000000"
    "0000000000000000000000000000TestData "
    "SdchCompression1SdchCompression2SdchCompression3SdchCompression"
    "00000000000000000000000000000000000000000000000000000000000000000000000000"
    "000000000000000000000000000000000000000\n";

// Note SDCH compressed data will include a reference to the SDCH dictionary.
static const char kSdchCompressedTestData[] =
    "\326\303\304\0\0\001M\0\201S\202\004\0\201E\006\001"
    "00000000000000000000000000000000000000000000000000000000000000000000000000"
    "TestData 00000000000000000000000000000000000000000000000000000000000000000"
    "000000000000000000000000000000000000000000000000\n\001S\023\077\001r\r";

//------------------------------------------------------------------------------

class SdchFilterTest : public testing::Test {
 protected:
  SdchFilterTest()
    : test_vcdiff_dictionary_(kTestVcdiffDictionary,
                              sizeof(kTestVcdiffDictionary) - 1),
      vcdiff_compressed_data_(kSdchCompressedTestData,
                              sizeof(kSdchCompressedTestData) - 1),
      expanded_(kTestData, sizeof(kTestData) - 1),
      sdch_manager_(new SdchManager),
      filter_context_(new MockFilterContext) {
    URLRequestContext* url_request_context =
        filter_context_->GetModifiableURLRequestContext();

    url_request_context->set_sdch_manager(sdch_manager_.get());
  }

  MockFilterContext* filter_context() { return filter_context_.get(); }

  std::string NewSdchCompressedData(const std::string dictionary);

  const std::string test_vcdiff_dictionary_;
  const std::string vcdiff_compressed_data_;
  const std::string expanded_;  // Desired final, decompressed data.

  scoped_ptr<SdchManager> sdch_manager_;
  scoped_ptr<MockFilterContext> filter_context_;
};

std::string SdchFilterTest::NewSdchCompressedData(
    const std::string dictionary) {
  std::string client_hash;
  std::string server_hash;
  SdchManager::GenerateHash(dictionary, &client_hash, &server_hash);

  // Build compressed data that refers to our dictionary.
  std::string compressed(server_hash);
  compressed.append("\0", 1);
  compressed.append(vcdiff_compressed_data_);
  return compressed;
}

//------------------------------------------------------------------------------


TEST_F(SdchFilterTest, Hashing) {
  std::string client_hash, server_hash;
  std::string dictionary("test contents");
  SdchManager::GenerateHash(dictionary, &client_hash, &server_hash);

  EXPECT_EQ(client_hash, "lMQBjS3P");
  EXPECT_EQ(server_hash, "MyciMVll");
}


//------------------------------------------------------------------------------
// Provide a generic helper function for trying to filter data.
// This function repeatedly calls the filter to process data, until the entire
// source is consumed.  The return value from the filter is appended to output.
// This allows us to vary input and output block sizes in order to test for edge
// effects (boundary effects?) during the filtering process.
// This function provides data to the filter in blocks of no-more-than the
// specified input_block_length.  It allows the filter to fill no more than
// output_buffer_length in any one call to proccess (a.k.a., Read) data, and
// concatenates all these little output blocks into the singular output string.
static bool FilterTestData(const std::string& source,
                           size_t input_block_length,
                           const size_t output_buffer_length,
                           Filter* filter, std::string* output) {
  CHECK_GT(input_block_length, 0u);
  Filter::FilterStatus status(Filter::FILTER_NEED_MORE_DATA);
  size_t source_index = 0;
  scoped_ptr<char[]> output_buffer(new char[output_buffer_length]);
  size_t input_amount = std::min(input_block_length,
      static_cast<size_t>(filter->stream_buffer_size()));

  do {
    int copy_amount = std::min(input_amount, source.size() - source_index);
    if (copy_amount > 0 && status == Filter::FILTER_NEED_MORE_DATA) {
      memcpy(filter->stream_buffer()->data(), source.data() + source_index,
             copy_amount);
      filter->FlushStreamBuffer(copy_amount);
      source_index += copy_amount;
    }
    int buffer_length = output_buffer_length;
    status = filter->ReadData(output_buffer.get(), &buffer_length);
    output->append(output_buffer.get(), buffer_length);
    if (status == Filter::FILTER_ERROR)
      return false;
    // Callers assume that FILTER_OK with no output buffer means FILTER_DONE.
    if (Filter::FILTER_OK == status && 0 == buffer_length)
      return true;
    if (copy_amount == 0 && buffer_length == 0)
      return true;
  } while (1);
}
//------------------------------------------------------------------------------
static std::string NewSdchDictionary(const std::string& domain) {
  std::string dictionary;
  if (!domain.empty()) {
    dictionary.append("Domain: ");
    dictionary.append(domain);
    dictionary.append("\n");
  }
  dictionary.append("\n");
  dictionary.append(kTestVcdiffDictionary, sizeof(kTestVcdiffDictionary) - 1);
  return dictionary;
}

//------------------------------------------------------------------------------

TEST_F(SdchFilterTest, EmptyInputOk) {
  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);
  char output_buffer[20];
  std::string url_string("http://ignore.com");
  filter_context()->SetURL(GURL(url_string));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));


  // With no input data, try to read output.
  int output_bytes_or_buffer_size = sizeof(output_buffer);
  Filter::FilterStatus status = filter->ReadData(output_buffer,
                                                 &output_bytes_or_buffer_size);

  EXPECT_EQ(0, output_bytes_or_buffer_size);
  EXPECT_EQ(Filter::FILTER_NEED_MORE_DATA, status);
}

TEST_F(SdchFilterTest, PassThroughWhenTentative) {
  std::vector<Filter::FilterType> filter_types;
  // Selective a tentative filter (which can fall back to pass through).
  filter_types.push_back(Filter::FILTER_TYPE_GZIP_HELPING_SDCH);
  char output_buffer[20];
  // Response code needs to be 200 to allow a pass through.
  filter_context()->SetResponseCode(200);
  std::string url_string("http://ignore.com");
  filter_context()->SetURL(GURL(url_string));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  // Supply enough data to force a pass-through mode..
  std::string non_gzip_content("not GZIPed data");

  char* input_buffer = filter->stream_buffer()->data();
  int input_buffer_size = filter->stream_buffer_size();

  EXPECT_LT(static_cast<int>(non_gzip_content.size()),
            input_buffer_size);
  memcpy(input_buffer, non_gzip_content.data(),
         non_gzip_content.size());
  filter->FlushStreamBuffer(non_gzip_content.size());

  // Try to read output.
  int output_bytes_or_buffer_size = sizeof(output_buffer);
  Filter::FilterStatus status = filter->ReadData(output_buffer,
                                                 &output_bytes_or_buffer_size);

  EXPECT_EQ(non_gzip_content.size(),
              static_cast<size_t>(output_bytes_or_buffer_size));
  ASSERT_GT(sizeof(output_buffer),
              static_cast<size_t>(output_bytes_or_buffer_size));
  output_buffer[output_bytes_or_buffer_size] = '\0';
  EXPECT_TRUE(non_gzip_content == output_buffer);
  EXPECT_EQ(Filter::FILTER_NEED_MORE_DATA, status);
}

TEST_F(SdchFilterTest, RefreshBadReturnCode) {
  std::vector<Filter::FilterType> filter_types;
  // Selective a tentative filter (which can fall back to pass through).
  filter_types.push_back(Filter::FILTER_TYPE_SDCH_POSSIBLE);
  char output_buffer[20];
  // Response code needs to be 200 to allow a pass through.
  filter_context()->SetResponseCode(403);
  // Meta refresh will only appear for html content
  filter_context()->SetMimeType("text/html");
  std::string url_string("http://ignore.com");
  filter_context()->SetURL(GURL(url_string));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  // Supply enough data to force a pass-through mode, which means we have
  // provided more than 9 characters that can't be a dictionary hash.
  std::string non_sdch_content("This is not SDCH");

  char* input_buffer = filter->stream_buffer()->data();
  int input_buffer_size = filter->stream_buffer_size();

  EXPECT_LT(static_cast<int>(non_sdch_content.size()),
            input_buffer_size);
  memcpy(input_buffer, non_sdch_content.data(),
         non_sdch_content.size());
  filter->FlushStreamBuffer(non_sdch_content.size());

  // Try to read output.
  int output_bytes_or_buffer_size = sizeof(output_buffer);
  Filter::FilterStatus status = filter->ReadData(output_buffer,
                                                 &output_bytes_or_buffer_size);

  // We should have read a long and complicated meta-refresh request.
  EXPECT_TRUE(sizeof(output_buffer) == output_bytes_or_buffer_size);
  // Check at least the prefix of the return.
  EXPECT_EQ(0, strncmp(output_buffer,
      "<head><META HTTP-EQUIV=\"Refresh\" CONTENT=\"0\"></head>",
      sizeof(output_buffer)));
  EXPECT_EQ(Filter::FILTER_OK, status);
}

TEST_F(SdchFilterTest, ErrorOnBadReturnCode) {
  std::vector<Filter::FilterType> filter_types;
  // Selective a tentative filter (which can fall back to pass through).
  filter_types.push_back(Filter::FILTER_TYPE_SDCH_POSSIBLE);
  char output_buffer[20];
  // Response code needs to be 200 to allow a pass through.
  filter_context()->SetResponseCode(403);
  // Meta refresh will only appear for html content, so set to something else
  // to induce an error (we can't meta refresh).
  filter_context()->SetMimeType("anything");
  std::string url_string("http://ignore.com");
  filter_context()->SetURL(GURL(url_string));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  // Supply enough data to force a pass-through mode, which means we have
  // provided more than 9 characters that can't be a dictionary hash.
  std::string non_sdch_content("This is not SDCH");

  char* input_buffer = filter->stream_buffer()->data();
  int input_buffer_size = filter->stream_buffer_size();

  EXPECT_LT(static_cast<int>(non_sdch_content.size()),
            input_buffer_size);
  memcpy(input_buffer, non_sdch_content.data(),
         non_sdch_content.size());
  filter->FlushStreamBuffer(non_sdch_content.size());

  // Try to read output.
  int output_bytes_or_buffer_size = sizeof(output_buffer);
  Filter::FilterStatus status = filter->ReadData(output_buffer,
                                                 &output_bytes_or_buffer_size);

  EXPECT_EQ(0, output_bytes_or_buffer_size);
  EXPECT_EQ(Filter::FILTER_ERROR, status);
}

TEST_F(SdchFilterTest, ErrorOnBadReturnCodeWithHtml) {
  std::vector<Filter::FilterType> filter_types;
  // Selective a tentative filter (which can fall back to pass through).
  filter_types.push_back(Filter::FILTER_TYPE_SDCH_POSSIBLE);
  char output_buffer[20];
  // Response code needs to be 200 to allow a pass through.
  filter_context()->SetResponseCode(403);
  // Meta refresh will only appear for html content
  filter_context()->SetMimeType("text/html");
  std::string url_string("http://ignore.com");
  filter_context()->SetURL(GURL(url_string));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  // Supply enough data to force a pass-through mode, which means we have
  // provided more than 9 characters that can't be a dictionary hash.
  std::string non_sdch_content("This is not SDCH");

  char* input_buffer = filter->stream_buffer()->data();
  int input_buffer_size = filter->stream_buffer_size();

  EXPECT_LT(static_cast<int>(non_sdch_content.size()),
            input_buffer_size);
  memcpy(input_buffer, non_sdch_content.data(),
         non_sdch_content.size());
  filter->FlushStreamBuffer(non_sdch_content.size());

  // Try to read output.
  int output_bytes_or_buffer_size = sizeof(output_buffer);
  Filter::FilterStatus status = filter->ReadData(output_buffer,
                                                 &output_bytes_or_buffer_size);

  // We should have read a long and complicated meta-refresh request.
  EXPECT_EQ(sizeof(output_buffer),
            static_cast<size_t>(output_bytes_or_buffer_size));
  // Check at least the prefix of the return.
  EXPECT_EQ(0, strncmp(output_buffer,
      "<head><META HTTP-EQUIV=\"Refresh\" CONTENT=\"0\"></head>",
      sizeof(output_buffer)));
  EXPECT_EQ(Filter::FILTER_OK, status);
}

TEST_F(SdchFilterTest, BasicBadDictionary) {
  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);
  char output_buffer[20];
  std::string url_string("http://ignore.com");
  filter_context()->SetURL(GURL(url_string));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  // Supply bogus data (which doesn't yet specify a full dictionary hash).
  // Dictionary hash is 8 characters followed by a null.
  std::string dictionary_hash_prefix("123");

  char* input_buffer = filter->stream_buffer()->data();
  int input_buffer_size = filter->stream_buffer_size();

  EXPECT_LT(static_cast<int>(dictionary_hash_prefix.size()),
            input_buffer_size);
  memcpy(input_buffer, dictionary_hash_prefix.data(),
         dictionary_hash_prefix.size());
  filter->FlushStreamBuffer(dictionary_hash_prefix.size());

  // With less than a dictionary specifier, try to read output.
  int output_bytes_or_buffer_size = sizeof(output_buffer);
  Filter::FilterStatus status = filter->ReadData(output_buffer,
                                                 &output_bytes_or_buffer_size);

  EXPECT_EQ(0, output_bytes_or_buffer_size);
  EXPECT_EQ(Filter::FILTER_NEED_MORE_DATA, status);

  // Provide enough data to complete *a* hash, but it is bogus, and not in our
  // list of dictionaries, so the filter should error out immediately.
  std::string dictionary_hash_postfix("4abcd\0", 6);

  CHECK_LT(dictionary_hash_postfix.size(),
           static_cast<size_t>(input_buffer_size));
  memcpy(input_buffer, dictionary_hash_postfix.data(),
         dictionary_hash_postfix.size());
  filter->FlushStreamBuffer(dictionary_hash_postfix.size());

  // With a non-existant dictionary specifier, try to read output.
  output_bytes_or_buffer_size = sizeof(output_buffer);
  status = filter->ReadData(output_buffer, &output_bytes_or_buffer_size);

  EXPECT_EQ(0, output_bytes_or_buffer_size);
  EXPECT_EQ(Filter::FILTER_ERROR, status);

  EXPECT_FALSE(sdch_manager_->IsInSupportedDomain(GURL(url_string)));
  sdch_manager_->ClearBlacklistings();
  EXPECT_TRUE(sdch_manager_->IsInSupportedDomain(GURL(url_string)));
}

TEST_F(SdchFilterTest, DictionaryAddOnce) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;
  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  // Check we can't add it twice.
  EXPECT_FALSE(sdch_manager_->AddSdchDictionary(dictionary, url));

  const std::string kSampleDomain2 = "sdchtest2.com";

  // Don't test adding a second dictionary if our limits are tight.
  if (SdchManager::kMaxDictionaryCount > 1) {
    // Construct a second SDCH dictionary from a VCDIFF dictionary.
    std::string dictionary2(NewSdchDictionary(kSampleDomain2));

    std::string url_string2 = "http://" + kSampleDomain2;
    GURL url2(url_string2);
    EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary2, url2));
  }
}

TEST_F(SdchFilterTest, BasicDictionary) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string compressed(NewSdchCompressedData(dictionary));

  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  filter_context()->SetURL(url);

  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  size_t feed_block_size = 100;
  size_t output_block_size = 100;
  std::string output;
  EXPECT_TRUE(FilterTestData(compressed, feed_block_size, output_block_size,
                             filter.get(), &output));
  EXPECT_EQ(output, expanded_);

  // Decode with really small buffers (size 1) to check for edge effects.
  filter.reset(Filter::Factory(filter_types, *filter_context()));

  feed_block_size = 1;
  output_block_size = 1;
  output.clear();
  EXPECT_TRUE(FilterTestData(compressed, feed_block_size, output_block_size,
                             filter.get(), &output));
  EXPECT_EQ(output, expanded_);
}

TEST_F(SdchFilterTest, NoDecodeHttps) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string compressed(NewSdchCompressedData(dictionary));

  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  filter_context()->SetURL(GURL("https://" + kSampleDomain));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  const size_t feed_block_size(100);
  const size_t output_block_size(100);
  std::string output;

  EXPECT_FALSE(FilterTestData(compressed, feed_block_size, output_block_size,
                             filter.get(), &output));
}

// Current failsafe TODO/hack refuses to decode any content that doesn't use
// http as the scheme (see use of DICTIONARY_SELECTED_FOR_NON_HTTP).
// The following tests this blockage.  Note that blacklisting results, so we
// we need separate tests for each of these.
TEST_F(SdchFilterTest, NoDecodeFtp) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string compressed(NewSdchCompressedData(dictionary));

  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  filter_context()->SetURL(GURL("ftp://" + kSampleDomain));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  const size_t feed_block_size(100);
  const size_t output_block_size(100);
  std::string output;

  EXPECT_FALSE(FilterTestData(compressed, feed_block_size, output_block_size,
                             filter.get(), &output));
}

TEST_F(SdchFilterTest, NoDecodeFileColon) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string compressed(NewSdchCompressedData(dictionary));

  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  filter_context()->SetURL(GURL("file://" + kSampleDomain));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  const size_t feed_block_size(100);
  const size_t output_block_size(100);
  std::string output;

  EXPECT_FALSE(FilterTestData(compressed, feed_block_size, output_block_size,
                             filter.get(), &output));
}

TEST_F(SdchFilterTest, NoDecodeAboutColon) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string compressed(NewSdchCompressedData(dictionary));

  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  filter_context()->SetURL(GURL("about://" + kSampleDomain));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  const size_t feed_block_size(100);
  const size_t output_block_size(100);
  std::string output;

  EXPECT_FALSE(FilterTestData(compressed, feed_block_size, output_block_size,
                             filter.get(), &output));
}

TEST_F(SdchFilterTest, NoDecodeJavaScript) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string compressed(NewSdchCompressedData(dictionary));

  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  filter_context()->SetURL(GURL("javascript://" + kSampleDomain));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  const size_t feed_block_size(100);
  const size_t output_block_size(100);
  std::string output;

  EXPECT_FALSE(FilterTestData(compressed, feed_block_size, output_block_size,
                             filter.get(), &output));
}

TEST_F(SdchFilterTest, CanStillDecodeHttp) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string compressed(NewSdchCompressedData(dictionary));

  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  filter_context()->SetURL(GURL("http://" + kSampleDomain));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  const size_t feed_block_size(100);
  const size_t output_block_size(100);
  std::string output;

  EXPECT_TRUE(FilterTestData(compressed, feed_block_size, output_block_size,
                             filter.get(), &output));
}

TEST_F(SdchFilterTest, CrossDomainDictionaryUse) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string compressed(NewSdchCompressedData(dictionary));

  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  // Decode with content arriving from the "wrong" domain.
  // This tests SdchManager::CanSet().
  GURL wrong_domain_url("http://www.wrongdomain.com");
  filter_context()->SetURL(wrong_domain_url);
  scoped_ptr<Filter> filter(Filter::Factory(filter_types,  *filter_context()));

  size_t feed_block_size = 100;
  size_t output_block_size = 100;
  std::string output;
  EXPECT_FALSE(FilterTestData(compressed, feed_block_size, output_block_size,
                              filter.get(), &output));
  EXPECT_EQ(output.size(), 0u);  // No output written.

  EXPECT_TRUE(sdch_manager_->IsInSupportedDomain(GURL(url_string)));
  EXPECT_FALSE(sdch_manager_->IsInSupportedDomain(wrong_domain_url));
  sdch_manager_->ClearBlacklistings();
  EXPECT_TRUE(sdch_manager_->IsInSupportedDomain(wrong_domain_url));
}

TEST_F(SdchFilterTest, DictionaryPathValidation) {
  // Can't test path distinction between dictionaries if we aren't allowed
  // more than one dictionary.
  if (SdchManager::kMaxDictionaryCount <= 1)
    return;

  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  // Create a dictionary with a path restriction, by prefixing dictionary.
  const std::string path("/special_path/bin");
  std::string dictionary_with_path("Path: " + path + "\n");
  dictionary_with_path.append(dictionary);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary_with_path, url));

  std::string compressed_for_path(NewSdchCompressedData(dictionary_with_path));

  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  // Test decode the path data, arriving from a valid path.
  filter_context()->SetURL(GURL(url_string + path));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  size_t feed_block_size = 100;
  size_t output_block_size = 100;
  std::string output;

  EXPECT_TRUE(FilterTestData(compressed_for_path, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);

  // Test decode the path data, arriving from a invalid path.
  filter_context()->SetURL(GURL(url_string));
  filter.reset(Filter::Factory(filter_types, *filter_context()));

  feed_block_size = 100;
  output_block_size = 100;
  output.clear();
  EXPECT_FALSE(FilterTestData(compressed_for_path, feed_block_size,
                              output_block_size, filter.get(), &output));
  EXPECT_EQ(output.size(), 0u);  // No output written.

  EXPECT_FALSE(sdch_manager_->IsInSupportedDomain(GURL(url_string)));
  sdch_manager_->ClearBlacklistings();
  EXPECT_TRUE(sdch_manager_->IsInSupportedDomain(GURL(url_string)));
}

TEST_F(SdchFilterTest, DictionaryPortValidation) {
  // Can't test port distinction between dictionaries if we aren't allowed
  // more than one dictionary.
  if (SdchManager::kMaxDictionaryCount <= 1)
    return;

  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));


  // Create a dictionary with a port restriction, by prefixing old dictionary.
  const std::string port("502");
  std::string dictionary_with_port("Port: " + port + "\n");
  dictionary_with_port.append("Port: 80\n");  // Add default port.
  dictionary_with_port.append(dictionary);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary_with_port,
                                            GURL(url_string + ":" + port)));

  std::string compressed_for_port(NewSdchCompressedData(dictionary_with_port));

  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  // Test decode the port data, arriving from a valid port.
  filter_context()->SetURL(GURL(url_string + ":" + port));
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  size_t feed_block_size = 100;
  size_t output_block_size = 100;
  std::string output;
  EXPECT_TRUE(FilterTestData(compressed_for_port, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);

  // Test decode the port data, arriving from a valid (default) port.
  filter_context()->SetURL(GURL(url_string));  // Default port.
  filter.reset(Filter::Factory(filter_types, *filter_context()));

  feed_block_size = 100;
  output_block_size = 100;
  output.clear();
  EXPECT_TRUE(FilterTestData(compressed_for_port, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);

  // Test decode the port data, arriving from a invalid port.
  filter_context()->SetURL(GURL(url_string + ":" + port + "1"));
  filter.reset(Filter::Factory(filter_types, *filter_context()));

  feed_block_size = 100;
  output_block_size = 100;
  output.clear();
  EXPECT_FALSE(FilterTestData(compressed_for_port, feed_block_size,
                              output_block_size, filter.get(), &output));
  EXPECT_EQ(output.size(), 0u);  // No output written.

  EXPECT_FALSE(sdch_manager_->IsInSupportedDomain(GURL(url_string)));
  sdch_manager_->ClearBlacklistings();
  EXPECT_TRUE(sdch_manager_->IsInSupportedDomain(GURL(url_string)));
}

//------------------------------------------------------------------------------
// Helper function to perform gzip compression of data.

static std::string gzip_compress(const std::string &input) {
  z_stream zlib_stream;
  memset(&zlib_stream, 0, sizeof(zlib_stream));
  int code;

  // Initialize zlib
  code = deflateInit2(&zlib_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                      -MAX_WBITS,
                      8,  // DEF_MEM_LEVEL
                      Z_DEFAULT_STRATEGY);

  CHECK_EQ(Z_OK, code);

  // Fill in zlib control block
  zlib_stream.next_in = bit_cast<Bytef*>(input.data());
  zlib_stream.avail_in = input.size();

  // Assume we can compress into similar buffer (add 100 bytes to be sure).
  size_t gzip_compressed_length = zlib_stream.avail_in + 100;
  scoped_ptr<char[]> gzip_compressed(new char[gzip_compressed_length]);
  zlib_stream.next_out = bit_cast<Bytef*>(gzip_compressed.get());
  zlib_stream.avail_out = gzip_compressed_length;

  // The GZIP header (see RFC 1952):
  //   +---+---+---+---+---+---+---+---+---+---+
  //   |ID1|ID2|CM |FLG|     MTIME     |XFL|OS |
  //   +---+---+---+---+---+---+---+---+---+---+
  //     ID1     \037
  //     ID2     \213
  //     CM      \010 (compression method == DEFLATE)
  //     FLG     \000 (special flags that we do not support)
  //     MTIME   Unix format modification time (0 means not available)
  //     XFL     2-4? DEFLATE flags
  //     OS      ???? Operating system indicator (255 means unknown)
  //
  // Header value we generate:
  const char kGZipHeader[] = { '\037', '\213', '\010', '\000', '\000',
                               '\000', '\000', '\000', '\002', '\377' };
  CHECK_GT(zlib_stream.avail_out, sizeof(kGZipHeader));
  memcpy(zlib_stream.next_out, kGZipHeader, sizeof(kGZipHeader));
  zlib_stream.next_out += sizeof(kGZipHeader);
  zlib_stream.avail_out -= sizeof(kGZipHeader);

  // Do deflate
  code = deflate(&zlib_stream, Z_FINISH);
  gzip_compressed_length -= zlib_stream.avail_out;
  std::string compressed(gzip_compressed.get(), gzip_compressed_length);
  deflateEnd(&zlib_stream);
  return compressed;
}

//------------------------------------------------------------------------------

class SdchFilterChainingTest {
 public:
  static Filter* Factory(const std::vector<Filter::FilterType>& types,
                           const FilterContext& context, int size) {
    return Filter::FactoryForTests(types, context, size);
  }
};

// Test that filters can be cascaded (chained) so that the output of one filter
// is processed by the next one.  This is most critical for SDCH, which is
// routinely followed by gzip (during encoding).  The filter we'll test for will
// do the gzip decoding first, and then decode the SDCH content.
TEST_F(SdchFilterTest, FilterChaining) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string sdch_compressed(NewSdchCompressedData(dictionary));

  // Use Gzip to compress the sdch sdch_compressed data.
  std::string gzip_compressed_sdch = gzip_compress(sdch_compressed);

  // Construct a chained filter.
  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);
  filter_types.push_back(Filter::FILTER_TYPE_GZIP);

  // First try with a large buffer (larger than test input, or compressed data).
  const size_t kLargeInputBufferSize(1000);  // Used internally in filters.
  CHECK_GT(kLargeInputBufferSize, gzip_compressed_sdch.size());
  CHECK_GT(kLargeInputBufferSize, sdch_compressed.size());
  CHECK_GT(kLargeInputBufferSize, expanded_.size());
  filter_context()->SetURL(url);
  scoped_ptr<Filter> filter(
      SdchFilterChainingTest::Factory(filter_types, *filter_context(),
                                      kLargeInputBufferSize));
  EXPECT_EQ(static_cast<int>(kLargeInputBufferSize),
            filter->stream_buffer_size());

  // Verify that chained filter is waiting for data.
  char tiny_output_buffer[10];
  int tiny_output_size = sizeof(tiny_output_buffer);
  EXPECT_EQ(Filter::FILTER_NEED_MORE_DATA,
            filter->ReadData(tiny_output_buffer, &tiny_output_size));

  // Make chain process all data.
  size_t feed_block_size = kLargeInputBufferSize;
  size_t output_block_size = kLargeInputBufferSize;
  std::string output;
  EXPECT_TRUE(FilterTestData(gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);

  // Next try with a mid-sized internal buffer size.
  const size_t kMidSizedInputBufferSize(100);
  // Buffer should be big enough to swallow whole gzip content.
  CHECK_GT(kMidSizedInputBufferSize, gzip_compressed_sdch.size());
  // Buffer should be small enough that entire SDCH content can't fit.
  // We'll go even further, and force the chain to flush the buffer between the
  // two filters more than once (that is why we multiply by 2).
  CHECK_LT(kMidSizedInputBufferSize * 2, sdch_compressed.size());
  filter_context()->SetURL(url);
  filter.reset(
      SdchFilterChainingTest::Factory(filter_types, *filter_context(),
                                      kMidSizedInputBufferSize));
  EXPECT_EQ(static_cast<int>(kMidSizedInputBufferSize),
            filter->stream_buffer_size());

  feed_block_size = kMidSizedInputBufferSize;
  output_block_size = kMidSizedInputBufferSize;
  output.clear();
  EXPECT_TRUE(FilterTestData(gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);

  // Next try with a tiny input and output buffer to cover edge effects.
  filter.reset(SdchFilterChainingTest::Factory(filter_types, *filter_context(),
                                               kLargeInputBufferSize));
  EXPECT_EQ(static_cast<int>(kLargeInputBufferSize),
            filter->stream_buffer_size());

  feed_block_size = 1;
  output_block_size = 1;
  output.clear();
  EXPECT_TRUE(FilterTestData(gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);
}

TEST_F(SdchFilterTest, DefaultGzipIfSdch) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string sdch_compressed(NewSdchCompressedData(dictionary));

  // Use Gzip to compress the sdch sdch_compressed data.
  std::string gzip_compressed_sdch = gzip_compress(sdch_compressed);

  // Only claim to have sdch content, but really use the gzipped sdch content.
  // System should automatically add the missing (optional) gzip.
  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_SDCH);

  filter_context()->SetMimeType("anything/mime");
  filter_context()->SetSdchResponse(true);
  Filter::FixupEncodingTypes(*filter_context(), &filter_types);
  ASSERT_EQ(filter_types.size(), 2u);
  EXPECT_EQ(filter_types[0], Filter::FILTER_TYPE_SDCH);
  EXPECT_EQ(filter_types[1], Filter::FILTER_TYPE_GZIP_HELPING_SDCH);

  // First try with a large buffer (larger than test input, or compressed data).
  filter_context()->SetURL(url);
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));


  // Verify that chained filter is waiting for data.
  char tiny_output_buffer[10];
  int tiny_output_size = sizeof(tiny_output_buffer);
  EXPECT_EQ(Filter::FILTER_NEED_MORE_DATA,
            filter->ReadData(tiny_output_buffer, &tiny_output_size));

  size_t feed_block_size = 100;
  size_t output_block_size = 100;
  std::string output;
  EXPECT_TRUE(FilterTestData(gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);

  // Next try with a tiny buffer to cover edge effects.
  filter.reset(Filter::Factory(filter_types, *filter_context()));

  feed_block_size = 1;
  output_block_size = 1;
  output.clear();
  EXPECT_TRUE(FilterTestData(gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);
}

TEST_F(SdchFilterTest, AcceptGzipSdchIfGzip) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string sdch_compressed(NewSdchCompressedData(dictionary));

  // Use Gzip to compress the sdch sdch_compressed data.
  std::string gzip_compressed_sdch = gzip_compress(sdch_compressed);

  // Some proxies strip the content encoding statement down to a mere gzip, but
  // pass through the original content (with full sdch,gzip encoding).
  // Only claim to have gzip content, but really use the gzipped sdch content.
  // System should automatically add the missing (optional) sdch.
  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_GZIP);

  filter_context()->SetMimeType("anything/mime");
  filter_context()->SetSdchResponse(true);
  Filter::FixupEncodingTypes(*filter_context(), &filter_types);
  ASSERT_EQ(filter_types.size(), 3u);
  EXPECT_EQ(filter_types[0], Filter::FILTER_TYPE_SDCH_POSSIBLE);
  EXPECT_EQ(filter_types[1], Filter::FILTER_TYPE_GZIP_HELPING_SDCH);
  EXPECT_EQ(filter_types[2], Filter::FILTER_TYPE_GZIP);

  // First try with a large buffer (larger than test input, or compressed data).
  filter_context()->SetURL(url);
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));


  // Verify that chained filter is waiting for data.
  char tiny_output_buffer[10];
  int tiny_output_size = sizeof(tiny_output_buffer);
  EXPECT_EQ(Filter::FILTER_NEED_MORE_DATA,
            filter->ReadData(tiny_output_buffer, &tiny_output_size));

  size_t feed_block_size = 100;
  size_t output_block_size = 100;
  std::string output;
  EXPECT_TRUE(FilterTestData(gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);

  // Next try with a tiny buffer to cover edge effects.
  filter.reset(Filter::Factory(filter_types, *filter_context()));

  feed_block_size = 1;
  output_block_size = 1;
  output.clear();
  EXPECT_TRUE(FilterTestData(gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);
}

TEST_F(SdchFilterTest, DefaultSdchGzipIfEmpty) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string sdch_compressed(NewSdchCompressedData(dictionary));

  // Use Gzip to compress the sdch sdch_compressed data.
  std::string gzip_compressed_sdch = gzip_compress(sdch_compressed);

  // Only claim to have non-encoded content, but really use the gzipped sdch
  // content.
  // System should automatically add the missing (optional) sdch,gzip.
  std::vector<Filter::FilterType> filter_types;

  filter_context()->SetMimeType("anything/mime");
  filter_context()->SetSdchResponse(true);
  Filter::FixupEncodingTypes(*filter_context(), &filter_types);
  ASSERT_EQ(filter_types.size(), 2u);
  EXPECT_EQ(filter_types[0], Filter::FILTER_TYPE_SDCH_POSSIBLE);
  EXPECT_EQ(filter_types[1], Filter::FILTER_TYPE_GZIP_HELPING_SDCH);

  // First try with a large buffer (larger than test input, or compressed data).
  filter_context()->SetURL(url);
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));


  // Verify that chained filter is waiting for data.
  char tiny_output_buffer[10];
  int tiny_output_size = sizeof(tiny_output_buffer);
  EXPECT_EQ(Filter::FILTER_NEED_MORE_DATA,
            filter->ReadData(tiny_output_buffer, &tiny_output_size));

  size_t feed_block_size = 100;
  size_t output_block_size = 100;
  std::string output;
  EXPECT_TRUE(FilterTestData(gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);

  // Next try with a tiny buffer to cover edge effects.
  filter.reset(Filter::Factory(filter_types, *filter_context()));

  feed_block_size = 1;
  output_block_size = 1;
  output.clear();
  EXPECT_TRUE(FilterTestData(gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);
}

TEST_F(SdchFilterTest, AcceptGzipGzipSdchIfGzip) {
  // Construct a valid SDCH dictionary from a VCDIFF dictionary.
  const std::string kSampleDomain = "sdchtest.com";
  std::string dictionary(NewSdchDictionary(kSampleDomain));

  std::string url_string = "http://" + kSampleDomain;

  GURL url(url_string);
  EXPECT_TRUE(sdch_manager_->AddSdchDictionary(dictionary, url));

  std::string sdch_compressed(NewSdchCompressedData(dictionary));

  // Vodaphone (UK) Mobile Broadband provides double gzipped sdch with a content
  // encoding of merely gzip (apparently, only listing the extra level of
  // wrapper compression they added, but discarding the actual content encoding.
  // Use Gzip to double compress the sdch sdch_compressed data.
  std::string double_gzip_compressed_sdch = gzip_compress(gzip_compress(
      sdch_compressed));

  // Only claim to have gzip content, but really use the double gzipped sdch
  // content.
  // System should automatically add the missing (optional) sdch, gzip decoders.
  std::vector<Filter::FilterType> filter_types;
  filter_types.push_back(Filter::FILTER_TYPE_GZIP);

  filter_context()->SetMimeType("anything/mime");
  filter_context()->SetSdchResponse(true);
  Filter::FixupEncodingTypes(*filter_context(), &filter_types);
  ASSERT_EQ(filter_types.size(), 3u);
  EXPECT_EQ(filter_types[0], Filter::FILTER_TYPE_SDCH_POSSIBLE);
  EXPECT_EQ(filter_types[1], Filter::FILTER_TYPE_GZIP_HELPING_SDCH);
  EXPECT_EQ(filter_types[2], Filter::FILTER_TYPE_GZIP);

  // First try with a large buffer (larger than test input, or compressed data).
  filter_context()->SetURL(url);
  scoped_ptr<Filter> filter(Filter::Factory(filter_types, *filter_context()));

  // Verify that chained filter is waiting for data.
  char tiny_output_buffer[10];
  int tiny_output_size = sizeof(tiny_output_buffer);
  EXPECT_EQ(Filter::FILTER_NEED_MORE_DATA,
            filter->ReadData(tiny_output_buffer, &tiny_output_size));

  size_t feed_block_size = 100;
  size_t output_block_size = 100;
  std::string output;
  EXPECT_TRUE(FilterTestData(double_gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);

  // Next try with a tiny buffer to cover edge effects.
  filter.reset(Filter::Factory(filter_types, *filter_context()));

  feed_block_size = 1;
  output_block_size = 1;
  output.clear();
  EXPECT_TRUE(FilterTestData(double_gzip_compressed_sdch, feed_block_size,
                             output_block_size, filter.get(), &output));
  EXPECT_EQ(output, expanded_);
}

}  // namespace net
