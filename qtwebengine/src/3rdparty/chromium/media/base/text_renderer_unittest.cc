// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/memory/scoped_vector.h"
#include "base/message_loop/message_loop.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/decoder_buffer.h"
#include "media/base/demuxer_stream.h"
#include "media/base/fake_text_track_stream.h"
#include "media/base/text_renderer.h"
#include "media/base/text_track_config.h"
#include "media/base/video_decoder_config.h"
#include "media/filters/webvtt_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Eq;
using ::testing::Exactly;
using ::testing::Invoke;
using ::testing::_;

namespace media {

// Local implementation of the TextTrack interface.
class FakeTextTrack : public TextTrack {
 public:
  FakeTextTrack(const base::Closure& destroy_cb,
                const TextTrackConfig& config)
      : destroy_cb_(destroy_cb),
        config_(config) {
  }
  virtual ~FakeTextTrack() {
    destroy_cb_.Run();
  }

  MOCK_METHOD5(addWebVTTCue, void(const base::TimeDelta& start,
                                  const base::TimeDelta& end,
                                  const std::string& id,
                                  const std::string& content,
                                  const std::string& settings));

  const base::Closure destroy_cb_;
  const TextTrackConfig config_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeTextTrack);
};

class TextRendererTest : public testing::Test {
 public:
  TextRendererTest() {}

  void CreateTextRenderer() {
    DCHECK(!text_renderer_);

    text_renderer_.reset(
        new TextRenderer(message_loop_.message_loop_proxy(),
                         base::Bind(&TextRendererTest::OnAddTextTrack,
                                    base::Unretained(this))));
    text_renderer_->Initialize(base::Bind(&TextRendererTest::OnEnd,
                                          base::Unretained(this)));
  }

  void DestroyTextRenderer() {
    EXPECT_CALL(*this, OnStop());
    text_renderer_->Stop(base::Bind(&TextRendererTest::OnStop,
                                    base::Unretained(this)));
    message_loop_.RunUntilIdle();

    text_renderer_.reset();
    text_track_streams_.clear();
  }

  void AddTextTrack(TextKind kind,
                    const std::string& name,
                    const std::string& language,
                    bool expect_read) {
    const size_t idx = text_track_streams_.size();
    text_track_streams_.push_back(new FakeTextTrackStream);

    if (expect_read)
      ExpectRead(idx);

    const TextTrackConfig config(kind, name, language, std::string());
    text_renderer_->AddTextStream(text_track_streams_.back(), config);
    message_loop_.RunUntilIdle();

    EXPECT_EQ(text_tracks_.size(), text_track_streams_.size());
    FakeTextTrack* const text_track = text_tracks_.back();
    EXPECT_TRUE(text_track);
    EXPECT_TRUE(text_track->config_.Matches(config));
  }

  void OnAddTextTrack(const TextTrackConfig& config,
                      const AddTextTrackDoneCB& done_cb) {
    base::Closure destroy_cb =
        base::Bind(&TextRendererTest::OnDestroyTextTrack,
                   base::Unretained(this),
                   text_tracks_.size());
    // Text track objects are owned by the text renderer, but we cache them
    // here so we can inspect them.  They get removed from our cache when the
    // text renderer deallocates them.
    text_tracks_.push_back(new FakeTextTrack(destroy_cb, config));
    scoped_ptr<TextTrack> text_track(text_tracks_.back());
    done_cb.Run(text_track.Pass());
  }

  void RemoveTextTrack(unsigned idx) {
    FakeTextTrackStream* const stream = text_track_streams_[idx];
    text_renderer_->RemoveTextStream(stream);
    EXPECT_FALSE(text_tracks_[idx]);
  }

  void SatisfyPendingReads(const base::TimeDelta& start,
                           const base::TimeDelta& duration,
                           const std::string& id,
                           const std::string& content,
                           const std::string& settings) {
    for (TextTrackStreams::iterator itr = text_track_streams_.begin();
         itr != text_track_streams_.end(); ++itr) {
      (*itr)->SatisfyPendingRead(start, duration, id, content, settings);
    }
  }

  void AbortPendingRead(unsigned idx) {
    FakeTextTrackStream* const stream = text_track_streams_[idx];
    stream->AbortPendingRead();
    message_loop_.RunUntilIdle();
  }

  void AbortPendingReads() {
    for (size_t idx = 0; idx < text_track_streams_.size(); ++idx) {
      AbortPendingRead(idx);
    }
  }

  void SendEosNotification(unsigned idx) {
    FakeTextTrackStream* const stream = text_track_streams_[idx];
    stream->SendEosNotification();
    message_loop_.RunUntilIdle();
  }

  void SendEosNotifications() {
    for (size_t idx = 0; idx < text_track_streams_.size(); ++idx) {
      SendEosNotification(idx);
    }
  }

  void SendCue(unsigned idx, bool expect_cue) {
    FakeTextTrackStream* const text_stream = text_track_streams_[idx];

    const base::TimeDelta start;
    const base::TimeDelta duration = base::TimeDelta::FromSeconds(42);
    const std::string id = "id";
    const std::string content = "subtitle";
    const std::string settings;

    if (expect_cue) {
      FakeTextTrack* const text_track = text_tracks_[idx];
      EXPECT_CALL(*text_track, addWebVTTCue(start,
                                            start + duration,
                                            id,
                                            content,
                                            settings));
    }

    text_stream->SatisfyPendingRead(start, duration, id, content, settings);
    message_loop_.RunUntilIdle();
  }

  void SendCues(bool expect_cue) {
    for (size_t idx = 0; idx < text_track_streams_.size(); ++idx) {
      SendCue(idx, expect_cue);
    }
  }

  void OnDestroyTextTrack(unsigned idx) {
    text_tracks_[idx] = NULL;
  }

  void Play() {
    EXPECT_CALL(*this, OnPlay());
    text_renderer_->Play(base::Bind(&TextRendererTest::OnPlay,
                                    base::Unretained(this)));
    message_loop_.RunUntilIdle();
  }

  void Pause() {
    text_renderer_->Pause(base::Bind(&TextRendererTest::OnPause,
                                     base::Unretained(this)));
    message_loop_.RunUntilIdle();
  }

  void Flush() {
    EXPECT_CALL(*this, OnFlush());
    text_renderer_->Flush(base::Bind(&TextRendererTest::OnFlush,
                                     base::Unretained(this)));
  }

  void Stop() {
    text_renderer_->Stop(base::Bind(&TextRendererTest::OnStop,
                                    base::Unretained(this)));
    message_loop_.RunUntilIdle();
  }

  void ExpectRead(size_t idx) {
    FakeTextTrackStream* const stream = text_track_streams_[idx];
    EXPECT_CALL(*stream, OnRead());
  }

  MOCK_METHOD0(OnEnd, void());
  MOCK_METHOD0(OnStop, void());
  MOCK_METHOD0(OnPlay, void());
  MOCK_METHOD0(OnPause, void());
  MOCK_METHOD0(OnFlush, void());

  scoped_ptr<TextRenderer> text_renderer_;
  base::MessageLoop message_loop_;

  typedef ScopedVector<FakeTextTrackStream> TextTrackStreams;
  TextTrackStreams text_track_streams_;

  typedef std::vector<FakeTextTrack*> TextTracks;
  TextTracks text_tracks_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TextRendererTest);
};

TEST_F(TextRendererTest, CreateTextRendererNoInit) {
  text_renderer_.reset(
      new TextRenderer(message_loop_.message_loop_proxy(),
                       base::Bind(&TextRendererTest::OnAddTextTrack,
                                  base::Unretained(this))));
  text_renderer_.reset();
}

TEST_F(TextRendererTest, TestStop) {
  CreateTextRenderer();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTextTrackOnly_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", false);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTextTrackOnly_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "track 1", "", false);
  AddTextTrack(kTextSubtitles, "track 2", "", false);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayOnly) {
  CreateTextRenderer();
  Play();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackBeforePlay_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  AbortPendingReads();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackBeforePlay_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingReads();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackAfterPlay_OneTrackAfter) {
  CreateTextRenderer();
  Play();
  AddTextTrack(kTextSubtitles, "", "", true);
  AbortPendingReads();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackAfterPlay_TwoTracksAfter) {
  CreateTextRenderer();
  Play();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  AbortPendingReads();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackAfterPlay_OneTrackBeforeOneTrackAfter) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  Play();
  AddTextTrack(kTextSubtitles, "2", "", true);
  AbortPendingReads();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayAddCue_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  ExpectRead(0);
  SendCues(true);
  AbortPendingReads();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayAddCue_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  ExpectRead(0);
  ExpectRead(1);
  SendCues(true);
  AbortPendingReads();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosOnly_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosOnly_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCueEos_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  ExpectRead(0);
  SendCues(true);
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCueEos_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  ExpectRead(0);
  ExpectRead(1);
  SendCues(true);
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, StopPending_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  Stop();
  EXPECT_CALL(*this, OnStop());
  SendEosNotifications();
  text_renderer_.reset();
  text_track_streams_.clear();
}

TEST_F(TextRendererTest, StopPending_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Stop();
  EXPECT_CALL(*this, OnStop());
  SendEosNotifications();
  text_renderer_.reset();
  text_track_streams_.clear();
}

TEST_F(TextRendererTest, PlayPause_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  AbortPendingReads();
  EXPECT_CALL(*this, OnPause());
  Pause();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayPause_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingReads();
  EXPECT_CALL(*this, OnPause());
  Pause();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPausePending_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  Pause();
  EXPECT_CALL(*this, OnPause());
  SendEosNotifications();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPausePending_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  EXPECT_CALL(*this, OnPause());
  SendEosNotifications();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCuePausePending_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  Pause();
  EXPECT_CALL(*this, OnPause());
  SendCues(true);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCuePausePending_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  EXPECT_CALL(*this, OnPause());
  SendCues(true);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  EXPECT_CALL(*this, OnPause());
  Pause();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  EXPECT_CALL(*this, OnPause());
  Pause();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_SplitEos) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  SendEosNotification(0);
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  EXPECT_CALL(*this, OnPause());
  Pause();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosFlush_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  EXPECT_CALL(*this, OnPause());
  Pause();
  Flush();
  ExpectRead(0);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosFlush_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  EXPECT_CALL(*this, OnPause());
  Pause();
  Flush();
  ExpectRead(0);
  ExpectRead(1);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTextTrackOnlyRemove_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", false);
  EXPECT_TRUE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTextTrackOnlyRemove_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "track 1", "", false);
  AddTextTrack(kTextSubtitles, "track 2", "", false);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackBeforePlayRemove_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  AbortPendingReads();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackBeforePlayRemove_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingReads();
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackBeforePlayRemove_SeparateCancel) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingRead(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  AbortPendingRead(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackBeforePlayRemove_RemoveOneThenPlay) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", false);
  AddTextTrack(kTextSubtitles, "2", "", true);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  Play();
  AbortPendingRead(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackBeforePlayRemove_RemoveTwoThenPlay) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", false);
  AddTextTrack(kTextSubtitles, "2", "", false);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  Play();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackAfterPlayRemove_OneTrack) {
  CreateTextRenderer();
  Play();
  AddTextTrack(kTextSubtitles, "", "", true);
  AbortPendingReads();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackAfterPlayRemove_TwoTracks) {
  CreateTextRenderer();
  Play();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  AbortPendingReads();
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackAfterPlayRemove_SplitCancel) {
  CreateTextRenderer();
  Play();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  AbortPendingRead(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  AbortPendingRead(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddTrackAfterPlayRemove_SplitAdd) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  Play();
  AddTextTrack(kTextSubtitles, "2", "", true);
  AbortPendingRead(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  AbortPendingRead(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayAddCueRemove_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  ExpectRead(0);
  SendCues(true);
  AbortPendingReads();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayAddCueRemove_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  ExpectRead(0);
  ExpectRead(1);
  SendCues(true);
  AbortPendingRead(0);
  AbortPendingRead(1);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosOnlyRemove_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosOnlyRemove_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCueEosRemove_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  ExpectRead(0);
  SendCues(true);
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCueEosRemove_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  ExpectRead(0);
  ExpectRead(1);
  SendCues(true);
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, TestStopPendingRemove_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  Stop();
  EXPECT_CALL(*this, OnStop());
  SendEosNotifications();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  text_renderer_.reset();
  text_track_streams_.clear();
}

TEST_F(TextRendererTest, TestStopPendingRemove_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Stop();
  SendEosNotification(0);
  EXPECT_CALL(*this, OnStop());
  SendEosNotification(1);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  text_renderer_.reset();
  text_track_streams_.clear();
}

TEST_F(TextRendererTest, TestStopPendingRemove_RemoveThenSendEos) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Stop();
  SendEosNotification(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnStop());
  SendEosNotification(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  text_renderer_.reset();
  text_track_streams_.clear();
}

TEST_F(TextRendererTest, PlayPauseRemove_PauseThenRemove) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  AbortPendingReads();
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayPauseRemove_RemoveThanPause) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  AbortPendingReads();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  Pause();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayPause_PauseThenRemoveTwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingReads();
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayPauseRemove_RemoveThenPauseTwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingReads();
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayPauseRemove_SplitCancel) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingRead(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  AbortPendingRead(1);
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}


TEST_F(TextRendererTest, PlayPauseRemove_PauseLast) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingRead(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  AbortPendingRead(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  Pause();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPausePendingRemove_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  Pause();
  EXPECT_CALL(*this, OnPause());
  SendEosNotifications();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPausePendingRemove_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  SendEosNotification(0);
  EXPECT_CALL(*this, OnPause());
  SendEosNotification(1);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPausePendingRemove_SplitEos) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  SendEosNotification(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  SendEosNotification(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCuePausePendingRemove_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  Pause();
  EXPECT_CALL(*this, OnPause());
  SendCues(true);
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCuePausePendingRemove_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  SendCue(0, true);
  EXPECT_CALL(*this, OnPause());
  SendCue(1, true);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCuePausePendingRemove_SplitSendCue) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  SendCue(0, true);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  SendCue(1, true);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPauseRemove_PauseThenRemove) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPauseRemove_RemoveThenPause) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  Pause();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_PauseThenRemoveTwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  SendEosNotification(0);
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_RemovePauseRemove) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  SendEosNotification(0);
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_EosThenPause) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  SendEosNotification(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_PauseLast) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  SendEosNotification(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  Pause();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_EosPauseRemove) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  SendEosNotification(0);
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_EosRemovePause) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  SendEosNotification(0);
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_EosRemoveEosPause) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  SendEosNotification(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  EXPECT_CALL(*this, OnPause());
  Pause();
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosPause_EosRemoveEosRemovePause) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  SendEosNotification(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  Pause();
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosFlushRemove_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  EXPECT_CALL(*this, OnPause());
  Pause();
  Flush();
  ExpectRead(0);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  RemoveTextTrack(0);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosFlushRemove_TwoTracks) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  EXPECT_CALL(*this, OnPause());
  Pause();
  Flush();
  ExpectRead(0);
  ExpectRead(1);
  Play();
  SendEosNotification(0);
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayEosFlushRemove_EosRemove) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotifications();
  EXPECT_CALL(*this, OnPause());
  Pause();
  Flush();
  ExpectRead(0);
  ExpectRead(1);
  Play();
  SendEosNotification(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayShort_SendCueThenEos) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  SendCue(0, true);
  EXPECT_CALL(*this, OnPause());
  SendEosNotification(1);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayShort_EosThenSendCue) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  SendEosNotification(0);
  EXPECT_CALL(*this, OnPause());
  SendCue(1, true);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayShortRemove_SendEosRemove) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  SendCue(0, true);
  EXPECT_CALL(*this, OnPause());
  SendEosNotification(1);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayShortRemove_SendRemoveEos) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  SendCue(0, true);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnPause());
  SendEosNotification(1);
  RemoveTextTrack(1);
  EXPECT_FALSE(text_renderer_->HasTracks());
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCuePausePendingCancel_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  Pause();
  EXPECT_CALL(*this, OnPause());
  AbortPendingRead(0);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCuePausePendingCancel_SendThenCancel) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  SendCue(0, true);
  EXPECT_CALL(*this, OnPause());
  AbortPendingRead(1);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCuePausePendingCancel_CancelThenSend) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  AbortPendingRead(0);
  EXPECT_CALL(*this, OnPause());
  SendCue(1, true);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, PlayCueStopPendingCancel_OneTrack) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  Pause();
  Stop();
  EXPECT_CALL(*this, OnStop());
  AbortPendingRead(0);
  text_renderer_.reset();
  text_track_streams_.clear();
}

TEST_F(TextRendererTest, PlayCueStopPendingCancel_SendThenCancel) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  Stop();
  SendCue(0, false);
  EXPECT_CALL(*this, OnStop());
  AbortPendingRead(1);
  text_renderer_.reset();
  text_track_streams_.clear();
}

TEST_F(TextRendererTest, PlayCueStopPendingCancel_CancelThenSend) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  Pause();
  Stop();
  AbortPendingRead(0);
  EXPECT_CALL(*this, OnStop());
  SendCue(1, false);
  text_renderer_.reset();
  text_track_streams_.clear();
}

TEST_F(TextRendererTest, AddRemoveAdd) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  AbortPendingRead(0);
  RemoveTextTrack(0);
  EXPECT_CALL(*this, OnPause());
  Pause();
  AddTextTrack(kTextSubtitles, "", "", true);
  Play();
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddRemoveEos) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingRead(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  EXPECT_CALL(*this, OnEnd());
  SendEosNotification(1);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddRemovePause) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingRead(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  Pause();
  EXPECT_CALL(*this, OnPause());
  SendEosNotification(1);
  DestroyTextRenderer();
}

TEST_F(TextRendererTest, AddRemovePauseStop) {
  CreateTextRenderer();
  AddTextTrack(kTextSubtitles, "1", "", true);
  AddTextTrack(kTextSubtitles, "2", "", true);
  Play();
  AbortPendingRead(0);
  RemoveTextTrack(0);
  EXPECT_TRUE(text_renderer_->HasTracks());
  Pause();
  Stop();
  EXPECT_CALL(*this, OnStop());
  SendEosNotification(1);
  text_renderer_.reset();
  text_track_streams_.clear();
}

}  // namespace media
