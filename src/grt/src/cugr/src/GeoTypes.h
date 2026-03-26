#pragma once
#include <ostream>

#include "geo.h"
#include "odb/geom.h"

namespace grt {

BoxT getBoxFromRect(const odb::Rect& bounds);

class BoxOnLayer : public BoxT
{
 public:
  template <typename... Args>
  BoxOnLayer(int layer_index = -1, Args... params)
      : BoxT(params...), layer_idx_(layer_index)
  {
  }

  // inherit setters from BoxT in batch
  template <typename... Args>
  void set(int layer_index = -1, Args... params)
  {
    layer_idx_ = layer_index;
    BoxT::set(params...);
  }

  bool isConnected(const BoxOnLayer& rhs) const;

  int getLayerIdx() const { return layer_idx_; }

  friend std::ostream& operator<<(std::ostream& os, const BoxOnLayer& box);

 private:
  int layer_idx_;
};

}  // namespace grt
