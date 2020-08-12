#ifndef __MACRO_PLACER_HASH_UTIL__
#define __MACRO_PLACER_HASH_UTIL__

#include <cstdlib>
#include <utility>
#include <functional>

namespace MacroPlace {

struct PointerPairHash {
  std::size_t operator()(const std::pair<void*, void*> &k) const;
};

struct PointerPairEqual {
  bool operator()(const std::pair<void*, void*> &p1, 
                  const std::pair<void*, void*> &p2) const;
};

};

#endif
