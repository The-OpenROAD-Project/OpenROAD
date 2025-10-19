// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbDatabase.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbNet;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbRSegFlags
{
  uint _path_dir : 1;       // 0 == low to hi coord
  uint _allocated_cap : 1;  // 0, cap points to target node cap
                            // 1, cap is allocated
  uint _update_cap : 1;
  uint _spare_bits_29 : 29;
};

class _dbRSeg : public _dbObject
{
 public:
  enum Field  // dbJournal field names
  {
    FLAGS,
    SOURCE,
    TARGET,
    RESISTANCE,
    CAPACITANCE,
    COORDINATES,
    ADDRSEGCAPACITANCE,
    ADDRSEGRESISTANCE
  };

  // PERSISTANT-MEMBERS
  _dbRSegFlags _flags;
  uint _source;  // rc-network node-id
  uint _target;  // rc-network node-id
  int _xcoord;
  int _ycoord;
  dbId<_dbRSeg> _next;

  _dbRSeg(_dbDatabase*, const _dbRSeg& s);
  _dbRSeg(_dbDatabase*);
  ~_dbRSeg();

  bool operator==(const _dbRSeg& rhs) const;
  bool operator!=(const _dbRSeg& rhs) const { return !operator==(rhs); }
  void collectMemInfo(MemInfo& info);
  bool operator<(const _dbRSeg& rhs) const
  {
    _dbRSeg* o1 = (_dbRSeg*) this;
    _dbRSeg* o2 = (_dbRSeg*) &rhs;
    return o1->getOID() < o2->getOID();
  }
};

inline _dbRSeg::_dbRSeg(_dbDatabase*, const _dbRSeg& s)
    : _flags(s._flags),
      _source(s._source),
      _target(s._target),
      _xcoord(s._xcoord),
      _ycoord(s._ycoord),
      _next(s._next)
{
}

inline _dbRSeg::_dbRSeg(_dbDatabase*)
{
  _flags._spare_bits_29 = 0;
  _flags._update_cap = 0;
  _flags._path_dir = 0;
  _flags._allocated_cap = 0;

  _source = 0;
  _target = 0;
  _xcoord = 0;
  _ycoord = 0;
}

inline _dbRSeg::~_dbRSeg()
{
  /*
  if ( _res )
      free( (void *) _res );

  if (( _cap ) && (_flags._allocated_cap>0))
      free( (void *) _cap );
  */
}

inline dbOStream& operator<<(dbOStream& stream, const _dbRSeg& seg)
{
  uint* bit_field = (uint*) &seg._flags;
  stream << *bit_field;
  stream << seg._source;
  stream << seg._target;
  stream << seg._xcoord;
  stream << seg._ycoord;
  stream << seg._next;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbRSeg& seg)
{
  uint* bit_field = (uint*) &seg._flags;
  stream >> *bit_field;
  stream >> seg._source;
  stream >> seg._target;
  stream >> seg._xcoord;
  stream >> seg._ycoord;
  stream >> seg._next;
  return stream;
}

}  // namespace odb
