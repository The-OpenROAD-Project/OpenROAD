#include "Layers.h"

MetalLayer::MetalLayer(const Rsyn::PhysicalLayer& rsynLayer, const vector<Rsyn::PhysicalTracks>& rsynTracks, const DBU libDBU) {
    lefiLayer* layer = rsynLayer.getLayer();
    name = layer->name();
    index = rsynLayer.getRelativeIndex();
    direction = strcmp(layer->direction(), "HORIZONTAL") ? V : H;
    width = static_cast<DBU>(std::round(layer->width() * libDBU));
    minWidth = static_cast<DBU>(std::round(layer->minwidth() * libDBU));
    
    for (const Rsyn::PhysicalTracks& rsynTrack : rsynTracks) {
        if ((rsynTrack.getDirection() == Rsyn::TRACK_HORIZONTAL) == (direction == H)) {
            // Only consider the tracks of the prefered direction
            pitch = rsynTrack.getSpace();
            numTracks = rsynTrack.getNumberOfTracks();
            firstTrackLoc = rsynTrack.getLocation();
            lastTrackLoc = firstTrackLoc + pitch * (numTracks - 1);
            break;
        }
    }
    
    // Design rules not parsed thoroughly
    // Min area
    minArea = static_cast<DBU>(std::round(layer->area() * libDBU * libDBU));
    minLength = max(minArea / width - width, (DBU)0);
    
    // Parallel run spacing
    const int numSpacingTable = layer->numSpacingTable();
    if (numSpacingTable == 0) {
        log() << "Warning in " << __func__ << ": no spacing table for " << name << std::endl;
    } else {
        for (unsigned tableIdx = 0; tableIdx < numSpacingTable; tableIdx++) {
            if (!layer->spacingTable(tableIdx)->isParallel()) continue;
            const lefiParallel* parallel = layer->spacingTable(tableIdx)->parallel();
            const int numLength = parallel->numLength();
            if (numLength > 0) {
                parallelLength.resize(numLength);
                for (unsigned l = 0; l < numLength; l++) {
                    parallelLength[l] = static_cast<DBU>(std::round(parallel->length(l) * libDBU));
                }
            }
            const int numWidth = parallel->numWidth();
            if (numWidth > 0) {
                parallelWidth.resize(numWidth);
                parallelSpacing.resize(numWidth);
                for (unsigned w = 0; w < numWidth; w++) {
                    parallelWidth[w] = static_cast<DBU>(std::round(parallel->width(w) * libDBU));
                    parallelSpacing[w].resize(max(1, numLength), 0);
                    for (int l = 0; l < numLength; l++) {
                        parallelSpacing[w][l] = static_cast<DBU>(std::round(parallel->widthSpacing(w, l) * libDBU));
                    }
                }
            }
        }
    }
    defaultSpacing = getParallelSpacing(width);
    
    // End-of-line spacing
    if (!layer->hasSpacingNumber()) {
        log() << "Warning in " << __func__ << ": no spacing rules for " << name << std::endl;
    } else {
        const int numSpace = layer->numSpacing();
        for (int spacingIdx = 0; spacingIdx < numSpace; ++spacingIdx) {
            const DBU spacing = static_cast<DBU>(std::round(layer->spacing(spacingIdx) * libDBU));
            const DBU eolWidth = static_cast<DBU>(std::round(layer->spacingEolWidth(spacingIdx) * libDBU));
            const DBU eolWithin = static_cast<DBU>(std::round(layer->spacingEolWithin(spacingIdx) * libDBU));
    
            // Not considered
            // const DBU parSpace = static_cast<DBU>(std::round(layer->spacingParSpace(spacingIdx) * libDBU));
            // const DBU parWithin = static_cast<DBU>(std::round(layer->spacingParWithin(spacingIdx) * libDBU));
    
            // Pessimistic
            maxEolSpacing = std::max(maxEolSpacing, spacing);
            maxEolWidth = std::max(maxEolWidth, eolWidth);
            maxEolWithin = std::max(maxEolWithin, eolWithin);
        }
    }
    
    delete rsynLayer.getLayer();
}

DBU MetalLayer::getTrackLocation(const int trackIndex) const {
    return firstTrackLoc + trackIndex * pitch;
}

utils::IntervalT<int> MetalLayer::rangeSearchTracks(const utils::IntervalT<DBU>& locRange, bool includeBound) const {
    utils::IntervalT<int> trackRange(
        ceil(double(max(locRange.low, firstTrackLoc) - firstTrackLoc) / double(pitch)),
        floor(double(min(locRange.high, lastTrackLoc) - firstTrackLoc) / double(pitch))
    );
    if (!trackRange.IsValid()) return trackRange;
    if (!includeBound) {
        if (getTrackLocation(trackRange.low) == locRange.low) ++trackRange.low;
        if (getTrackLocation(trackRange.high) == locRange.high) --trackRange.high;
    }
    return trackRange;
}

DBU MetalLayer::getParallelSpacing(const DBU width, const DBU length) const {
    unsigned w = parallelWidth.size() - 1;
    while (w > 0 && parallelWidth[w] >= width) w--;
    if (length == 0) return parallelSpacing[w][0];
    unsigned l = parallelLength.size() - 1;
    while (l > 0 && parallelLength[l] >= length) l--;
    return parallelSpacing[w][l];
}
