/* Authors: Osama Hammad */
/*
 * Copyright (c) 2022, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
