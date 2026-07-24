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
  // Min spacing to a neighbour on this layer; used for the via-demand model.
  int getSpacing() const { return spacing_; }
  odb::dbTechLayer* getTechLayer() const { return tech_layer_; }
  // Sheet resistance (ohms/square) of this routing layer.
  double getResistance() const { return resistance_; }
  // Per-cut resistance (ohms) of the via layer just above this routing
  // layer, i.e. the via from this layer to the next routing layer up.
  double getViaResistance() const { return via_resistance_; }
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
  odb::dbTechLayer* tech_layer_ = nullptr;
  std::string name_;
  int index_;
  int direction_;
  int width_;
  int min_width_;
  int spacing_ = 0;
  double resistance_ = 0.0;
  double via_resistance_ = 0.0;

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
