#include "Layers.h"

namespace grt {

MetalLayer::MetalLayer(odb::dbTechLayer* tech_layer,
                       odb::dbTrackGrid* track_grid)
{
  name = tech_layer->getName();
  index = tech_layer->getRoutingLevel();
  direction = tech_layer->getDirection();
  width = static_cast<int>(std::round(tech_layer->getWidth()));
  minWidth = static_cast<int>(std::round(tech_layer->getMinWidth()));

  track_grid->getAverageTrackSpacing(pitch, firstTrackLoc, numTracks);
  lastTrackLoc = firstTrackLoc + pitch * (numTracks - 1);

  // Design rules not parsed thoroughly
  // Min area
  minArea = static_cast<int>(std::round(tech_layer->getArea()));
  minLength = std::max(minArea / width - width, 0);

  // TODO: Handle parallel run spacing
  // // Parallel run spacing
  // const int numSpacingTable = layer->numSpacingTable();
  // if (numSpacingTable == 0) {
  //   logger_->report("No spacing table for layer {}", name);
  // } else {
  //   for (unsigned tableIdx = 0; tableIdx < numSpacingTable; tableIdx++) {
  //     if (!layer->spacingTable(tableIdx)->isParallel()) {
  //       continue;
  //     }
  //     const lefiParallel* parallel = layer->spacingTable(tableIdx)->parallel();
  //     const int numLength = parallel->numLength();
  //     if (numLength > 0) {
  //       parallelLength.resize(numLength);
  //       for (unsigned l = 0; l < numLength; l++) {
  //         parallelLength[l]
  //             = static_cast<int>(std::round(parallel->length(l)));
  //       }
  //     }
  //     const int numWidth = parallel->numWidth();
  //     if (numWidth > 0) {
  //       parallelWidth.resize(numWidth);
  //       parallelSpacing.resize(numWidth);
  //       for (unsigned w = 0; w < numWidth; w++) {
  //         parallelWidth[w]
  //             = static_cast<int>(std::round(parallel->width(w)));
  //         parallelSpacing[w].resize(max(1, numLength), 0);
  //         for (int l = 0; l < numLength; l++) {
  //           parallelSpacing[w][l] = static_cast<int>(
  //               std::round(parallel->widthSpacing(w, l)));
  //         }
  //       }
  //     }
  //   }
  // }
  // defaultSpacing = getParallelSpacing(width);

  // TODO: Handle end-of-line spacing
  // End-of-line spacing
  // if (!layer->hasSpacingNumber()) {
  //   logger_->report("No spacing rules for layer {}", name);
  // } else {
  //   const int numSpace = layer->numSpacing();
  //   for (int spacingIdx = 0; spacingIdx < numSpace; ++spacingIdx) {
  //     const int spacing
  //         = static_cast<int>(std::round(layer->spacing(spacingIdx)));
  //     const int eolWidth = static_cast<int>(
  //         std::round(layer->spacingEolWidth(spacingIdx)));
  //     const int eolWithin = static_cast<int>(
  //         std::round(layer->spacingEolWithin(spacingIdx)));

  //     // Pessimistic
  //     maxEolSpacing = std::max(maxEolSpacing, spacing);
  //     maxEolWidth = std::max(maxEolWidth, eolWidth);
  //     maxEolWithin = std::max(maxEolWithin, eolWithin);
  //   }
  // }
}

int MetalLayer::getTrackLocation(const int trackIndex) const
{
  return firstTrackLoc + trackIndex * pitch;
}

IntervalT<int> MetalLayer::rangeSearchTracks(const IntervalT<int>& locRange,
                                             bool includeBound) const
{
  IntervalT<int> trackRange(
      ceil(double(std::max(locRange.low, firstTrackLoc) - firstTrackLoc)
           / double(pitch)),
      floor(double(std::min(locRange.high, lastTrackLoc) - firstTrackLoc)
            / double(pitch)));
  if (!trackRange.IsValid()) {
    return trackRange;
  }
  if (!includeBound) {
    if (getTrackLocation(trackRange.low) == locRange.low) {
      ++trackRange.low;
    }
    if (getTrackLocation(trackRange.high) == locRange.high) {
      --trackRange.high;
    }
  }
  return trackRange;
}

int MetalLayer::getParallelSpacing(const int width, const int length) const
{
  unsigned w = parallelWidth.size() - 1;
  while (w > 0 && parallelWidth[w] >= width) {
    w--;
  }
  if (length == 0) {
    return parallelSpacing[w][0];
  }
  unsigned l = parallelLength.size() - 1;
  while (l > 0 && parallelLength[l] >= length) {
    l--;
  }
  return parallelSpacing[w][l];
}

}  // namespace grt
