#include "GeoTypes.h"

utils::BoxT<DBU> getBoxFromRsynBounds(const Bounds& bounds) {
    return {
        bounds.getLower().x, 
        bounds.getLower().y, 
        bounds.getUpper().x, 
        bounds.getUpper().y
    };
}

// BoxOnLayer

bool BoxOnLayer::isConnected(const BoxOnLayer& rhs) const {
    return abs(rhs.layerIdx - layerIdx) < 2 && HasIntersectWith(rhs);
}

ostream& operator<<(ostream& os, const BoxOnLayer& box) {
    os << "box(l=" << box.layerIdx << ", x=" << box[0] << ", y=" << box[1] << ")";
    return os;
}