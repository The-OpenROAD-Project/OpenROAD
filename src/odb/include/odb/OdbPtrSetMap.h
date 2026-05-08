#pragma once

#include <functional>
#include <map>
#include <set>

namespace odb {

class dbObject;

bool compare_by_id(const dbObject* lhs, const dbObject* rhs);

struct ODBPtrLess
{
  bool operator()(const dbObject* lhs, const dbObject* rhs) const
  {
    return compare_by_id(lhs, rhs);
  }
};

template <typename T>
using OdbPtrSet = std::set<T*, ODBPtrLess>;

template <typename K, typename V>
using OdbPtrMap = std::map<K*, V, ODBPtrLess>;

}  // namespace odb
