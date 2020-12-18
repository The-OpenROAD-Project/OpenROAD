///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "odb.h"
#include "dbTypes.h"
#include "geom.h"

namespace odb {

class dbOStream;
class dbIStream;
class dbDiff;

//
// Database Transform - Transform points by rotation and translation.
//
class dbTransform
{
  friend class _dbBlock;
  dbOrientType::Value _orient;
  Point            _offset;

 public:
  // T = <R0, (0,0)>
  dbTransform() : _orient(dbOrientType::R0), _offset(0, 0) {}

  //  T = <R0, offset>
  dbTransform(Point offset) : _orient(dbOrientType::R0), _offset(offset) {}

  //  T = <orient, (0,0)>
  dbTransform(dbOrientType orient) : _orient(orient), _offset(0, 0) {}

  //  T = <orient, offset>
  dbTransform(dbOrientType orient, Point offset)
      : _orient(orient), _offset(offset)
  {
  }

  bool operator==(const dbTransform& t) const
  {
    return (_orient == t._orient) && (_offset == t._offset);
  }

  bool operator!=(const dbTransform& t) const { return !operator==(t); }

  void setOrient(dbOrientType orient) { _orient = orient; }

  void setOffset(Point offset) { _offset = offset; }

  void setTransform(dbOrientType orient, Point offset)
  {
    _orient = orient;
    _offset = offset;
  }

  // Apply transform to this point
  void apply(Point& p) const;

  // Apply transform to this point
  void apply(Rect& r) const;

  // Post multiply transform.
  void concat(const dbTransform& t);

  // Post multiply transform
  void concat(const dbTransform& t, dbTransform& result);

  // Compute inverse transform
  void invert(dbTransform& result) const;

  // Compute inverse transform
  void invert();

  dbOrientType getOrient() const { return _orient; }
  Point     getOffset() const { return _offset; }

  friend dbOStream& operator<<(dbOStream& stream, const dbTransform& t);
  friend dbIStream& operator>>(dbIStream& stream, dbTransform& t);
  friend dbDiff&    operator<<(dbDiff& diff, const dbTransform& t);
};

dbOStream& operator<<(dbOStream& stream, const dbTransform& t);
dbIStream& operator>>(dbIStream& stream, dbTransform& t);
dbDiff&    operator<<(dbDiff& diff, const dbTransform& t);

inline void dbTransform::concat(const dbTransform& t)
{
  dbTransform result;
  concat(t, result);
  *this = result;
}

inline void dbTransform::invert()
{
  dbTransform result;
  invert(result);
  *this = result;
}

}  // namespace odb


