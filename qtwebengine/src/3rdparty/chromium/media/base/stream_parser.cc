// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/stream_parser.h"

#include "media/base/buffers.h"
#include "media/base/stream_parser_buffer.h"

namespace media {

StreamParser::InitParameters::InitParameters(base::TimeDelta duration)
    : duration(duration),
      auto_update_timestamp_offset(false),
      liveness(Demuxer::LIVENESS_UNKNOWN) {
}

StreamParser::StreamParser() {}

StreamParser::~StreamParser() {}

static bool MergeBufferQueuesInternal(
    const std::vector<const StreamParser::BufferQueue*>& buffer_queues,
    StreamParser::BufferQueue* merged_buffers) {
  // Instead of std::merge usage, this method implements a custom merge because:
  // 1) |buffer_queues| may contain N queues,
  // 2) we must detect and return false if any of the queues in |buffer_queues|
  // is unsorted, and
  // 3) we must detect and return false if any of the buffers in |buffer_queues|
  // has a decode timestamp prior to the last, if any, buffer in
  // |merged_buffers|.
  // TODO(wolenetz/acolwell): Refactor stream parsers to eliminate need for
  // this large grain merge. See http://crbug.com/338484.

  // Done if no inputs to merge.
  if (buffer_queues.empty())
    return true;

  // Build a vector of iterators, one for each input, to traverse inputs.
  // The union of these iterators points to the set of candidate buffers
  // for being appended to |merged_buffers|.
  size_t num_itrs = buffer_queues.size();
  std::vector<StreamParser::BufferQueue::const_iterator> itrs(num_itrs);
  for (size_t i = 0; i < num_itrs; ++i)
    itrs[i] = buffer_queues[i]->begin();

  // |last_decode_timestamp| tracks the lower bound, if any, that all candidate
  // buffers must not be less than. If |merged_buffers| already has buffers,
  // initialize |last_decode_timestamp| to the decode timestamp of the last
  // buffer in it.
  base::TimeDelta last_decode_timestamp = kNoTimestamp();
  if (!merged_buffers->empty())
    last_decode_timestamp = merged_buffers->back()->GetDecodeTimestamp();

  // Repeatedly select and append the next buffer from the candidate buffers
  // until either:
  // 1) returning false, to indicate detection of decreasing DTS in some queue,
  //    when a candidate buffer has decode timestamp below
  //    |last_decode_timestamp|, which means either an input buffer wasn't
  //    sorted correctly or had a buffer with decode timestamp below the last
  //    buffer, if any, in |merged_buffers|, or
  // 2) returning true when all buffers have been merged successfully;
  //    equivalently, when all of the iterators in |itrs| have reached the end
  //    of their respective queue from |buffer_queues|.
  // TODO(wolenetz/acolwell): Ideally, we would use a heap to store the head of
  // all queues and pop the head with lowest decode timestamp in log(N) time.
  // However, N will typically be small and usage of this implementation is
  // meant to be short-term. See http://crbug.com/338484.
  while (true) {
    // Tracks which queue's iterator is pointing to the candidate buffer to
    // append next, or -1 if no candidate buffers found. This indexes |itrs|.
    int index_of_queue_with_next_decode_timestamp = -1;
    base::TimeDelta next_decode_timestamp = kNoTimestamp();

    // Scan each of the iterators for |buffer_queues| to find the candidate
    // buffer, if any, that has the lowest decode timestamp.
    for (size_t i = 0; i < num_itrs; ++i) {
      if (itrs[i] == buffer_queues[i]->end())
        continue;

      // Extract the candidate buffer's decode timestamp.
      base::TimeDelta ts = (*itrs[i])->GetDecodeTimestamp();

      if (last_decode_timestamp != kNoTimestamp() &&
          ts < last_decode_timestamp)
        return false;

      if (ts < next_decode_timestamp ||
          next_decode_timestamp == kNoTimestamp()) {
        // Remember the decode timestamp and queue iterator index for this
        // potentially winning candidate buffer.
        next_decode_timestamp = ts;
        index_of_queue_with_next_decode_timestamp = i;
      }
    }

    // All done if no further candidate buffers exist.
    if (index_of_queue_with_next_decode_timestamp == -1)
      return true;

    // Otherwise, append the winning candidate buffer to |merged_buffers|,
    // remember its decode timestamp as |last_decode_timestamp| now that it is
    // the last buffer in |merged_buffers|, advance the corresponding
    // input BufferQueue iterator, and continue.
    scoped_refptr<StreamParserBuffer> buffer =
        *itrs[index_of_queue_with_next_decode_timestamp];
    last_decode_timestamp = buffer->GetDecodeTimestamp();
    merged_buffers->push_back(buffer);
    ++itrs[index_of_queue_with_next_decode_timestamp];
  }
}

bool MergeBufferQueues(const StreamParser::BufferQueue& audio_buffers,
                       const StreamParser::BufferQueue& video_buffers,
                       const StreamParser::TextBufferQueueMap& text_buffers,
                       StreamParser::BufferQueue* merged_buffers) {
  DCHECK(merged_buffers);

  // Prepare vector containing pointers to any provided non-empty buffer queues.
  std::vector<const StreamParser::BufferQueue*> buffer_queues;
  if (!audio_buffers.empty())
    buffer_queues.push_back(&audio_buffers);
  if (!video_buffers.empty())
    buffer_queues.push_back(&video_buffers);
  for (StreamParser::TextBufferQueueMap::const_iterator map_itr =
           text_buffers.begin();
       map_itr != text_buffers.end();
       map_itr++) {
    if (!map_itr->second.empty())
      buffer_queues.push_back(&(map_itr->second));
  }

  // Do the merge.
  return MergeBufferQueuesInternal(buffer_queues, merged_buffers);
}

}  // namespace media
