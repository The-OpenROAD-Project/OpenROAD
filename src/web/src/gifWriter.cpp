// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Gui's GIF recording.  The backend renders each frame to RGBA pixels and
// this file writes them, so nothing here is Qt.
//
// This is the one translation unit in :core that completes GifWriter, which
// is why GIF's constructor and destructor are defined here rather than left
// implicit in the header.

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"
#include "web/core.h"

// Defines GifBegin/GifWriteFrame/GifEnd with external linkage behind only a
// per-TU include guard, so exactly one translation unit per link may include
// it this way.  This is that unit: gui.cpp no longer does, and web_gif.cpp
// includes it inside an anonymous namespace to keep its copy internal.
#include "third-party/gif-h/gif.h"

namespace web {

GIF::GIF() = default;
GIF::~GIF() = default;

int Gui::gifStart(const std::string& filename)
{
  if (!hasUI()) {
    logger_->error(utl::WEB, 87, "Cannot generate GIF without GUI enabled");
  }

  if (filename.empty()) {
    logger_->error(utl::WEB, 103, "Filename is required to save a GIF.");
  }

  auto gif = std::make_unique<GIF>();
  gif->filename = filename;
  gifs_.emplace_back(std::move(gif));
  return gifs_.size() - 1;
}

void Gui::gifAddFrame(std::optional<int> key,
                      const odb::Rect& region,
                      int width_px,
                      double dbu_per_pixel,
                      std::optional<int> delay)
{
  if (!hasUI()) {
    return;
  }
  if (!key.has_value()) {
    key = gifs_.size() - 1;
  }
  if (*key < 0 || *key >= gifs_.size() || gifs_[*key] == nullptr) {
    logger_->warn(utl::WEB, 89, "GIF not active");
    return;
  }

  if (db_ == nullptr) {
    logger_->error(utl::WEB, 88, "No design loaded.");
  }

  auto& gif = gifs_[*key];

  odb::Rect save_region = region;
  const bool use_die_area = region.dx() == 0 || region.dy() == 0;
  if (activeBackend()->isOffscreen() && use_die_area) {
    // Onscreen the viewport's own extents are what the user is looking at;
    // offscreen they are not meaningful, so fall back to the die area.
    auto* chip = db_->getChip();
    if (chip == nullptr) {
      logger_->error(utl::WEB, 101, "No design loaded.");
    }

    auto* block = chip->getBlock();
    if (block == nullptr) {
      logger_->error(utl::WEB, 102, "No design loaded.");
    }

    // The same rect Gui::saveImage falls back to, so a GIF frame and a
    // save_image of one state cover the same area.
    save_region = block->getBBox()->getBox();
    const odb::Rect die = block->getDieArea();
    if (die.area() > 0) {
      save_region.merge(die);
    }
    const double bloat_by = 0.05;  // 5%
    const int bloat = std::min(save_region.dx(), save_region.dy()) * bloat_by;

    save_region.bloat(bloat, save_region);
  }

  // Frames after the first are scaled to the size the first one set, since a
  // gif has one canvas for its whole length.
  std::optional<std::pair<int, int>> scale_to;
  if (gif->writer != nullptr) {
    scale_to = std::make_pair(gif->width, gif->height);
  }
  RenderedImage img = activeBackend()->renderImage(
      save_region, width_px, dbu_per_pixel, scale_to);

  // A backend that cannot draw hands back an empty image, and one row short is
  // as unusable as none at all.  Either way there is no frame to write, and
  // taking the dimensions on trust would open a zero-sized gif or read off the
  // end of the pixels below.
  if (img.width <= 0 || img.height <= 0
      || img.rgba.size() < static_cast<size_t>(img.width) * img.height * 4) {
    logger_->warn(utl::WEB,
                  109,
                  "Backend rendered no image; frame not added to {}.",
                  gif->filename);
    return;
  }

  if (gif->writer == nullptr) {
    // Only adopt the writer once the file is open.  GifBegin returns false
    // when it cannot be created -- a bad directory, no permission, a full
    // disk -- and leaves a null FILE* behind, which every later
    // GifWriteFrame would then write nowhere.
    auto writer = std::make_unique<GifWriter>();
    if (!GifBegin(writer.get(),
                  gif->filename.c_str(),
                  img.width,
                  img.height,
                  delay.value_or(kDefaultGifDelay))) {
      logger_->error(
          utl::WEB, 110, "Unable to open {} to write a GIF.", gif->filename);
    }
    gif->width = img.width;
    gif->height = img.height;
    gif->writer = std::move(writer);
  }

  // The canvas is the first frame's size, which is what the backend was asked
  // to scale to, so the usual case is pixels that already fit and can be handed
  // straight to the writer.  Keeping the aspect ratio can leave a frame short
  // in one dimension; that one is copied into the top-left corner of a
  // transparent canvas.
  std::vector<uint8_t> frame;
  if (img.width == gif->width && img.height == gif->height) {
    frame = std::move(img.rgba);
  } else {
    frame.assign(static_cast<size_t>(gif->width) * gif->height * 4, 0);
    const int copy_width = std::min(img.width, gif->width);
    const int copy_height = std::min(img.height, gif->height);
    for (int y = 0; y < copy_height; y++) {
      const auto* src = &img.rgba[static_cast<size_t>(y) * img.width * 4];
      auto* dst = &frame[static_cast<size_t>(y) * gif->width * 4];
      std::copy_n(src, static_cast<size_t>(copy_width) * 4, dst);
    }
  }

  GifWriteFrame(gif->writer.get(),
                frame.data(),
                gif->width,
                gif->height,
                delay.value_or(kDefaultGifDelay));
}

void Gui::gifEnd(std::optional<int> key)
{
  if (!key.has_value()) {
    key = gifs_.size() - 1;
  }
  if (*key < 0 || *key >= gifs_.size() || gifs_[*key] == nullptr) {
    logger_->warn(utl::WEB, 91, "GIF not active");
    return;
  }

  auto& gif = gifs_[*key];
  if (gif->writer == nullptr) {
    logger_->warn(utl::WEB,
                  107,
                  "Nothing to save to {}. No frames added to gif.",
                  gif->filename);
    gif = nullptr;
    return;
  }

  GifEnd(gif->writer.get());
  gifs_[*key] = nullptr;
}

}  // namespace web
