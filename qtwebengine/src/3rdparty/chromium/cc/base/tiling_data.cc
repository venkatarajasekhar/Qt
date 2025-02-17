// Copyright 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/base/tiling_data.h"

#include <algorithm>

#include "ui/gfx/rect.h"
#include "ui/gfx/vector2d.h"

namespace cc {

static int ComputeNumTiles(int max_texture_size,
                           int total_size,
                           int border_texels) {
  if (max_texture_size - 2 * border_texels <= 0)
    return total_size > 0 && max_texture_size >= total_size ? 1 : 0;

  int num_tiles = std::max(1,
                           1 + (total_size - 1 - 2 * border_texels) /
                           (max_texture_size - 2 * border_texels));
  return total_size > 0 ? num_tiles : 0;
}

TilingData::TilingData()
    : border_texels_(0) {
  RecomputeNumTiles();
}

TilingData::TilingData(const gfx::Size& max_texture_size,
                       const gfx::Rect& tiling_rect,
                       bool has_border_texels)
    : max_texture_size_(max_texture_size),
      tiling_rect_(tiling_rect),
      border_texels_(has_border_texels ? 1 : 0) {
  RecomputeNumTiles();
}

TilingData::TilingData(const gfx::Size& max_texture_size,
                       const gfx::Rect& tiling_rect,
                       int border_texels)
    : max_texture_size_(max_texture_size),
      tiling_rect_(tiling_rect),
      border_texels_(border_texels) {
  RecomputeNumTiles();
}

void TilingData::SetTilingRect(const gfx::Rect& tiling_rect) {
  tiling_rect_ = tiling_rect;
  RecomputeNumTiles();
}

void TilingData::SetMaxTextureSize(const gfx::Size& max_texture_size) {
  max_texture_size_ = max_texture_size;
  RecomputeNumTiles();
}

void TilingData::SetHasBorderTexels(bool has_border_texels) {
  border_texels_ = has_border_texels ? 1 : 0;
  RecomputeNumTiles();
}

void TilingData::SetBorderTexels(int border_texels) {
  border_texels_ = border_texels;
  RecomputeNumTiles();
}

int TilingData::TileXIndexFromSrcCoord(int src_position) const {
  if (num_tiles_x_ <= 1)
    return 0;

  src_position -= tiling_rect_.x();

  DCHECK_GT(max_texture_size_.width() - 2 * border_texels_, 0);
  int x = (src_position - border_texels_) /
      (max_texture_size_.width() - 2 * border_texels_);
  return std::min(std::max(x, 0), num_tiles_x_ - 1);
}

int TilingData::TileYIndexFromSrcCoord(int src_position) const {
  if (num_tiles_y_ <= 1)
    return 0;

  src_position -= tiling_rect_.y();

  DCHECK_GT(max_texture_size_.height() - 2 * border_texels_, 0);
  int y = (src_position - border_texels_) /
      (max_texture_size_.height() - 2 * border_texels_);
  return std::min(std::max(y, 0), num_tiles_y_ - 1);
}

int TilingData::FirstBorderTileXIndexFromSrcCoord(int src_position) const {
  if (num_tiles_x_ <= 1)
    return 0;

  src_position -= tiling_rect_.x();

  DCHECK_GT(max_texture_size_.width() - 2 * border_texels_, 0);
  int inner_tile_size = max_texture_size_.width() - 2 * border_texels_;
  int x = (src_position - 2 * border_texels_) / inner_tile_size;
  return std::min(std::max(x, 0), num_tiles_x_ - 1);
}

int TilingData::FirstBorderTileYIndexFromSrcCoord(int src_position) const {
  if (num_tiles_y_ <= 1)
    return 0;

  src_position -= tiling_rect_.y();

  DCHECK_GT(max_texture_size_.height() - 2 * border_texels_, 0);
  int inner_tile_size = max_texture_size_.height() - 2 * border_texels_;
  int y = (src_position - 2 * border_texels_) / inner_tile_size;
  return std::min(std::max(y, 0), num_tiles_y_ - 1);
}

int TilingData::LastBorderTileXIndexFromSrcCoord(int src_position) const {
  if (num_tiles_x_ <= 1)
    return 0;

  src_position -= tiling_rect_.x();

  DCHECK_GT(max_texture_size_.width() - 2 * border_texels_, 0);
  int inner_tile_size = max_texture_size_.width() - 2 * border_texels_;
  int x = src_position / inner_tile_size;
  return std::min(std::max(x, 0), num_tiles_x_ - 1);
}

int TilingData::LastBorderTileYIndexFromSrcCoord(int src_position) const {
  if (num_tiles_y_ <= 1)
    return 0;

  src_position -= tiling_rect_.y();

  DCHECK_GT(max_texture_size_.height() - 2 * border_texels_, 0);
  int inner_tile_size = max_texture_size_.height() - 2 * border_texels_;
  int y = src_position / inner_tile_size;
  return std::min(std::max(y, 0), num_tiles_y_ - 1);
}

gfx::Rect TilingData::ExpandRectToTileBoundsWithBorders(
    const gfx::Rect& rect) const {
  if (!rect.Intersects(tiling_rect_) || has_empty_bounds())
    return gfx::Rect();
  int index_x = FirstBorderTileXIndexFromSrcCoord(rect.x());
  int index_y = FirstBorderTileYIndexFromSrcCoord(rect.y());
  int index_right = LastBorderTileXIndexFromSrcCoord(rect.right() - 1);
  int index_bottom = LastBorderTileYIndexFromSrcCoord(rect.bottom() - 1);

  gfx::Rect rect_top_left(TileBoundsWithBorder(index_x, index_y));
  gfx::Rect rect_bottom_right(TileBoundsWithBorder(index_right, index_bottom));

  return gfx::UnionRects(rect_top_left, rect_bottom_right);
}

gfx::Rect TilingData::ExpandRectToTileBounds(const gfx::Rect& rect) const {
  if (!rect.Intersects(tiling_rect_) || has_empty_bounds())
    return gfx::Rect();
  int index_x = FirstBorderTileXIndexFromSrcCoord(rect.x());
  int index_y = FirstBorderTileYIndexFromSrcCoord(rect.y());
  int index_right = LastBorderTileXIndexFromSrcCoord(rect.right() - 1);
  int index_bottom = LastBorderTileYIndexFromSrcCoord(rect.bottom() - 1);

  gfx::Rect rect_top_left(TileBounds(index_x, index_y));
  gfx::Rect rect_bottom_right(TileBounds(index_right, index_bottom));

  return gfx::UnionRects(rect_top_left, rect_bottom_right);
}

gfx::Rect TilingData::TileBounds(int i, int j) const {
  AssertTile(i, j);
  int max_texture_size_x = max_texture_size_.width() - 2 * border_texels_;
  int max_texture_size_y = max_texture_size_.height() - 2 * border_texels_;

  int lo_x = tiling_rect_.x() + max_texture_size_x * i;
  if (i != 0)
    lo_x += border_texels_;

  int lo_y = tiling_rect_.y() + max_texture_size_y * j;
  if (j != 0)
    lo_y += border_texels_;

  int hi_x = tiling_rect_.x() + max_texture_size_x * (i + 1) + border_texels_;
  if (i + 1 == num_tiles_x_)
    hi_x += border_texels_;

  int hi_y = tiling_rect_.y() + max_texture_size_y * (j + 1) + border_texels_;
  if (j + 1 == num_tiles_y_)
    hi_y += border_texels_;

  hi_x = std::min(hi_x, tiling_rect_.right());
  hi_y = std::min(hi_y, tiling_rect_.bottom());

  int x = lo_x;
  int y = lo_y;
  int width = hi_x - lo_x;
  int height = hi_y - lo_y;
  DCHECK_GE(x, tiling_rect_.x());
  DCHECK_GE(y, tiling_rect_.y());
  DCHECK_GE(width, 0);
  DCHECK_GE(height, 0);
  DCHECK_LE(x, tiling_rect_.right());
  DCHECK_LE(y, tiling_rect_.bottom());
  return gfx::Rect(x, y, width, height);
}

gfx::Rect TilingData::TileBoundsWithBorder(int i, int j) const {
  AssertTile(i, j);
  int max_texture_size_x = max_texture_size_.width() - 2 * border_texels_;
  int max_texture_size_y = max_texture_size_.height() - 2 * border_texels_;

  int lo_x = tiling_rect_.x() + max_texture_size_x * i;
  int lo_y = tiling_rect_.y() + max_texture_size_y * j;

  int hi_x = lo_x + max_texture_size_x + 2 * border_texels_;
  int hi_y = lo_y + max_texture_size_y + 2 * border_texels_;

  hi_x = std::min(hi_x, tiling_rect_.right());
  hi_y = std::min(hi_y, tiling_rect_.bottom());

  int x = lo_x;
  int y = lo_y;
  int width = hi_x - lo_x;
  int height = hi_y - lo_y;
  DCHECK_GE(x, tiling_rect_.x());
  DCHECK_GE(y, tiling_rect_.y());
  DCHECK_GE(width, 0);
  DCHECK_GE(height, 0);
  DCHECK_LE(x, tiling_rect_.right());
  DCHECK_LE(y, tiling_rect_.bottom());
  return gfx::Rect(x, y, width, height);
}

int TilingData::TilePositionX(int x_index) const {
  DCHECK_GE(x_index, 0);
  DCHECK_LT(x_index, num_tiles_x_);

  int pos = (max_texture_size_.width() - 2 * border_texels_) * x_index;
  if (x_index != 0)
    pos += border_texels_;

  pos += tiling_rect_.x();

  return pos;
}

int TilingData::TilePositionY(int y_index) const {
  DCHECK_GE(y_index, 0);
  DCHECK_LT(y_index, num_tiles_y_);

  int pos = (max_texture_size_.height() - 2 * border_texels_) * y_index;
  if (y_index != 0)
    pos += border_texels_;

  pos += tiling_rect_.y();

  return pos;
}

int TilingData::TileSizeX(int x_index) const {
  DCHECK_GE(x_index, 0);
  DCHECK_LT(x_index, num_tiles_x_);

  if (!x_index && num_tiles_x_ == 1)
    return tiling_rect_.width();
  if (!x_index && num_tiles_x_ > 1)
    return max_texture_size_.width() - border_texels_;
  if (x_index < num_tiles_x_ - 1)
    return max_texture_size_.width() - 2 * border_texels_;
  if (x_index == num_tiles_x_ - 1)
    return tiling_rect_.right() - TilePositionX(x_index);

  NOTREACHED();
  return 0;
}

int TilingData::TileSizeY(int y_index) const {
  DCHECK_GE(y_index, 0);
  DCHECK_LT(y_index, num_tiles_y_);

  if (!y_index && num_tiles_y_ == 1)
    return tiling_rect_.height();
  if (!y_index && num_tiles_y_ > 1)
    return max_texture_size_.height() - border_texels_;
  if (y_index < num_tiles_y_ - 1)
    return max_texture_size_.height() - 2 * border_texels_;
  if (y_index == num_tiles_y_ - 1)
    return tiling_rect_.bottom() - TilePositionY(y_index);

  NOTREACHED();
  return 0;
}

gfx::Vector2d TilingData::TextureOffset(int x_index, int y_index) const {
  int left = (!x_index || num_tiles_x_ == 1) ? 0 : border_texels_;
  int top = (!y_index || num_tiles_y_ == 1) ? 0 : border_texels_;

  return gfx::Vector2d(left, top);
}

void TilingData::RecomputeNumTiles() {
  num_tiles_x_ = ComputeNumTiles(
      max_texture_size_.width(), tiling_rect_.width(), border_texels_);
  num_tiles_y_ = ComputeNumTiles(
      max_texture_size_.height(), tiling_rect_.height(), border_texels_);
}

TilingData::BaseIterator::BaseIterator(const TilingData* tiling_data)
    : tiling_data_(tiling_data),
      index_x_(-1),
      index_y_(-1) {
}

TilingData::Iterator::Iterator() : BaseIterator(NULL) { done(); }

TilingData::Iterator::Iterator(const TilingData* tiling_data,
                               const gfx::Rect& tiling_rect,
                               bool include_borders)
    : BaseIterator(tiling_data), left_(-1), right_(-1), bottom_(-1) {
  if (tiling_data_->num_tiles_x() <= 0 || tiling_data_->num_tiles_y() <= 0) {
    done();
    return;
  }

  gfx::Rect rect(tiling_rect);
  rect.Intersect(tiling_data_->tiling_rect());

  gfx::Rect top_left_tile;
  if (include_borders) {
    index_x_ = tiling_data_->FirstBorderTileXIndexFromSrcCoord(rect.x());
    index_y_ = tiling_data_->FirstBorderTileYIndexFromSrcCoord(rect.y());
    right_ = tiling_data_->LastBorderTileXIndexFromSrcCoord(rect.right() - 1);
    bottom_ = tiling_data_->LastBorderTileYIndexFromSrcCoord(rect.bottom() - 1);
    top_left_tile = tiling_data_->TileBoundsWithBorder(index_x_, index_y_);
  } else {
    index_x_ = tiling_data_->TileXIndexFromSrcCoord(rect.x());
    index_y_ = tiling_data_->TileYIndexFromSrcCoord(rect.y());
    right_ = tiling_data_->TileXIndexFromSrcCoord(rect.right() - 1);
    bottom_ = tiling_data_->TileYIndexFromSrcCoord(rect.bottom() - 1);
    top_left_tile = tiling_data_->TileBounds(index_x_, index_y_);
  }
  left_ = index_x_;

  // Index functions always return valid indices, so explicitly check
  // for non-intersecting rects.
  if (!top_left_tile.Intersects(rect))
    done();
}

TilingData::Iterator& TilingData::Iterator::operator++() {
  if (!*this)
    return *this;

  index_x_++;
  if (index_x_ > right_) {
    index_x_ = left_;
    index_y_++;
    if (index_y_ > bottom_)
      done();
  }

  return *this;
}

TilingData::DifferenceIterator::DifferenceIterator(
    const TilingData* tiling_data,
    const gfx::Rect& consider_rect,
    const gfx::Rect& ignore_rect)
    : BaseIterator(tiling_data),
      consider_left_(-1),
      consider_top_(-1),
      consider_right_(-1),
      consider_bottom_(-1),
      ignore_left_(-1),
      ignore_top_(-1),
      ignore_right_(-1),
      ignore_bottom_(-1) {
  if (tiling_data_->num_tiles_x() <= 0 || tiling_data_->num_tiles_y() <= 0) {
    done();
    return;
  }

  gfx::Rect consider(consider_rect);
  gfx::Rect ignore(ignore_rect);
  consider.Intersect(tiling_data_->tiling_rect());
  ignore.Intersect(tiling_data_->tiling_rect());
  if (consider.IsEmpty()) {
    done();
    return;
  }

  consider_left_ =
      tiling_data_->FirstBorderTileXIndexFromSrcCoord(consider.x());
  consider_top_ =
      tiling_data_->FirstBorderTileYIndexFromSrcCoord(consider.y());
  consider_right_ =
      tiling_data_->LastBorderTileXIndexFromSrcCoord(consider.right() - 1);
  consider_bottom_ =
      tiling_data_->LastBorderTileYIndexFromSrcCoord(consider.bottom() - 1);

  if (!ignore.IsEmpty()) {
    ignore_left_ =
        tiling_data_->FirstBorderTileXIndexFromSrcCoord(ignore.x());
    ignore_top_ =
        tiling_data_->FirstBorderTileYIndexFromSrcCoord(ignore.y());
    ignore_right_ =
        tiling_data_->LastBorderTileXIndexFromSrcCoord(ignore.right() - 1);
    ignore_bottom_ =
        tiling_data_->LastBorderTileYIndexFromSrcCoord(ignore.bottom() - 1);

    // Clamp ignore indices to consider indices.
    ignore_left_ = std::max(ignore_left_, consider_left_);
    ignore_top_ = std::max(ignore_top_, consider_top_);
    ignore_right_ = std::min(ignore_right_, consider_right_);
    ignore_bottom_ = std::min(ignore_bottom_, consider_bottom_);
  }

  if (ignore_left_ == consider_left_ && ignore_right_ == consider_right_ &&
      ignore_top_ == consider_top_ && ignore_bottom_ == consider_bottom_) {
    done();
    return;
  }

  index_x_ = consider_left_;
  index_y_ = consider_top_;

  if (in_ignore_rect())
    ++(*this);
}

TilingData::DifferenceIterator& TilingData::DifferenceIterator::operator++() {
  if (!*this)
    return *this;

  index_x_++;
  if (in_ignore_rect())
    index_x_ = ignore_right_ + 1;

  if (index_x_ > consider_right_) {
    index_x_ = consider_left_;
    index_y_++;

    if (in_ignore_rect()) {
      index_x_ = ignore_right_ + 1;
      // If the ignore rect spans the whole consider rect horizontally, then
      // ignore_right + 1 will be out of bounds.
      if (in_ignore_rect() || index_x_ > consider_right_) {
        index_y_ = ignore_bottom_ + 1;
        index_x_ = consider_left_;
      }
    }

    if (index_y_ > consider_bottom_)
      done();
  }

  return *this;
}

TilingData::SpiralDifferenceIterator::SpiralDifferenceIterator()
    : BaseIterator(NULL) {
  done();
}

TilingData::SpiralDifferenceIterator::SpiralDifferenceIterator(
    const TilingData* tiling_data,
    const gfx::Rect& consider_rect,
    const gfx::Rect& ignore_rect,
    const gfx::Rect& center_rect)
    : BaseIterator(tiling_data),
      consider_left_(-1),
      consider_top_(-1),
      consider_right_(-1),
      consider_bottom_(-1),
      ignore_left_(-1),
      ignore_top_(-1),
      ignore_right_(-1),
      ignore_bottom_(-1),
      direction_(RIGHT),
      delta_x_(1),
      delta_y_(0),
      current_step_(0),
      horizontal_step_count_(0),
      vertical_step_count_(0) {
  if (tiling_data_->num_tiles_x() <= 0 || tiling_data_->num_tiles_y() <= 0) {
    done();
    return;
  }

  gfx::Rect consider(consider_rect);
  gfx::Rect ignore(ignore_rect);
  gfx::Rect center(center_rect);
  consider.Intersect(tiling_data_->tiling_rect());
  ignore.Intersect(tiling_data_->tiling_rect());
  if (consider.IsEmpty()) {
    done();
    return;
  }

  consider_left_ =
      tiling_data_->FirstBorderTileXIndexFromSrcCoord(consider.x());
  consider_top_ = tiling_data_->FirstBorderTileYIndexFromSrcCoord(consider.y());
  consider_right_ =
      tiling_data_->LastBorderTileXIndexFromSrcCoord(consider.right() - 1);
  consider_bottom_ =
      tiling_data_->LastBorderTileYIndexFromSrcCoord(consider.bottom() - 1);

  if (!ignore.IsEmpty()) {
    ignore_left_ = tiling_data_->FirstBorderTileXIndexFromSrcCoord(ignore.x());
    ignore_top_ = tiling_data_->FirstBorderTileYIndexFromSrcCoord(ignore.y());
    ignore_right_ =
        tiling_data_->LastBorderTileXIndexFromSrcCoord(ignore.right() - 1);
    ignore_bottom_ =
        tiling_data_->LastBorderTileYIndexFromSrcCoord(ignore.bottom() - 1);

    // Clamp ignore indices to consider indices.
    ignore_left_ = std::max(ignore_left_, consider_left_);
    ignore_top_ = std::max(ignore_top_, consider_top_);
    ignore_right_ = std::min(ignore_right_, consider_right_);
    ignore_bottom_ = std::min(ignore_bottom_, consider_bottom_);
  }

  if (ignore_left_ == consider_left_ && ignore_right_ == consider_right_ &&
      ignore_top_ == consider_top_ && ignore_bottom_ == consider_bottom_) {
    done();
    return;
  }

  // Determine around left, such that it is between -1 and num_tiles_x.
  int around_left = 0;
  if (center.x() < tiling_data->tiling_rect().x() || center.IsEmpty())
    around_left = -1;
  else if (center.x() > tiling_data->tiling_rect().right())
    around_left = tiling_data->num_tiles_x();
  else
    around_left = tiling_data->FirstBorderTileXIndexFromSrcCoord(center.x());

  // Determine around top, such that it is between -1 and num_tiles_y.
  int around_top = 0;
  if (center.y() < tiling_data->tiling_rect().y() || center.IsEmpty())
    around_top = -1;
  else if (center.y() > tiling_data->tiling_rect().bottom())
    around_top = tiling_data->num_tiles_y();
  else
    around_top = tiling_data->FirstBorderTileYIndexFromSrcCoord(center.y());

  // Determine around right, such that it is between -1 and num_tiles_x.
  int right_src_coord = center.right() - 1;
  int around_right = 0;
  if (right_src_coord < tiling_data->tiling_rect().x() || center.IsEmpty()) {
    around_right = -1;
  } else if (right_src_coord > tiling_data->tiling_rect().right()) {
    around_right = tiling_data->num_tiles_x();
  } else {
    around_right =
        tiling_data->LastBorderTileXIndexFromSrcCoord(right_src_coord);
  }

  // Determine around bottom, such that it is between -1 and num_tiles_y.
  int bottom_src_coord = center.bottom() - 1;
  int around_bottom = 0;
  if (bottom_src_coord < tiling_data->tiling_rect().y() || center.IsEmpty()) {
    around_bottom = -1;
  } else if (bottom_src_coord > tiling_data->tiling_rect().bottom()) {
    around_bottom = tiling_data->num_tiles_y();
  } else {
    around_bottom =
        tiling_data->LastBorderTileYIndexFromSrcCoord(bottom_src_coord);
  }

  vertical_step_count_ = around_bottom - around_top + 1;
  horizontal_step_count_ = around_right - around_left + 1;
  current_step_ = horizontal_step_count_ - 1;

  index_x_ = around_right;
  index_y_ = around_bottom;

  // The current index is the bottom right of the around rect, which is also
  // ignored. So we have to advance.
  ++(*this);
}

TilingData::SpiralDifferenceIterator& TilingData::SpiralDifferenceIterator::
operator++() {
  int cannot_hit_consider_count = 0;
  while (cannot_hit_consider_count < 4) {
    if (needs_direction_switch())
      switch_direction();

    index_x_ += delta_x_;
    index_y_ += delta_y_;
    ++current_step_;

    if (in_consider_rect()) {
      cannot_hit_consider_count = 0;

      if (!in_ignore_rect())
        break;

      // Steps needed to reach the very edge of the ignore rect, while remaining
      // inside (so that the continue would take us outside).
      int steps_to_edge = 0;
      switch (direction_) {
        case UP:
          steps_to_edge = index_y_ - ignore_top_;
          break;
        case LEFT:
          steps_to_edge = index_x_ - ignore_left_;
          break;
        case DOWN:
          steps_to_edge = ignore_bottom_ - index_y_;
          break;
        case RIGHT:
          steps_to_edge = ignore_right_ - index_x_;
          break;
      }

      // We need to switch directions in |max_steps|.
      int max_steps = current_step_count() - current_step_;

      int steps_to_take = std::min(steps_to_edge, max_steps);
      DCHECK_GE(steps_to_take, 0);

      index_x_ += steps_to_take * delta_x_;
      index_y_ += steps_to_take * delta_y_;
      current_step_ += steps_to_take;
    } else {
      int max_steps = current_step_count() - current_step_;
      int steps_to_take = max_steps;
      bool can_hit_consider_rect = false;
      switch (direction_) {
        case UP:
          if (valid_column() && consider_bottom_ < index_y_)
            steps_to_take = index_y_ - consider_bottom_ - 1;
          can_hit_consider_rect |= consider_right_ >= index_x_;
          break;
        case LEFT:
          if (valid_row() && consider_right_ < index_x_)
            steps_to_take = index_x_ - consider_right_ - 1;
          can_hit_consider_rect |= consider_top_ <= index_y_;
          break;
        case DOWN:
          if (valid_column() && consider_top_ > index_y_)
            steps_to_take = consider_top_ - index_y_ - 1;
          can_hit_consider_rect |= consider_left_ <= index_x_;
          break;
        case RIGHT:
          if (valid_row() && consider_left_ > index_x_)
            steps_to_take = consider_left_ - index_x_ - 1;
          can_hit_consider_rect |= consider_bottom_ >= index_y_;
          break;
      }
      steps_to_take = std::min(steps_to_take, max_steps);
      DCHECK_GE(steps_to_take, 0);

      index_x_ += steps_to_take * delta_x_;
      index_y_ += steps_to_take * delta_y_;
      current_step_ += steps_to_take;

      if (can_hit_consider_rect)
        cannot_hit_consider_count = 0;
      else
        ++cannot_hit_consider_count;
    }
  }

  if (cannot_hit_consider_count >= 4)
    done();
  return *this;
}

bool TilingData::SpiralDifferenceIterator::needs_direction_switch() const {
  return current_step_ >= current_step_count();
}

void TilingData::SpiralDifferenceIterator::switch_direction() {
  int new_delta_x_ = delta_y_;
  delta_y_ = -delta_x_;
  delta_x_ = new_delta_x_;

  current_step_ = 0;
  direction_ = static_cast<Direction>((direction_ + 1) % 4);

  if (direction_ == RIGHT || direction_ == LEFT) {
    ++vertical_step_count_;
    ++horizontal_step_count_;
  }
}

}  // namespace cc
