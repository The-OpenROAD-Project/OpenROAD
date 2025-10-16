#pragma once

#include <string>
#include <vector>

#include "geo.h"
#include "odb/db.h"

namespace grt {

class MetalLayer
{
 public:
  const static int H = 0;
  const static int V = 1;

  MetalLayer(odb::dbTechLayer* tech_layer, odb::dbTrackGrid* track_grid);

  std::string getName() const { return name_; }
  int getIndex() const { return index_; }
  int getDirection() const { return direction_; }
  int getWidth() const { return width_; }
  int getPitch() const { return pitch_; }
  int getTrackLocation(int track_index) const;
  IntervalT rangeSearchTracks(const IntervalT& loc_range,
                              bool include_bound = true) const;

  // Design rule related methods
  int getMinLength() const { return min_length_; }
  int getDefaultSpacing() const { return default_spacing_; }
  int getParallelSpacing(int width, int length = 0) const;
  int getMaxEolSpacing() const { return max_eol_spacing_; }
  float getAdjustment() const { return adjustment_; }
  void setAdjustment(float adjustment) { adjustment_ = adjustment; }

 private:
  std::string name_;
  int index_;
  int direction_;
  int width_;
  int min_width_;

  // tracks
  int first_track_loc_;
  int last_track_loc_;
  int pitch_;
  int num_tracks_;

  // Design rules
  int min_length_;

  // Parallel run spacing
  std::vector<int> parallel_width_ = {0};
  std::vector<int> parallel_length_ = {0};
  // width, length -> spacing
  std::vector<std::vector<int>> parallel_spacing_ = {{0}};
  int default_spacing_ = 0;

  // End-of-line spacing
  int max_eol_spacing_ = 0;
  int max_eol_width_ = 0;
  int max_eol_within_ = 0;

  // User-defined capacity adjustment
  float adjustment_ = 0.0;

  // Corner spacing
};

}  // namespace grt
