///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;

class _dbTechLayerForbiddenSpacingRule : public _dbObject
{
 public:
  _dbTechLayerForbiddenSpacingRule(_dbDatabase*,
                                   const _dbTechLayerForbiddenSpacingRule& r);
  _dbTechLayerForbiddenSpacingRule(_dbDatabase*);

  ~_dbTechLayerForbiddenSpacingRule() = default;

  bool operator==(const _dbTechLayerForbiddenSpacingRule& rhs) const;
  bool operator!=(const _dbTechLayerForbiddenSpacingRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerForbiddenSpacingRule& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbTechLayerForbiddenSpacingRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  std::pair<int, int> forbidden_spacing_;
  int width_;
  int within_;
  int prl_;
  int two_edges_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerForbiddenSpacingRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerForbiddenSpacingRule& obj);
}  // namespace odb
   // Generator Code End Header
