#pragma once
#include "global.h"

namespace grt {

BoxT<DBU> getBoxFromRsynBounds(const Bounds& bounds);

class BoxOnLayer : public BoxT<DBU>
{
 public:
  int layerIdx;

  //  constructors
  template <typename... Args>
  BoxOnLayer(int layerIndex = -1, Args... params)
      : layerIdx(layerIndex), BoxT<DBU>(params...)
  {
  }

  // inherit setters from BoxT in batch
  template <typename... Args>
  void Set(int layerIndex = -1, Args... params)
  {
    layerIdx = layerIndex;
    BoxT<DBU>::Set(params...);
  }

  bool isConnected(const BoxOnLayer& rhs) const;

  friend ostream& operator<<(ostream& os, const BoxOnLayer& box);
};

}  // namespace grt