// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbStream.h"

namespace odb {

template <class T>
class dbId
{
  constexpr static unsigned int invalid = 0;
  unsigned int _id;

 public:
  using _type = T;

  dbId() { _id = invalid; }
  dbId(const dbId<T>& id) : _id(id._id) {}
  dbId(unsigned int id) { _id = id; }

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
