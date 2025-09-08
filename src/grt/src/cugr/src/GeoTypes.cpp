#include "GeoTypes.h"

#include <cmath>
#include <ostream>

#include "geo.h"
#include "odb/geom.h"

namespace grt {

BoxT getBoxFromRect(const odb::Rect& bounds)
{
  return {bounds.ll().x(), bounds.ll().y(), bounds.ur().x(), bounds.ur().y()};
}

// BoxOnLayer

bool BoxOnLayer::isConnected(const BoxOnLayer& rhs) const
{
  return abs(rhs.getLayerIdx() - layer_idx_) < 2 && HasIntersectWith(rhs);
}

std::ostream& operator<<(std::ostream& os, const BoxOnLayer& box)
{
  os << "box(l=" << box.getLayerIdx() << ", x=" << box[0] << ", y=" << box[1]
     << ")";
  return os;
}

}  // namespace grt
