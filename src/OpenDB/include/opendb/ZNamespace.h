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

#include <string.h>
#include <map>
#include <vector>

#include "odb.h"

namespace odb {

class ZObject;
class dbDatabase;
class dbObject;

////////////////////////////////////
/// ZNamespace
////////////////////////////////////
class ZNamespace
{
  struct ltstr
  {
    bool operator()(const char* s1, const char* s2) const
    {
      return strcmp(s1, s2) < 0;
    }
  };

  class ZEntry;
  class dbEntry;

  std::map<ZObject*, ZEntry*>            _zobj_entries;
  std::map<dbDatabase*, dbEntry*>        _db_entries;
  std::map<const char*, ZEntry*, ltstr>  _zobj_names;
  std::map<const char*, dbEntry*, ltstr> _db_names;
  uint                                   _unique_id;

 public:
  ZNamespace();
  ~ZNamespace();

  // These functions may throw a ZException
  const char* addZObject(ZObject* obj);
  void        removeZObject(ZObject* obj);
  ZObject*    resolveZObject(const char* zname);
  void        registerDb(dbDatabase* db);
  void        unregisterDb(dbDatabase* db);
  dbObject*   resolveDB(const char* dbname);
};

}  // namespace odb


