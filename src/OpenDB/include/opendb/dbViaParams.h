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
#include "dbId.h"

namespace odb {

class _dbTechLayer;
class _dbDatabase;
class dbIStream;
class dbOStream;
class dbDiff;

class _dbViaParams
{
 public:
  int                _x_cut_size;
  int                _y_cut_size;
  int                _x_cut_spacing;
  int                _y_cut_spacing;
  int                _x_top_enclosure;
  int                _y_top_enclosure;
  int                _x_bot_enclosure;
  int                _y_bot_enclosure;
  int                _num_cut_rows;
  int                _num_cut_cols;
  int                _x_origin;
  int                _y_origin;
  int                _x_top_offset;
  int                _y_top_offset;
  int                _x_bot_offset;
  int                _y_bot_offset;
  dbId<_dbTechLayer> _top_layer;
  dbId<_dbTechLayer> _cut_layer;
  dbId<_dbTechLayer> _bot_layer;

  _dbViaParams(const _dbViaParams& v);
  _dbViaParams();
  ~_dbViaParams();

  bool operator==(const _dbViaParams& rhs) const;
  bool operator!=(const _dbViaParams& rhs) const { return !operator==(rhs); }
  void differences(dbDiff&             diff,
                   const char*         field,
                   const _dbViaParams& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  friend dbOStream& operator<<(dbOStream& stream, const _dbViaParams& v);
  friend dbIStream& operator>>(dbIStream& stream, _dbViaParams& v);
};

dbOStream& operator<<(dbOStream& stream, const _dbViaParams& v);
dbIStream& operator>>(dbIStream& stream, _dbViaParams& v);

}  // namespace odb


