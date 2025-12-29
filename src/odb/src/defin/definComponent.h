// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstring>
#include <map>
#include <vector>

#include "definBase.h"
#include "odb/dbTypes.h"

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

  using MasterMap = std::map<const char*, dbMaster*, ltstr>;
  using SiteMap = std::map<const char*, dbSite*, ltstr>;
  std::vector<dbLib*> _libs;
  MasterMap _masters;
  SiteMap _sites;
  dbInst* _cur_inst;

 public:
  int _inst_cnt;
  int _update_cnt;
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
  dbSite* getSite(const char* name);

 public:
  definComponent();
  ~definComponent() override;

  void setLibs(std::vector<dbLib*>& libs) { _libs = libs; }
};

}  // namespace odb
