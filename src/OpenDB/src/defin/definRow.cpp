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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "dbShape.h"
#include "definRow.h"

namespace odb {

definRow::definRow()
{
}

definRow::~definRow()
{
  SiteMap::iterator sitr;

  for (sitr = _sites.begin(); sitr != _sites.end(); ++sitr)
    free((void*) (*sitr).first);
}

void definRow::init()
{
  definBase::init();
  _libs.clear();

  SiteMap::iterator sitr;

  for (sitr = _sites.begin(); sitr != _sites.end(); ++sitr)
    free((void*) (*sitr).first);

  _sites.clear();
  _cur_row = NULL;
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

  return NULL;
}

void definRow::begin(const char*  name,
                     const char*  site_name,
                     int          x,
                     int          y,
                     dbOrientType orient,
                     defRow       direction,
                     int          num_sites,
                     int          spacing)
{
  dbSite* site = getSite(site_name);

  if (site == NULL) {
    notice(0,
           "error: undefined site (%s) referenced in row (%s) statement.\n",
           site_name,
           name);
    ++_errors;
    return;
  }

  if (direction == DEF_VERTICAL)
    _cur_row = dbRow::create(_block,
                             name,
                             site,
                             dbdist(x),
                             dbdist(y),
                             orient,
                             dbRowDir::VERTICAL,
                             num_sites,
                             dbdist(spacing));
  else
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
void definRow::property(const char* name, const char* value)
{
  if (_cur_row == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_row, name);
  if (p)
    dbProperty::destroy(p);

  dbStringProperty::create(_cur_row, name, value);
}

void definRow::property(const char* name, int value)
{
  if (_cur_row == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_row, name);
  if (p)
    dbProperty::destroy(p);

  dbIntProperty::create(_cur_row, name, value);
}

void definRow::property(const char* name, double value)
{
  if (_cur_row == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_row, name);

  if (p)
    dbProperty::destroy(p);

  dbDoubleProperty::create(_cur_row, name, value);
}

void definRow::end()
{
  if (_cur_row) {
    dbSet<dbProperty> props = dbProperty::getProperties(_cur_row);

    if (!props.empty() && props.orderReversed())
      props.reverse();
  }

  _cur_row = NULL;
}

}  // namespace odb
