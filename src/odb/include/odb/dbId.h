// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <functional>

#include "odb/dbStream.h"

namespace odb {

template <class T>
class dbId
{
 public:
  using _type = T;

  dbId() = default;
  dbId(const dbId<T>& id) = default;
  dbId(unsigned int id) : id_(id) {}

  operator unsigned int() const { return id_; }
  unsigned int id() const { return id_; }

  bool isValid() const { return id_ != invalid; }
  void clear() { id_ = invalid; }

  friend dbOStream& operator<<(dbOStream& stream, const dbId<T>& id)
  {
    stream << id.id_;
    return stream;
  }

  friend dbIStream& operator>>(dbIStream& stream, dbId<T>& id)
  {
    stream >> id.id_;
    return stream;
  }

 private:
  constexpr static unsigned int invalid = 0;
  unsigned int id_ = invalid;
};

}  // namespace odb

// Enable unordered_map/set usage
namespace std {

template <typename T>
struct hash<odb::dbId<T>>
{
  std::size_t operator()(const odb::dbId<T>& db_id) const
  {
    return std::hash<unsigned int>()(db_id);
  }
};

}  // namespace std
