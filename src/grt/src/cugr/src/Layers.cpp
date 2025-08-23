#include "Layers.h"

namespace grt {

MetalLayer::MetalLayer(odb::dbTechLayer* tech_layer,
                       odb::dbTrackGrid* track_grid)
{
  name_ = tech_layer->getName();
  index_ = tech_layer->getRoutingLevel() - 1;
  direction_
      = tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL ? H : V;
  width_ = static_cast<int>(std::round(tech_layer->getWidth()));
  min_width_ = static_cast<int>(std::round(tech_layer->getMinWidth()));

  track_grid->getAverageTrackSpacing(pitch_, first_track_loc_, num_tracks_);
  last_track_loc_ = first_track_loc_ + pitch_ * (num_tracks_ - 1);

  // Design rules not parsed thoroughly
  // Min area
  min_area_ = static_cast<int>(std::round(tech_layer->getArea()));
  min_length_ = std::max(min_area_ / width_ - width_, 0);

  // TODO: Handle parallel run spacing
  // // Parallel run spacing
  // auto spacingTable = tech_layer->getTechLayerSpacingTablePrlRules();
  // if (spacingTable.empty()) {
  //   printf("No spacing table for layer %s\n", name_.c_str());
  // } else {
  //   for (const auto& parallel : spacingTable) {
  //     parallel->getTechLayerSpacingTablePrlRule()->getTable(
  //         parallel_width_, parallel_length_, parallel_spacing_, _within_tbl);
  //   }
  //   for (unsigned tableIdx = 0; tableIdx < numSpacingTable; tableIdx++) {
  //     if (!layer_->spacingTable(tableIdx)->isParallel()) {
  //       continue;
  //     }
  //     const lefiParallel* parallel =
  //     layer_->spacingTable(tableIdx)->parallel(); const int numLength =
  //     parallel->numLength(); if (numLength > 0) {
  //       parallel_length_.resize(numLength);
  //       for (unsigned l = 0; l < numLength; l++) {
  //         parallel_length_[l]
  //             = static_cast<int>(std::round(parallel->length(l)));
  //       }
  //     }
  //     const int numWidth = parallel->numWidth();
  //     if (numWidth > 0) {
  //       parallel_width_.resize(numWidth);
  //       parallel_spacing_.resize(numWidth);
  //       for (unsigned w = 0; w < numWidth; w++) {
  //         parallel_width_[w]
  //             = static_cast<int>(std::round(parallel->width(w)));
  //         parallel_spacing_[w].resize(std::max(1, numLength), 0);
  //         for (int l = 0; l < numLength; l++) {
  //           parallel_spacing_[w][l] = static_cast<int>(
  //               std::round(parallel->widthSpacing(w, l)));
  //         }
  //       }
  //     }
  //   }
  // }
  // default_spacing_ = getParallelSpacing(width_);

  // TODO: Handle end-of-line spacing
  // End-of-line spacing
  // if (!layer_->hasSpacingNumber()) {
  //   printf("No spacing rules for layer %s\n", name_.c_str());
  // } else {
  //   const int numSpace = layer_->numSpacing();
  //   for (int spacingIdx = 0; spacingIdx < numSpace; ++spacingIdx) {
  //     const int spacing
  //         = static_cast<int>(std::round(layer_->spacing(spacingIdx)));
  //     const int eolWidth = static_cast<int>(
  //         std::round(layer_->spacingEolWidth(spacingIdx)));
  //     const int eolWithin = static_cast<int>(
  //         std::round(layer_->spacingEolWithin(spacingIdx)));

  //     // Pessimistic
  //     max_eol_spacing_ = std::max(max_eol_spacing_, spacing);
  //     max_eol_width_ = std::max(max_eol_width_, eolWidth);
  //     max_eol_within_ = std::max(max_eol_within_, eolWithin);
  //   }
  // }
}

int MetalLayer::getTrackLocation(const int trackIndex) const
{
  return first_track_loc_ + trackIndex * pitch_;
}

IntervalT<int> MetalLayer::rangeSearchTracks(const IntervalT<int>& loc_range,
                                             bool include_bound) const
{
  IntervalT<int> track_range(
      ceil(double(std::max(loc_range.low, first_track_loc_) - first_track_loc_)
           / double(pitch_)),
      floor(double(std::min(loc_range.high, last_track_loc_) - first_track_loc_)
            / double(pitch_)));
  if (!track_range.IsValid()) {
    return track_range;
  }
  if (!include_bound) {
    if (getTrackLocation(track_range.low) == loc_range.low) {
      ++track_range.low;
    }
    if (getTrackLocation(track_range.high) == loc_range.high) {
      --track_range.high;
    }
  }
  return track_range;
}

int MetalLayer::getParallelSpacing(const int width, const int length) const
{
  unsigned w = parallel_width_.size() - 1;
  while (w > 0 && parallel_width_[w] >= width) {
    w--;
  }
  if (length == 0) {
    return parallel_spacing_[w][0];
  }
  unsigned l = parallel_length_.size() - 1;
  while (l > 0 && parallel_length_[l] >= length) {
    l--;
  }
  return parallel_spacing_[w][l];
}

}  // namespace grt
