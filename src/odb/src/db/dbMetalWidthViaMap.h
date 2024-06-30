///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/odb.h"

// User Code Begin Includes
#include "dbVector.h"
#include "odb/db.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbTechLayer;

class _dbMetalWidthViaMap : public _dbObject
{
 public:
  _dbMetalWidthViaMap(_dbDatabase*, const _dbMetalWidthViaMap& r);
  _dbMetalWidthViaMap(_dbDatabase*);

  ~_dbMetalWidthViaMap() = default;

  bool operator==(const _dbMetalWidthViaMap& rhs) const;
  bool operator!=(const _dbMetalWidthViaMap& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbMetalWidthViaMap& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbMetalWidthViaMap& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  bool via_cut_class_;
  dbId<_dbTechLayer> cut_layer_;
  int below_layer_width_low_;
  int below_layer_width_high_;
  int above_layer_width_low_;
  int above_layer_width_high_;
  std::string via_name_;
  bool pg_via_;
};
dbIStream& operator>>(dbIStream& stream, _dbMetalWidthViaMap& obj);
dbOStream& operator<<(dbOStream& stream, const _dbMetalWidthViaMap& obj);
}  // namespace odb
   // Generator Code End Header
