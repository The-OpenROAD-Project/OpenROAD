#include "hashUtil.h"

namespace MacroPlace {

std::size_t 
PointerPairHash::operator()(const std::pair<void*, void*> &k) const {
  return std::hash<void*>()(k.first) * 3 + std::hash<void*>()(k.second);
}

bool
PointerPairEqual::operator()(const std::pair<void*, void*> &p1, 
                  const std::pair<void*, void*> &p2) const {
  return p1 == p2;
}

}
