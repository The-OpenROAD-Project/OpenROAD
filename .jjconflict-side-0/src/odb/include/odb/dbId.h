// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstddef>

#include "odb/dbStream.h"

namespace odb {

template <class T>
class dbId
{
  constexpr static unsigned int invalid = 0;
  unsigned int _id = invalid;

 public:
  using _type = T;

  dbId() = default;
  dbId(const dbId<T>& id) = default;
  dbId(unsigned int id) : _id(id) {}

  operator unsigned int() const { return _id; }
  unsigned int& id() { return _id; }

  bool isValid() const { return _id != invalid; }
  void clear() { _id = invalid; }

  friend dbOStream& operator<<(dbOStream& stream, const dbId<T>& id)
  {
    stream << id._id;
    return stream;
  }

  friend dbIStream& operator>>(dbIStream& stream, dbId<T>& id)
  {
    stream >> id._id;
    return stream;
  }
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
