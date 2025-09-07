#include "Layers.h"

#include <algorithm>
#include <cmath>
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
  width_ = static_cast<int>(std::round(tech_layer->getWidth()));
  min_width_ = static_cast<int>(std::round(tech_layer->getMinWidth()));

  track_grid->getAverageTrackSpacing(pitch_, first_track_loc_, num_tracks_);
  last_track_loc_ = first_track_loc_ + pitch_ * (num_tracks_ - 1);

  // Design rules not parsed thoroughly
  // Min area
  const int database_unit = tech_layer->getTech()->getDbUnitsPerMicron();
  int min_area = static_cast<int>(
      std::round(tech_layer->getArea() * database_unit * database_unit));
  min_length_ = std::max(min_area / width_ - width_, 0);

  // Parallel run spacing
  std::vector<std::vector<uint>> spacing_table;
  std::vector<uint> widths;
  std::vector<uint> lengths;
  tech_layer->getV55SpacingTable(spacing_table);
  tech_layer->getV55SpacingWidthsAndLengths(widths, lengths);

  for (int table_idx = 0; table_idx < spacing_table.size(); table_idx++) {
    const int num_length = lengths.size();
    if (num_length > 0) {
      parallel_length_.resize(num_length);
      for (int l = 0; l < num_length; l++) {
        parallel_length_[l] = static_cast<int>(std::round(lengths[l]));
      }
    }

    const int num_width = widths.size();
    if (num_width > 0) {
      parallel_width_.resize(num_width);
      parallel_spacing_.resize(num_width);
      for (int w = 0; w < num_width; w++) {
        parallel_width_[w] = static_cast<int>(std::round(widths[w]));
        parallel_spacing_[w].resize(std::max(1, num_length), 0);
        for (int l = 0; l < num_length; l++) {
          parallel_spacing_[w][l]
              = static_cast<int>(std::round(spacing_table[w][l]));
        }
      }
    }
  }

  default_spacing_ = getParallelSpacing(width_);

  // // End-of-line spacing
  auto spacing_rules = tech_layer->getV54SpacingRules();
  for (auto rule : spacing_rules) {
    const int spacing = static_cast<int>(std::round(rule->getSpacing()));
    uint eol_width, eol_within, parallel_space, parallel_within;
    bool parallel_edge, two_edges;
    rule->getEol(eol_width,
                 eol_within,
                 parallel_edge,
                 parallel_space,
                 parallel_within,
                 two_edges);
    // Pessimistic
    max_eol_spacing_ = std::max(max_eol_spacing_, spacing);
    max_eol_width_ = std::max(max_eol_width_, static_cast<int>(eol_width));
    max_eol_within_ = std::max(max_eol_within_, static_cast<int>(eol_within));
  }
}

int MetalLayer::getTrackLocation(const int track_index) const
{
  return first_track_loc_ + track_index * pitch_;
}

IntervalT MetalLayer::rangeSearchTracks(const IntervalT& loc_range,
                                        bool include_bound) const
{
  IntervalT track_range(
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
