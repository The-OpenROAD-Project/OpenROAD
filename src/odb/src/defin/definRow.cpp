// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definRow.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {

definRow::definRow()
{
  _cur_row = nullptr;
}

definRow::~definRow()
{
  SiteMap::iterator sitr;

  for (sitr = _sites.begin(); sitr != _sites.end(); ++sitr) {
    free((void*) (*sitr).first);
  }
}

dbSite* definRow::getSite(const char* name)
{
  SiteMap::iterator mitr;

  mitr = _sites.find(name);

  if (mitr != _sites.end()) {
    dbSite* site = (*mitr).second;
    return site;
  }

  std::vector<dbLib*>::iterator litr;

  for (litr = _libs.begin(); litr != _libs.end(); ++litr) {
    dbLib* lib = *litr;

    dbSite* site = lib->findSite(name);

    if (site) {
      const char* n = strdup(name);
      assert(n);
      _sites[n] = site;
      return site;
    }
  }

  return nullptr;
}

void definRow::begin(const char* name,
                     const char* site_name,
                     int x,
                     int y,
                     dbOrientType orient,
                     defRow direction,
                     int num_sites,
                     int spacing)
{
  dbSite* site = getSite(site_name);

  if (site == nullptr) {
    _logger->warn(
        utl::ODB,
        155,
        "error: undefined site ({}) referenced in row ({}) statement.",
        site_name,
        name);
    ++_errors;
    return;
  }

  if (direction == DEF_VERTICAL) {
    _cur_row = dbRow::create(_block,
                             name,
                             site,
                             dbdist(x),
                             dbdist(y),
                             orient,
                             dbRowDir::VERTICAL,
                             num_sites,
                             dbdist(spacing));
  } else {
    _cur_row = dbRow::create(_block,
                             name,
                             site,
                             dbdist(x),
                             dbdist(y),
                             orient,
                             dbRowDir::HORIZONTAL,
                             num_sites,
                             dbdist(spacing));
  }
}
void definRow::property(const char* name, const char* value)
{
  if (_cur_row == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_row, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbStringProperty::create(_cur_row, name, value);
}

void definRow::property(const char* name, int value)
{
  if (_cur_row == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_row, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbIntProperty::create(_cur_row, name, value);
}

void definRow::property(const char* name, double value)
{
  if (_cur_row == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_row, name);

  if (p) {
    dbProperty::destroy(p);
  }

  dbDoubleProperty::create(_cur_row, name, value);
}

void definRow::end()
{
  if (_cur_row) {
    dbSet<dbProperty> props = dbProperty::getProperties(_cur_row);

    if (!props.empty() && props.orderReversed()) {
      props.reverse();
    }
  }

  _cur_row = nullptr;
}

}  // namespace odb
