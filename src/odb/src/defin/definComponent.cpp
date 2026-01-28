// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definComponent.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "defiComponent.hpp"
#include "defiUtil.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

definComponent::definComponent()
{
  _libs.clear();

  MasterMap::iterator mitr;
  for (mitr = _masters.begin(); mitr != _masters.end(); ++mitr) {
    free((void*) (*mitr).first);
  }

  _masters.clear();

  SiteMap::iterator sitr;

  for (sitr = _sites.begin(); sitr != _sites.end(); ++sitr) {
    free((void*) (*sitr).first);
  }

  _sites.clear();
  _inst_cnt = 0;
  _update_cnt = 0;
  _iterm_cnt = 0;
  _cur_inst = nullptr;
}

definComponent::~definComponent()
{
  MasterMap::iterator mitr;

  for (mitr = _masters.begin(); mitr != _masters.end(); ++mitr) {
    free((void*) (*mitr).first);
  }

  SiteMap::iterator sitr;

  for (sitr = _sites.begin(); sitr != _sites.end(); ++sitr) {
    free((void*) (*sitr).first);
  }
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

  return nullptr;
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

  return nullptr;
}

void definComponent::begin(const char* iname, const char* mname)
{
  assert(_cur_inst == nullptr);
  dbMaster* master = getMaster(mname);

  if (master == nullptr) {
    _logger->warn(
        utl::ODB,
        92,
        "error: unknown library cell referenced ({}) for instance ({})",
        mname,
        iname);
    _errors++;
    return;
  }
  if (_mode != defin::DEFAULT) {
    _cur_inst = _block->findInst(iname);
    if (_cur_inst == nullptr) {
      _errors++;
      return;
    }
    _update_cnt++;
  } else {
    _cur_inst = dbInst::create(_block, master, iname);
    if (_cur_inst != nullptr) {
      _inst_cnt++;
      _iterm_cnt += master->getMTermCount();
    } else {
      _logger->warn(
          utl::ODB, 93, "error: duplicate instance definition({})", iname);
      _errors++;
      return;
    }
    if (_inst_cnt % 100000 == 0) {
      _logger->info(utl::ODB, 94, "\t\tCreated {} Insts", _inst_cnt);
    }
  }
}

void definComponent::placement(int status, int x, int y, int orient)
{
  if (_cur_inst == nullptr) {
    return;
  }

  dbPlacementStatus placement_status;
  switch (status) {
    case DEFI_COMPONENT_FIXED:
      placement_status = dbPlacementStatus::FIRM;
      break;
    case DEFI_COMPONENT_COVER:
      placement_status = dbPlacementStatus::COVER;
      break;
    case DEFI_COMPONENT_PLACED:
      placement_status = dbPlacementStatus::PLACED;
      break;
    case DEFI_COMPONENT_UNPLACED:
      _cur_inst->setPlacementStatus(dbPlacementStatus::UNPLACED);
      return;
    default:  // placement status is unset
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
  Rect bbox;
  master->getPlacementBoundary(bbox);
  dbTransform t(_cur_inst->getOrient());
  t.apply(bbox);
  _cur_inst->setOrigin(dbdist(x) - bbox.xMin(), dbdist(y) - bbox.yMin());
  _cur_inst->setPlacementStatus(placement_status);
}

void definComponent::halo(int left, int bottom, int right, int top)
{
  if (_cur_inst == nullptr) {
    return;
  }

  dbBox::create(
      _cur_inst, dbdist(left), dbdist(bottom), dbdist(right), dbdist(top));
}

void definComponent::region(const char* name)
{
  if (_cur_inst == nullptr) {
    return;
  }

  dbRegion* region = _block->findRegion(name);

  if (region == nullptr) {
    region = dbRegion::create(_block, name);
  } else if (_mode != defin::DEFAULT && _cur_inst->getRegion() == region) {
    return;
  }

  region->addInst(_cur_inst);
}

void definComponent::source(dbSourceType source)
{
  if (_cur_inst == nullptr) {
    return;
  }

  _cur_inst->setSourceType(source);
}

void definComponent::weight(int weight)
{
  if (_cur_inst == nullptr) {
    return;
  }

  _cur_inst->setWeight(weight);
}

void definComponent::property(const char* name, const char* value)
{
  if (_cur_inst == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_inst, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbStringProperty::create(_cur_inst, name, value);
}

void definComponent::property(const char* name, int value)
{
  if (_cur_inst == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_inst, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbIntProperty::create(_cur_inst, name, value);
}

void definComponent::property(const char* name, double value)
{
  if (_cur_inst == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_inst, name);

  if (p) {
    dbProperty::destroy(p);
  }

  dbDoubleProperty::create(_cur_inst, name, value);
}

void definComponent::end()
{
  if (_cur_inst == nullptr) {
    return;
  }

  dbSet<dbProperty> props = dbProperty::getProperties(_cur_inst);

  if (!props.empty() && props.orderReversed()) {
    props.reverse();
  }

  _cur_inst = nullptr;
}

}  // namespace odb
