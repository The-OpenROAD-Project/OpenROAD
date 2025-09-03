#pragma once
#include <ostream>

#include "CUGR.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace grt {

BoxT<int> getBoxFromRect(const odb::Rect& bounds);

class BoxOnLayer : public BoxT<int>
{
 public:
  template <typename... Args>
  BoxOnLayer(int layerIndex = -1, Args... params)
      : BoxT<int>(params...), layer_idx_(layerIndex)
  {
  }

  // inherit setters from BoxT in batch
  template <typename... Args>
  void Set(int layerIndex = -1, Args... params)
  {
    layer_idx_ = layerIndex;
    BoxT<int>::Set(params...);
  }

  bool isConnected(const BoxOnLayer& rhs) const;

  int getLayerIdx() const { return layer_idx_; }

  friend std::ostream& operator<<(std::ostream& os, const BoxOnLayer& box);

 private:
  int layer_idx_;
};

}  // namespace grt