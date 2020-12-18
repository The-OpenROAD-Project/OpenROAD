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
#include "definBase.h"

namespace odb {

class dbInst;
class dbMaster;
class dbLib;
class dbSite;

class definComponent : public definBase
{
  struct ltstr
  {
    bool operator()(const char* s1, const char* s2) const
    {
      return strcmp(s1, s2) < 0;
    }
  };

  typedef std::map<const char*, dbMaster*, ltstr> MasterMap;
  typedef std::map<const char*, dbSite*, ltstr>   SiteMap;
  std::vector<dbLib*>                             _libs;
  MasterMap                                       _masters;
  SiteMap                                         _sites;
  dbInst*                                         _cur_inst;

 public:
  int _inst_cnt;
  int _iterm_cnt;

  /// Component interface methods
  virtual void begin(const char* name, const char* cell);
  virtual void placement(int status, int x, int y, int orient);
  virtual void region(const char* region);
  virtual void halo(int left, int bottom, int right, int top);
  virtual void source(dbSourceType source);
  virtual void weight(int weight);
  virtual void property(const char* name, const char* value);
  virtual void property(const char* name, int value);
  virtual void property(const char* name, double value);
  virtual void end();

  dbMaster* getMaster(const char* name);
  dbSite*   getSite(const char* name);

 public:
  definComponent();
  virtual ~definComponent();

  void init();

  void setLibs(std::vector<dbLib*>& libs) { _libs = libs; }
};

}  // namespace odb


