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

#include "db.h"
#include "defiUtil.hpp"
#include "definBase.h"

namespace odb {

definBase::definBase()
{
  _tech        = NULL;
  _block       = NULL;
  _errors      = 0;
  _dist_factor = 10;
}

void definBase::init()
{
  _tech        = NULL;
  _block       = NULL;
  _errors      = 0;
  _dist_factor = 10;
}

void definBase::units(int d)
{
  int dbu      = _tech->getDbUnitsPerMicron();
  _dist_factor = dbu / d;
}

void definBase::setTech(dbTech* tech)
{
  _tech        = tech;
  int dbu      = _tech->getDbUnitsPerMicron();
  _dist_factor = dbu / 100;
}

void definBase::setBlock(dbBlock* block)
{
  _block = block;
}

dbOrientType definBase::translate_orientation(int orient)
{
  switch (orient) {
    case DEF_ORIENT_N:
      return dbOrientType::R0;
    case DEF_ORIENT_S:
      return dbOrientType::R180;
    case DEF_ORIENT_E:
      return dbOrientType::R270;
    case DEF_ORIENT_W:
      return dbOrientType::R90;
    case DEF_ORIENT_FN:
      return dbOrientType::MY;
    case DEF_ORIENT_FS:
      return dbOrientType::MX;
    case DEF_ORIENT_FE:
      return dbOrientType::MYR90;
    case DEF_ORIENT_FW:
      return dbOrientType::MXR90;
  }
  assert(0);
  return dbOrientType::R0;
}

}  // namespace odb
