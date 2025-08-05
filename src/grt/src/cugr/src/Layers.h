#pragma once

#include "geo.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace grt {

class MetalLayer
{
 public:
  MetalLayer(odb::dbTechLayer* tech_layer,
             odb::dbTrackGrid* track_grid);
  std::string getName() const { return name; }
  unsigned getDirection() const { return direction; }
  int getWidth() const { return width; }
  int getPitch() const { return pitch; }
  int getTrackLocation(const int trackIndex) const;
  IntervalT<int> rangeSearchTracks(const IntervalT<int>& locRange,
                                   bool includeBound = true) const;

  // Design rule related methods
  int getMinLength() const { return minLength; }
  int getDefaultSpacing() const { return defaultSpacing; }
  int getParallelSpacing(const int width, const int length = 0) const;
  int getMaxEolSpacing() const { return maxEolSpacing; }

 private:
  std::string name;
  int index;
  odb::dbTechLayerDir direction;
  int width;
  int minWidth;

  // tracks
  int firstTrackLoc;
  int lastTrackLoc;
  int pitch;
  int numTracks;

  // Design rules
  // Min area
  int minArea;
  int minLength;

  // Parallel run spacing
  std::vector<int> parallelWidth = {0};
  std::vector<int> parallelLength = {0};
  std::vector<std::vector<int>> parallelSpacing
      = {{0}};  // width, length -> spacing
  int defaultSpacing = 0;

  // End-of-line spacing
  int maxEolSpacing = 0;
  int maxEolWidth = 0;
  int maxEolWithin = 0;

  // Corner spacing
};

}  // namespace grt
