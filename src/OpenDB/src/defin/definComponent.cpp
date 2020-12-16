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
#include "dbTransform.h"
#include "definComponent.h"
#include "defiComponent.hpp"
#include "defiUtil.hpp"

namespace odb {

definComponent::definComponent()
{
  init();
}

definComponent::~definComponent()
{
  MasterMap::iterator mitr;

  for (mitr = _masters.begin(); mitr != _masters.end(); ++mitr)
    free((void*) (*mitr).first);

  SiteMap::iterator sitr;

  for (sitr = _sites.begin(); sitr != _sites.end(); ++sitr)
    free((void*) (*sitr).first);
}

void definComponent::init()
{
  definBase::init();
  _libs.clear();

  MasterMap::iterator mitr;
  for (mitr = _masters.begin(); mitr != _masters.end(); ++mitr)
    free((void*) (*mitr).first);

  _masters.clear();

  SiteMap::iterator sitr;

  for (sitr = _sites.begin(); sitr != _sites.end(); ++sitr)
    free((void*) (*sitr).first);

  _sites.clear();
  _inst_cnt  = 0;
  _iterm_cnt = 0;
  _cur_inst  = NULL;
}

dbMaster* definComponent::getMaster(const char* name)
{
  MasterMap::iterator mitr;

  mitr = _masters.find(name);

  if (mitr != _masters.end()) {
    dbMaster* master = (*mitr).second;
    return master;
  }

  std::vector<dbLib*>::iterator litr;

  for (litr = _libs.begin(); litr != _libs.end(); ++litr) {
    dbLib* lib = *litr;

    dbMaster* master = lib->findMaster(name);

    if (master) {
      const char* n = strdup(name);
      assert(n);
      _masters[n] = master;
      return master;
    }
  }

  return NULL;
}

dbSite* definComponent::getSite(const char* name)
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

void definComponent::begin(const char* iname, const char* mname)
{
  assert(_cur_inst == NULL);
  dbMaster* master = getMaster(mname);

  if (master == NULL) {
    notice(0,
           "error: unknown library cell referenced (%s) for instance (%s)\n",
           mname,
           iname);
    _errors++;
    return;
  }

  _cur_inst = dbInst::create(_block, master, iname);

  if (_cur_inst == NULL) {
    notice(0, "error: duplicate instance definition(%s)\n", iname);
    _errors++;
    return;
  }

  _iterm_cnt += master->getMTermCount();
  _inst_cnt++;
  if (_inst_cnt % 100000 == 0)
    notice(0, "\t\tCreated %d Insts\n", _inst_cnt);
}

void definComponent::placement(int status, int x, int y, int orient)
{
  if (_cur_inst == NULL)
    return;

  switch (status) {
    case DEFI_COMPONENT_FIXED:
      _cur_inst->setPlacementStatus(dbPlacementStatus::FIRM);
      break;
    case DEFI_COMPONENT_COVER:
      _cur_inst->setPlacementStatus(dbPlacementStatus::COVER);
      break;
    case DEFI_COMPONENT_PLACED:
      _cur_inst->setPlacementStatus(dbPlacementStatus::PLACED);
      break;
    case DEFI_COMPONENT_UNPLACED:
      _cur_inst->setPlacementStatus(dbPlacementStatus::UNPLACED);
      return;
    default: // placement status is unset
      return;
  }

  switch (orient) {
    case DEF_ORIENT_N:
      _cur_inst->setOrient(dbOrientType::R0);
      break;
    case DEF_ORIENT_S:
      _cur_inst->setOrient(dbOrientType::R180);
      break;
    case DEF_ORIENT_E:
      _cur_inst->setOrient(dbOrientType::R270);
      break;
    case DEF_ORIENT_W:
      _cur_inst->setOrient(dbOrientType::R90);
      break;
    case DEF_ORIENT_FN:
      _cur_inst->setOrient(dbOrientType::MY);
      break;
    case DEF_ORIENT_FS:
      _cur_inst->setOrient(dbOrientType::MX);
      break;
    case DEF_ORIENT_FE:
      _cur_inst->setOrient(dbOrientType::MYR90);
      break;
    case DEF_ORIENT_FW:
      _cur_inst->setOrient(dbOrientType::MXR90);
      break;
  }

  dbMaster* master = _cur_inst->getMaster();
  Rect   bbox;
  master->getPlacementBoundary(bbox);
  dbTransform t(_cur_inst->getOrient());
  t.apply(bbox);
  _cur_inst->setOrigin(dbdist(x) - bbox.xMin(), dbdist(y) - bbox.yMin());
}

void definComponent::halo(int left, int bottom, int right, int top)
{
  dbBox::create(
      _cur_inst, dbdist(left), dbdist(bottom), dbdist(right), dbdist(top));
}

void definComponent::region(const char* name)
{
  if (_cur_inst == NULL)
    return;

  dbRegion* region = _block->findRegion(name);

  if (region == NULL)
    region = dbRegion::create(_block, name);

  region->addInst(_cur_inst);
}

void definComponent::source(dbSourceType source)
{
  if (_cur_inst == NULL)
    return;

  _cur_inst->setSourceType(source);
}

void definComponent::weight(int weight)
{
  if (_cur_inst == NULL)
    return;

  _cur_inst->setWeight(weight);
}

void definComponent::property(const char* name, const char* value)
{
  if (_cur_inst == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_inst, name);
  if (p)
    dbProperty::destroy(p);

  dbStringProperty::create(_cur_inst, name, value);
}

void definComponent::property(const char* name, int value)
{
  if (_cur_inst == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_inst, name);
  if (p)
    dbProperty::destroy(p);

  dbIntProperty::create(_cur_inst, name, value);
}

void definComponent::property(const char* name, double value)
{
  if (_cur_inst == NULL)
    return;

  dbProperty* p = dbProperty::find(_cur_inst, name);

  if (p)
    dbProperty::destroy(p);

  dbDoubleProperty::create(_cur_inst, name, value);
}

void definComponent::end()
{
  if (_cur_inst == NULL)
    return;

  dbSet<dbProperty> props = dbProperty::getProperties(_cur_inst);

  if (!props.empty() && props.orderReversed())
    props.reverse();

  _cur_inst = NULL;
}

}  // namespace odb
