// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "definGroup.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "odb/db.h"
#include "utl/Logger.h"
namespace odb {

definGroup::definGroup() : cur_group_(nullptr)
{
  init();
}

definGroup::~definGroup()
{
}

void definGroup::init()
{
  definBase::init();
}

void definGroup::begin(const char* name)
{
  cur_group_ = _block->findGroup(name);
  std::string group_name(name);

  if (cur_group_) {
    _logger->warn(utl::ODB, 304, "Group \"{}\" already exists", group_name);
    ++_errors;
    cur_group_ = nullptr;
    return;
  }
  cur_group_ = dbGroup::create(_block, group_name.c_str());
}

void definGroup::region(const char* region_name)
{
  if (cur_group_ != nullptr) {
    auto region = _block->findRegion(region_name);
    if (region == nullptr) {
      _logger->warn(utl::ODB, 305, "Region \"{}\" is not found", region_name);
      ++_errors;
      return;
    }
    region->addGroup(cur_group_);
  }
}

void definGroup::inst(const char* name)
{
  if (cur_group_ != nullptr) {
    std::string pname = name;
    size_t pname_length = pname.length();
    if (pname[pname.length() - 1] == '*') {
      size_t prefix_length = pname_length - 1;
      std::string prefix = pname.substr(0, pname_length - 1);
      for (dbInst* inst : _block->getInsts()) {
        const char* inst_name = inst->getConstName();
        if (strncmp(inst_name, prefix.c_str(), prefix_length) == 0) {
          cur_group_->addInst(inst);
        }
      }
    } else {
      dbInst* inst = _block->findInst(name);
      if (inst == nullptr) {
        _logger->warn(utl::ODB,
                      306,
                      "error: netlist component ({}) is not defined",
                      name);
        ++_errors;
        return;
      }
      cur_group_->addInst(inst);
    }
  }
}

void definGroup::property(const char* name, const char* value)
{
  if (cur_group_ == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(cur_group_, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbStringProperty::create(cur_group_, name, value);
}

void definGroup::property(const char* name, int value)
{
  if (cur_group_ == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(cur_group_, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbIntProperty::create(cur_group_, name, value);
}

void definGroup::property(const char* name, double value)
{
  if (cur_group_ == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(cur_group_, name);

  if (p) {
    dbProperty::destroy(p);
  }

  dbDoubleProperty::create(cur_group_, name, value);
}

void definGroup::end()
{
  if (cur_group_ != nullptr) {
    dbSet<dbProperty> props = dbProperty::getProperties(cur_group_);

    if (!props.empty() && props.orderReversed()) {
      props.reverse();
    }
  }
}

}  // namespace odb
