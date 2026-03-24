// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstring>
#include <map>
#include <vector>

#include "definBase.h"
#include "definTypes.h"
#include "odb/dbTypes.h"

namespace odb {

class dbSite;
class dbLib;
class dbRow;

class definRow : public definBase
{
  struct ltstr
  {
    bool operator()(const char* s1, const char* s2) const
    {
      return strcmp(s1, s2) < 0;
    }
  };

  using SiteMap = std::map<const char*, dbSite*, ltstr>;
  SiteMap _sites;
  std::vector<dbLib*> _libs;
  dbRow* _cur_row{nullptr};

 public:
  /// Row interface methods
  virtual void begin(const char* name,
                     const char* site,
                     int origin_x,
                     int origin_y,
                     dbOrientType orient,
                     defRow direction,
                     int num_sites,
                     int spacing);
  virtual void property(const char* name, const char* value);
  virtual void property(const char* name, int value);
  virtual void property(const char* name, double value);
  virtual void end();

  dbSite* getSite(const char* name);

  definRow();
  ~definRow() override;
  void setLibs(std::vector<dbLib*>& libs) { _libs = libs; }
};

}  // namespace odb
