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

#include "definRegion.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

definRegion::definRegion()
{
  init();
}

definRegion::~definRegion()
{
}

void definRegion::init()
{
  definBase::init();
}

void definRegion::begin(const char* name)
{
  _cur_region = _block->findRegion(name);
  std::string region_name(name);

  if (_cur_region) {
    _logger->warn(utl::ODB, 152, "Region \"{}\" already exists", name);
    ++_errors;
    _cur_region = nullptr;
    return;
  }

  _cur_region = dbRegion::create(_block, region_name.c_str());
}

void definRegion::boundary(int x1, int y1, int x2, int y2)
{
  if (_cur_region) {
    dbBox::create(_cur_region, dbdist(x1), dbdist(y1), dbdist(x2), dbdist(y2));
  }
}

void definRegion::type(defRegionType type)
{
  if (_cur_region) {
    if (type == DEF_GUIDE) {
      _cur_region->setRegionType(dbRegionType(dbRegionType::SUGGESTED));

    } else if (type == DEF_FENCE) {
      _cur_region->setRegionType(dbRegionType(dbRegionType::EXCLUSIVE));
    }
  }
}

void definRegion::property(const char* name, const char* value)
{
  if (_cur_region == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_region, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbStringProperty::create(_cur_region, name, value);
}

void definRegion::property(const char* name, int value)
{
  if (_cur_region == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_region, name);
  if (p) {
    dbProperty::destroy(p);
  }

  dbIntProperty::create(_cur_region, name, value);
}

void definRegion::property(const char* name, double value)
{
  if (_cur_region == nullptr) {
    return;
  }

  dbProperty* p = dbProperty::find(_cur_region, name);

  if (p) {
    dbProperty::destroy(p);
  }

  dbDoubleProperty::create(_cur_region, name, value);
}

void definRegion::end()
{
  if (_cur_region) {
    dbSet<dbBox> boxes = _cur_region->getBoundaries();

    if (boxes.reversible() && boxes.orderReversed()) {
      boxes.reverse();
    }

    dbSet<dbProperty> props = dbProperty::getProperties(_cur_region);

    if (!props.empty() && props.orderReversed()) {
      props.reverse();
    }
  }
}

}  // namespace odb
