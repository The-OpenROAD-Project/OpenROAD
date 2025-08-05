#pragma once
#include "global.h"

class MetalLayer {
public:
    const static unsigned H = 0;
    const static unsigned V = 1;
    MetalLayer(const Rsyn::PhysicalLayer& rsynLayer, const vector<Rsyn::PhysicalTracks>& rsynTracks, const DBU libDBU);
    std::string getName() const { return name; }
    unsigned getDirection() const { return direction; }
    DBU getWidth() const { return width; }
    DBU getPitch() const { return pitch; }
    DBU getTrackLocation(const int trackIndex) const;
    utils::IntervalT<int> rangeSearchTracks(const utils::IntervalT<DBU>& locRange, bool includeBound = true) const;
    
    // Design rule related methods
    DBU getMinLength() const { return minLength; }
    DBU getDefaultSpacing() const { return defaultSpacing; }
    DBU getParallelSpacing(const DBU width, const DBU length = 0) const;
    DBU getMaxEolSpacing() const { return maxEolSpacing; }
    
private:
    std::string name;
    int index;
    unsigned direction;
    DBU width;
    DBU minWidth;
    
    // tracks 
    DBU firstTrackLoc;
    DBU lastTrackLoc;
    DBU pitch;
    int numTracks;
    
    // Design rules
    // Min area 
    DBU minArea;
    DBU minLength;
    
    // Parallel run spacing 
    vector<DBU> parallelWidth = {0};
    vector<DBU> parallelLength = {0};
    vector<vector<DBU>> parallelSpacing = {{0}}; // width, length -> spacing
    DBU defaultSpacing = 0;
    
    // End-of-line spacing
    DBU maxEolSpacing = 0;
    DBU maxEolWidth = 0;
    DBU maxEolWithin = 0;
    
    // Corner spacing
    
};
