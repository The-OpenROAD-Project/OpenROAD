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

#include "dbBlock.h"
#include "dbCore.h"
#include "dbMTerm.h"
#include "dbNet.h"
#include "dbRegion.h"
#include "dbVector.h"
#include "odb/odb.h"
// User Code Begin Includes
#include <map>
#include <regex>
#include <set>
#include <vector>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbRegion;
class _dbNet;
// User Code Begin Classes
class dbMaster;
class dbMTerm;
class dbITerm;
// User Code End Classes

class _dbGlobalConnect : public _dbObject
{
 public:
  _dbGlobalConnect(_dbDatabase*, const _dbGlobalConnect& r);
  _dbGlobalConnect(_dbDatabase*);

  ~_dbGlobalConnect() = default;

  bool operator==(const _dbGlobalConnect& rhs) const;
  bool operator!=(const _dbGlobalConnect& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbGlobalConnect& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbGlobalConnect& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin Methods
  void setupRegex();
  static void testRegex(utl::Logger* logger,
                        const std::string& pattern,
                        const std::string& type);
  std::map<dbMaster*, std::set<dbMTerm*>> getMTermMapping();
  std::set<dbITerm*> connect(const std::vector<dbInst*>& insts);
  bool appliesTo(dbInst* inst) const;
  // User Code End Methods

  dbId<_dbRegion> region_;
  dbId<_dbNet> net_;
  std::string inst_pattern_;
  std::string pin_pattern_;
  std::regex inst_regex_;
};
dbIStream& operator>>(dbIStream& stream, _dbGlobalConnect& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGlobalConnect& obj);
}  // namespace odb
   // Generator Code End Header
