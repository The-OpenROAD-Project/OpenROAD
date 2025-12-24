#include "Layers.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "geo.h"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace grt {

MetalLayer::MetalLayer(odb::dbTechLayer* tech_layer,
                       odb::dbTrackGrid* track_grid)
{
  name_ = tech_layer->getName();
  index_ = tech_layer->getRoutingLevel() - 1;
  direction_
      = tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL ? H : V;
  width_ = tech_layer->getWidth();
  min_width_ = tech_layer->getMinWidth();

  track_grid->getAverageTrackSpacing(pitch_, first_track_loc_, num_tracks_);
  last_track_loc_ = first_track_loc_ + pitch_ * (num_tracks_ - 1);

  // Design rules not parsed thoroughly
  // Min area
  const int database_unit = tech_layer->getTech()->getDbUnitsPerMicron();
  const int min_area = tech_layer->getArea() * database_unit * database_unit;
  min_length_ = std::max(min_area / width_ - width_, 0);

  // Parallel run spacing
  std::vector<std::vector<uint32_t>> spacing_table;
  std::vector<uint32_t> widths;
  std::vector<uint32_t> lengths;
  tech_layer->getV55SpacingTable(spacing_table);
  tech_layer->getV55SpacingWidthsAndLengths(widths, lengths);

  for (int table_idx = 0; table_idx < spacing_table.size(); table_idx++) {
    const int num_length = lengths.size();
    if (num_length > 0) {
      parallel_length_.resize(num_length);
      for (int l = 0; l < num_length; l++) {
        parallel_length_[l] = lengths[l];
      }
    }

    const int num_width = widths.size();
    if (num_width > 0) {
      parallel_width_.resize(num_width);
      parallel_spacing_.resize(num_width);
      for (int w = 0; w < num_width; w++) {
        parallel_width_[w] = widths[w];
        parallel_spacing_[w].resize(std::max(1, num_length), 0);
        for (int l = 0; l < num_length; l++) {
          parallel_spacing_[w][l] = spacing_table[w][l];
        }
      }
    }
  }

  default_spacing_ = getParallelSpacing(width_);

  // // End-of-line spacing
  auto spacing_rules = tech_layer->getV54SpacingRules();
  for (auto rule : spacing_rules) {
    const int spacing = static_cast<int>(std::round(rule->getSpacing()));
    uint32_t eol_width, eol_within, parallel_space, parallel_within;
    bool parallel_edge, two_edges;
    if (rule->getEol(eol_width,
                     eol_within,
                     parallel_edge,
                     parallel_space,
                     parallel_within,
                     two_edges)) {
      // Pessimistic
      max_eol_spacing_ = std::max(max_eol_spacing_, spacing);
      max_eol_width_ = std::max(max_eol_width_, static_cast<int>(eol_width));
      max_eol_within_ = std::max(max_eol_within_, static_cast<int>(eol_within));
    }
  }

  adjustment_ = tech_layer->getLayerAdjustment();
}

int MetalLayer::getTrackLocation(const int track_index) const
{
  return first_track_loc_ + track_index * pitch_;
}

IntervalT MetalLayer::rangeSearchTracks(const IntervalT& loc_range,
                                        const bool include_bound) const
{
  const int lo = std::max(loc_range.low(), first_track_loc_);
  const int hi = std::min(loc_range.high(), last_track_loc_);
  const double pitch = pitch_;
  IntervalT track_range(std::ceil((lo - first_track_loc_) / pitch),
                        std::floor((hi - first_track_loc_) / pitch));
  if (!track_range.IsValid()) {
    return track_range;
  }
  if (!include_bound) {
    if (getTrackLocation(track_range.low()) == loc_range.low()) {
      track_range.addToLow(1);
    }
    if (getTrackLocation(track_range.high()) == loc_range.high()) {
      track_range.addToHigh(-1);
    }
  }
  return track_range;
}

int MetalLayer::getParallelSpacing(const int width, const int length) const
{
  int w = parallel_width_.size() - 1;
  while (w > 0 && parallel_width_[w] >= width) {
    w--;
  }
  if (length == 0) {
    return parallel_spacing_[w][0];
  }
  int l = parallel_length_.size() - 1;
  while (l > 0 && parallel_length_[l] >= length) {
    l--;
  }
  return parallel_spacing_[w][l];
}

}  // namespace grt
