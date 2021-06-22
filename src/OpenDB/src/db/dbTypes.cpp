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

#include "dbTypes.h"

#include <ctype.h>
#include <string.h>

#include "dbId.h"

namespace odb {

//
// dbIdValidation methods here
//
bool dbIdValidation::isId(const char* inid)
{
  if (!inid)
    return false;

  for (; *inid; inid++) {
    if (isdigit(*inid) == 0)
      return false;
  }

  return true;
}

dbOrientType::dbOrientType(const char* orient)
{
  if (strcasecmp(orient, "R0") == 0)
    _value = R0;

  else if (strcasecmp(orient, "R90") == 0)
    _value = R90;

  else if (strcasecmp(orient, "R180") == 0)
    _value = R180;

  else if (strcasecmp(orient, "R270") == 0)
    _value = R270;

  else if (strcasecmp(orient, "MY") == 0)
    _value = MY;

  else if (strcasecmp(orient, "MYR90") == 0)
    _value = MYR90;

  else if (strcasecmp(orient, "MX") == 0)
    _value = MX;

  else if (strcasecmp(orient, "MXR90") == 0)
    _value = MXR90;

  else
    _value = R0;
}

dbOrientType::dbOrientType(Value orient)
{
  _value = orient;
}

dbOrientType::dbOrientType()
{
  _value = R0;
}

dbOrientType::dbOrientType(const dbOrientType& orient)
{
  _value = orient._value;
}

const char* dbOrientType::getString() const
{
  const char* value = "";

  switch (_value) {
    case R0:
      value = "R0";
      break;

    case R90:
      value = "R90";
      break;

    case R180:
      value = "R180";
      break;

    case R270:
      value = "R270";
      break;

    case MY:
      value = "MY";
      break;

    case MYR90:
      value = "MYR90";
      break;

    case MX:
      value = "MX";
      break;

    case MXR90:
      value = "MXR90";
      break;
  }

  return value;
}

dbGroupType::dbGroupType(const char* orient)
{
  if (strcasecmp(orient, "PHYSICAL_CLUSTER") == 0)
    _value = PHYSICAL_CLUSTER;

  else if (strcasecmp(orient, "VOLTAGE_DOMAIN") == 0)
    _value = VOLTAGE_DOMAIN;

  else
    _value = PHYSICAL_CLUSTER;
}

dbGroupType::dbGroupType(Value orient)
{
  _value = orient;
}

dbGroupType::dbGroupType()
{
  _value = PHYSICAL_CLUSTER;
}

dbGroupType::dbGroupType(const dbGroupType& type)
{
  _value = type._value;
}

const char* dbGroupType::getString() const
{
  const char* value = "";

  switch (_value) {
    case PHYSICAL_CLUSTER:
      value = "PHYSICAL_CLUSTER";
      break;

    case VOLTAGE_DOMAIN:
      value = "VOLTAGE_DOMAIN";
      break;
  }

  return value;
}

dbSigType::dbSigType(const char* value)
{
  if (strcasecmp(value, "SIGNAL") == 0)
    _value = SIGNAL;

  else if (strcasecmp(value, "POWER") == 0)
    _value = POWER;

  else if (strcasecmp(value, "GROUND") == 0)
    _value = GROUND;

  else if (strcasecmp(value, "CLOCK") == 0)
    _value = CLOCK;

  else if (strcasecmp(value, "ANALOG") == 0)
    _value = ANALOG;

  else if (strcasecmp(value, "ANALOG") == 0)
    _value = ANALOG;

  else if (strcasecmp(value, "RESET") == 0)
    _value = RESET;

  else if (strcasecmp(value, "SCAN") == 0)
    _value = SCAN;

  else if (strcasecmp(value, "TIEOFF") == 0)
    _value = TIEOFF;

  else
    _value = SIGNAL;
}

dbSigType::dbSigType(Value value)
{
  _value = value;
}

dbSigType::dbSigType()
{
  _value = SIGNAL;
}

dbSigType::dbSigType(const dbSigType& value)
{
  _value = value._value;
}

bool dbSigType::isSupply() const
{
  switch (_value) {
    case POWER:
    case GROUND:
      return true;
    case SIGNAL:
    case CLOCK:
    case ANALOG:
    case RESET:
    case SCAN:
    case TIEOFF:
      return false;
  }
  assert(false);
  return false;
}

const char* dbSigType::getString() const
{
  const char* value = "";

  switch (_value) {
    case SIGNAL:
      value = "SIGNAL";
      break;

    case POWER:
      value = "POWER";
      break;

    case GROUND:
      value = "GROUND";
      break;

    case CLOCK:
      value = "CLOCK";
      break;

    case ANALOG:
      value = "ANALOG";
      break;

    case RESET:
      value = "RESET";
      break;

    case SCAN:
      value = "SCAN";
      break;

    case TIEOFF:
      value = "TIEOFF";
      break;
  }

  return value;
}

dbIoType::dbIoType(const char* value)
{
  if (strcasecmp(value, "INPUT") == 0)
    _value = INPUT;

  else if (strcasecmp(value, "OUTPUT") == 0)
    _value = OUTPUT;

  else if (strcasecmp(value, "INOUT") == 0)
    _value = INOUT;

  else if (strcasecmp(value, "FEEDTHRU") == 0)
    _value = FEEDTHRU;

  else
    _value = INPUT;
}

dbIoType::dbIoType(Value value)
{
  _value = value;
}

dbIoType::dbIoType()
{
  _value = INPUT;
}

dbIoType::dbIoType(const dbIoType& value)
{
  _value = value._value;
}

const char* dbIoType::getString() const
{
  const char* value = "";

  switch (_value) {
    case INPUT:
      value = "INPUT";
      break;

    case OUTPUT:
      value = "OUTPUT";
      break;

    case INOUT:
      value = "INOUT";
      break;

    case FEEDTHRU:
      value = "FEEDTHRU";
      break;
  }

  return value;
}

dbPlacementStatus::dbPlacementStatus(const char* value)
{
  if (strcasecmp(value, "NONE") == 0)
    _value = NONE;

  else if (strcasecmp(value, "UNPLACED") == 0)
    _value = UNPLACED;

  else if (strcasecmp(value, "SUGGESTED") == 0)
    _value = SUGGESTED;

  else if (strcasecmp(value, "PLACED") == 0)
    _value = PLACED;

  else if (strcasecmp(value, "LOCKED") == 0)
    _value = LOCKED;

  else if (strcasecmp(value, "FIRM") == 0)
    _value = FIRM;

  else if (strcasecmp(value, "COVER") == 0)
    _value = COVER;

  else
    _value = NONE;
}

dbPlacementStatus::dbPlacementStatus(Value value)
{
  _value = value;
}

dbPlacementStatus::dbPlacementStatus()
{
  _value = NONE;
}

dbPlacementStatus::dbPlacementStatus(const dbPlacementStatus& value)
{
  _value = value._value;
}

const char* dbPlacementStatus::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case UNPLACED:
      value = "UNPLACED";
      break;

    case SUGGESTED:
      value = "SUGGESTED";
      break;

    case PLACED:
      value = "PLACED";
      break;

    case LOCKED:
      value = "LOCKED";
      break;

    case FIRM:
      value = "FIRM";
      break;

    case COVER:
      value = "COVER";
      break;
  }

  return value;
}

bool dbPlacementStatus::isPlaced() const
{
  switch (_value) {
    case NONE:
    case UNPLACED:
    case SUGGESTED:
      return false;
    case PLACED:
    case LOCKED:
    case FIRM:
    case COVER:
      return true;
  }
  assert(false);
  return false;
}

bool dbPlacementStatus::isFixed() const
{
  switch (_value) {
    case NONE:
    case UNPLACED:
    case SUGGESTED:
    case PLACED:
      return false;
    case LOCKED:
    case FIRM:
    case COVER:
      return true;
  }
  assert(false);
  return false;
}

dbMasterType::dbMasterType(const char* value)
{
  _value = NONE;

  if (strcasecmp(value, "NONE") == 0)
    _value = NONE;

  else if (strcasecmp(value, "COVER") == 0)
    _value = COVER;

  else if (strcasecmp(value, "COVER BUMP") == 0)
    _value = COVER_BUMP;

  else if (strcasecmp(value, "RING") == 0)
    _value = RING;

  else if (strcasecmp(value, "BLOCK") == 0)
    _value = BLOCK;

  else if (strcasecmp(value, "BLOCK BLACKBOX") == 0)
    _value = BLOCK_BLACKBOX;

  else if (strcasecmp(value, "BLOCK SOFT") == 0)
    _value = BLOCK_SOFT;

  else if (strcasecmp(value, "PAD") == 0)
    _value = PAD;

  else if (strcasecmp(value, "PAD INPUT") == 0)
    _value = PAD_INPUT;

  else if (strcasecmp(value, "PAD OUTPUT") == 0)
    _value = PAD_OUTPUT;

  else if (strcasecmp(value, "PAD INOUT") == 0)
    _value = PAD_INOUT;

  else if (strcasecmp(value, "PAD POWER") == 0)
    _value = PAD_POWER;

  else if (strcasecmp(value, "PAD SPACER") == 0)
    _value = PAD_SPACER;

  else if (strcasecmp(value, "PAD AREAIO") == 0)
    _value = PAD_AREAIO;

  else if (strcasecmp(value, "CORE") == 0)
    _value = CORE;

  else if (strcasecmp(value, "CORE FEEDTHRU") == 0)
    _value = CORE_FEEDTHRU;

  else if (strcasecmp(value, "CORE TIEHIGH") == 0)
    _value = CORE_TIEHIGH;

  else if (strcasecmp(value, "CORE TIELOW") == 0)
    _value = CORE_TIELOW;

  else if (strcasecmp(value, "CORE SPACER") == 0)
    _value = CORE_SPACER;

  else if (strcasecmp(value, "CORE ANTENNACELL") == 0)
    _value = CORE_ANTENNACELL;

  else if (strcasecmp(value, "CORE WELLTAP") == 0)
    _value = CORE_WELLTAP;

  else if (strcasecmp(value, "ENDCAP") == 0)
    _value = ENDCAP;

  else if (strcasecmp(value, "ENDCAP PRE") == 0)
    _value = ENDCAP_PRE;

  else if (strcasecmp(value, "ENDCAP POST") == 0)
    _value = ENDCAP_POST;

  else if (strcasecmp(value, "ENDCAP TOPLEFT") == 0)
    _value = ENDCAP_TOPLEFT;

  else if (strcasecmp(value, "ENDCAP TOPRIGHT") == 0)
    _value = ENDCAP_TOPRIGHT;

  else if (strcasecmp(value, "ENDCAP BOTTOMLEFT") == 0)
    _value = ENDCAP_BOTTOMLEFT;

  else if (strcasecmp(value, "ENDCAP BOTTOMRIGHT") == 0)
    _value = ENDCAP_BOTTOMRIGHT;
}

dbMasterType::dbMasterType(Value value)
{
  _value = value;
}

dbMasterType::dbMasterType()
{
  _value = NONE;
}

dbMasterType::dbMasterType(const dbMasterType& value)
{
  _value = value._value;
}

const char* dbMasterType::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case COVER:
      value = "COVER";
      break;

    case COVER_BUMP:
      value = "COVER BUMP";
      break;

    case RING:
      value = "RING";
      break;

    case BLOCK:
      value = "BLOCK";
      break;

    case BLOCK_BLACKBOX:
      value = "BLOCK BLACKBOX";
      break;

    case BLOCK_SOFT:
      value = "BLOCK SOFT";
      break;

    case PAD:
      value = "PAD";
      break;

    case PAD_INPUT:
      value = "PAD INPUT";
      break;

    case PAD_OUTPUT:
      value = "PAD OUTPUT";
      break;

    case PAD_INOUT:
      value = "PAD INOUT";
      break;

    case PAD_POWER:
      value = "PAD POWER";
      break;

    case PAD_SPACER:
      value = "PAD SPACER";
      break;

    case PAD_AREAIO:
      value = "PAD AREAIO";
      break;

    case CORE:
      value = "CORE";
      break;

    case CORE_FEEDTHRU:
      value = "CORE FEEDTHRU";
      break;

    case CORE_TIEHIGH:
      value = "CORE TIEHIGH";
      break;

    case CORE_TIELOW:
      value = "CORE TIELOW";
      break;

    case CORE_SPACER:
      value = "CORE SPACER";
      break;

    case CORE_ANTENNACELL:
      value = "CORE ANTENNACELL";
      break;

    case CORE_WELLTAP:
      value = "CORE WELLTAP";
      break;

    case ENDCAP:
      value = "ENDCAP";
      break;

    case ENDCAP_PRE:
      value = "ENDCAP PRE";
      break;

    case ENDCAP_POST:
      value = "ENDCAP POST";
      break;

    case ENDCAP_TOPLEFT:
      value = "ENDCAP TOPLEFT";
      break;

    case ENDCAP_TOPRIGHT:
      value = "ENDCAP TOPRIGHT";
      break;

    case ENDCAP_BOTTOMLEFT:
      value = "ENDCAP BOTTOMLEFT";
      break;

    case ENDCAP_BOTTOMRIGHT:
      value = "ENDCAP BOTTOMRIGHT";
      break;
  }

  return value;
}

bool dbMasterType::isBlock() const
{
  switch (_value) {
    case BLOCK:
    case BLOCK_BLACKBOX:
    case BLOCK_SOFT:
      return true;
    case NONE:
    case COVER:
    case COVER_BUMP:
    case RING:
    case PAD:
    case PAD_INPUT:
    case PAD_OUTPUT:
    case PAD_INOUT:
    case PAD_POWER:
    case PAD_SPACER:
    case PAD_AREAIO:
    case CORE:
    case CORE_FEEDTHRU:
    case CORE_TIEHIGH:
    case CORE_TIELOW:
    case CORE_SPACER:
    case CORE_ANTENNACELL:
    case CORE_WELLTAP:
    case ENDCAP:
    case ENDCAP_PRE:
    case ENDCAP_POST:
    case ENDCAP_TOPLEFT:
    case ENDCAP_TOPRIGHT:
    case ENDCAP_BOTTOMLEFT:
    case ENDCAP_BOTTOMRIGHT:
      return false;
  }
  assert(false);
  return false;
}

bool dbMasterType::isCore() const
{
  switch (_value) {
    case CORE:
    case CORE_FEEDTHRU:
    case CORE_TIEHIGH:
    case CORE_TIELOW:
    case CORE_SPACER:
    case CORE_ANTENNACELL:
    case CORE_WELLTAP:
      return true;
    case NONE:
    case COVER:
    case COVER_BUMP:
    case RING:
    case BLOCK:
    case BLOCK_BLACKBOX:
    case BLOCK_SOFT:
    case PAD:
    case PAD_INPUT:
    case PAD_OUTPUT:
    case PAD_INOUT:
    case PAD_POWER:
    case PAD_SPACER:
    case PAD_AREAIO:
    case ENDCAP:
    case ENDCAP_PRE:
    case ENDCAP_POST:
    case ENDCAP_TOPLEFT:
    case ENDCAP_TOPRIGHT:
    case ENDCAP_BOTTOMLEFT:
    case ENDCAP_BOTTOMRIGHT:
      return false;
  }
  assert(false);
  return false;
}

bool dbMasterType::isPad() const
{
  switch (_value) {
    case PAD:
    case PAD_INPUT:
    case PAD_OUTPUT:
    case PAD_INOUT:
    case PAD_POWER:
    case PAD_SPACER:
    case PAD_AREAIO:
      return true;
    case NONE:
    case COVER:
    case COVER_BUMP:
    case RING:
    case BLOCK:
    case BLOCK_BLACKBOX:
    case BLOCK_SOFT:
    case CORE:
    case CORE_FEEDTHRU:
    case CORE_TIEHIGH:
    case CORE_TIELOW:
    case CORE_SPACER:
    case CORE_ANTENNACELL:
    case CORE_WELLTAP:
    case ENDCAP:
    case ENDCAP_PRE:
    case ENDCAP_POST:
    case ENDCAP_TOPLEFT:
    case ENDCAP_TOPRIGHT:
    case ENDCAP_BOTTOMLEFT:
    case ENDCAP_BOTTOMRIGHT:
      return false;
  }
  assert(false);
  return false;
}

bool dbMasterType::isEndCap() const
{
  switch (_value) {
    case ENDCAP:
    case ENDCAP_PRE:
    case ENDCAP_POST:
    case ENDCAP_TOPLEFT:
    case ENDCAP_TOPRIGHT:
    case ENDCAP_BOTTOMLEFT:
    case ENDCAP_BOTTOMRIGHT:
      return true;
    case NONE:
    case COVER:
    case COVER_BUMP:
    case RING:
    case BLOCK:
    case BLOCK_BLACKBOX:
    case BLOCK_SOFT:
    case PAD:
    case PAD_INPUT:
    case PAD_OUTPUT:
    case PAD_INOUT:
    case PAD_POWER:
    case PAD_SPACER:
    case PAD_AREAIO:
    case CORE:
    case CORE_FEEDTHRU:
    case CORE_TIEHIGH:
    case CORE_TIELOW:
    case CORE_SPACER:
    case CORE_ANTENNACELL:
    case CORE_WELLTAP:
      return false;
  }
  assert(false);
  return false;
}

dbTechLayerType::dbTechLayerType(const char* value)
{
  if (strcasecmp(value, "ROUTING") == 0)
    _value = ROUTING;

  else if (strcasecmp(value, "CUT") == 0)
    _value = CUT;

  else if (strcasecmp(value, "MASTERSLICE") == 0)
    _value = MASTERSLICE;

  else if (strcasecmp(value, "OVERLAP") == 0)
    _value = OVERLAP;

  else if (strcasecmp(value, "IMPLANT") == 0)
    _value = IMPLANT;

  else
    _value = NONE;
}

dbTechLayerType::dbTechLayerType(Value value)
{
  _value = value;
}

dbTechLayerType::dbTechLayerType()
{
  _value = ROUTING;
}

dbTechLayerType::dbTechLayerType(const dbTechLayerType& value)
{
  _value = value._value;
}

const char* dbTechLayerType::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case ROUTING:
      value = "ROUTING";
      break;

    case CUT:
      value = "CUT";
      break;

    case MASTERSLICE:
      value = "MASTERSLICE";
      break;

    case OVERLAP:
      value = "OVERLAP";
      break;

    case IMPLANT:
      value = "IMPLANT";
      break;
  }

  return value;
}

dbTechLayerDir::dbTechLayerDir(const char* value)
{
  if (strcasecmp(value, "NONE") == 0)
    _value = NONE;

  else if (strcasecmp(value, "HORIZONTAL") == 0)
    _value = HORIZONTAL;

  else if (strcasecmp(value, "VERTICAL") == 0)
    _value = VERTICAL;

  else
    _value = NONE;
}

dbTechLayerDir::dbTechLayerDir(Value value)
{
  _value = value;
}

dbTechLayerDir::dbTechLayerDir()
{
  _value = NONE;
}

dbTechLayerDir::dbTechLayerDir(const dbTechLayerDir& value)
{
  _value = value._value;
}

const char* dbTechLayerDir::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case HORIZONTAL:
      value = "HORIZONTAL";
      break;

    case VERTICAL:
      value = "VERTICAL";
      break;
  }

  return value;
}

dbTechLayerMinStepType::dbTechLayerMinStepType(const char* value)
{
  if (strcasecmp(value, "INSIDECORNER") == 0)
    _value = INSIDE_CORNER;

  else if (strcasecmp(value, "OUTSIDECORNER") == 0)
    _value = OUTSIDE_CORNER;

  else if (strcasecmp(value, "STEP") == 0)
    _value = STEP;

  else
    _value = OUTSIDE_CORNER;
}

dbTechLayerMinStepType::dbTechLayerMinStepType(Value value)
{
  _value = value;
}

dbTechLayerMinStepType::dbTechLayerMinStepType()
{
  _value = OUTSIDE_CORNER;
}

dbTechLayerMinStepType::dbTechLayerMinStepType(
    const dbTechLayerMinStepType& value)
{
  _value = value._value;
}

const char* dbTechLayerMinStepType::getString() const
{
  const char* value = "";

  switch (_value) {
    case INSIDE_CORNER:
      value = "INSIDECORNER";
      break;

    case OUTSIDE_CORNER:
      value = "OUTSIDECORNER";
      break;

    case STEP:
      value = "STEP";
      break;
  }

  return value;
}

dbBoxOwner::dbBoxOwner(const char* value)
{
  if (strcasecmp(value, "UNKNOWN") == 0)
    _value = UNKNOWN;

  else if (strcasecmp(value, "BLOCK") == 0)
    _value = BLOCK;

  else if (strcasecmp(value, "INST") == 0)
    _value = INST;

  else if (strcasecmp(value, "BTERM") == 0)
    _value = BTERM;

  else if (strcasecmp(value, "BPIN") == 0)
    _value = BPIN;

  else if (strcasecmp(value, "BPIN") == 0)
    _value = BPIN;

  else if (strcasecmp(value, "VIA") == 0)
    _value = VIA;

  else if (strcasecmp(value, "OBSTRUCTION") == 0)
    _value = OBSTRUCTION;

  else if (strcasecmp(value, "BLOCKAGE") == 0)
    _value = BLOCKAGE;

  else if (strcasecmp(value, "MASTER") == 0)
    _value = MASTER;

  else if (strcasecmp(value, "MPIN") == 0)
    _value = MPIN;

  else if (strcasecmp(value, "TECH VIA") == 0)
    _value = TECH_VIA;

  else if (strcasecmp(value, "SWIRE") == 0)
    _value = SWIRE;
  else if (strcasecmp(value, "REGION") == 0)
    _value = REGION;
}

dbBoxOwner::dbBoxOwner(Value value)
{
  _value = value;
}

dbBoxOwner::dbBoxOwner()
{
  _value = BLOCK;
}

dbBoxOwner::dbBoxOwner(const dbBoxOwner& value)
{
  _value = value._value;
}

const char* dbBoxOwner::getString() const
{
  const char* value = "";

  switch (_value) {
    case UNKNOWN:
      value = "UNKNOWN";
      break;

    case BLOCK:
      value = "BLOCK";
      break;

    case INST:
      value = "INST";
      break;

    case BTERM:
      value = "BTERM";
      break;

    case BPIN:
      value = "BPIN";
      break;

    case VIA:
      value = "VIA";
      break;

    case OBSTRUCTION:
      value = "OBSTRUCTION";
      break;

    case BLOCKAGE:
      value = "BLOCKAGE";
      break;

    case MASTER:
      value = "MASTER";
      break;

    case MPIN:
      value = "MPIN";
      break;

    case TECH_VIA:
      value = "TECH_VIA";
      break;

    case SWIRE:
      value = "SWIRE";
      break;

    case REGION:
      value = "REGION";
      break;
  }

  return value;
}

dbWireType::dbWireType(const char* value)
{
  _value = NONE;
  if (strcasecmp(value, "NONE") == 0)
    _value = NONE;

  else if (strcasecmp(value, "COVER") == 0)
    _value = COVER;

  else if (strcasecmp(value, "FIXED") == 0)
    _value = FIXED;

  else if (strcasecmp(value, "ROUTED") == 0)
    _value = ROUTED;

  else if (strcasecmp(value, "SHIELD") == 0)
    _value = SHIELD;

  else if (strcasecmp(value, "NOSHIELD") == 0)
    _value = NOSHIELD;
}

dbWireType::dbWireType(Value value)
{
  _value = value;
}

dbWireType::dbWireType()
{
  _value = NONE;
}

dbWireType::dbWireType(const dbWireType& value)
{
  _value = value._value;
}

const char* dbWireType::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case COVER:
      value = "COVER";
      break;

    case FIXED:
      value = "FIXED";
      break;

    case ROUTED:
      value = "ROUTED";
      break;

    case SHIELD:
      value = "SHIELD";
      break;

    case NOSHIELD:
      value = "NOSHIELD";
      break;
  }

  return value;
}

dbWireShapeType::dbWireShapeType(const char* value)
{
  if (strcasecmp(value, "NONE") == 0)
    _value = NONE;

  else if (strcasecmp(value, "RING") == 0)
    _value = RING;

  else if (strcasecmp(value, "PADRING") == 0)
    _value = PADRING;

  else if (strcasecmp(value, "BLOCKRING") == 0)
    _value = BLOCKRING;

  else if (strcasecmp(value, "STRIPE") == 0)
    _value = STRIPE;

  else if (strcasecmp(value, "FOLLOWPIN") == 0)
    _value = FOLLOWPIN;

  else if (strcasecmp(value, "IOWIRE") == 0)
    _value = IOWIRE;

  else if (strcasecmp(value, "COREWIRE") == 0)
    _value = COREWIRE;

  else if (strcasecmp(value, "BLOCKWIRE") == 0)
    _value = BLOCKWIRE;

  else if (strcasecmp(value, "BLOCKAGEWIRE") == 0)
    _value = BLOCKAGEWIRE;

  else if (strcasecmp(value, "FILLWIRE") == 0)
    _value = FILLWIRE;

  else if (strcasecmp(value, "DRCFILL") == 0)
    _value = DRCFILL;
}

dbWireShapeType::dbWireShapeType(Value value)
{
  _value = value;
}

dbWireShapeType::dbWireShapeType()
{
  _value = NONE;
}

dbWireShapeType::dbWireShapeType(const dbWireShapeType& value)
{
  _value = value._value;
}

const char* dbWireShapeType::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case RING:
      value = "RING";
      break;

    case PADRING:
      value = "PADRING";
      break;

    case BLOCKRING:
      value = "BLOCKRING";
      break;

    case STRIPE:
      value = "STRIPE";
      break;

    case FOLLOWPIN:
      value = "FOLLOWPIN";
      break;

    case IOWIRE:
      value = "IOWIRE";
      break;

    case COREWIRE:
      value = "COREWIRE";
      break;

    case BLOCKWIRE:
      value = "BLOCKWIRE";
      break;

    case BLOCKAGEWIRE:
      value = "BLOCKAGEWIRE";
      break;

    case FILLWIRE:
      value = "FILLWIRE";
      break;

    case DRCFILL:
      value = "DRCFILL";
      break;
  }

  return value;
}

dbSiteClass::dbSiteClass(const char* value)
{
  if (strcasecmp(value, "NONE") == 0)
    _value = NONE;

  else if (strcasecmp(value, "PAD") == 0)
    _value = PAD;

  else if (strcasecmp(value, "CORE") == 0)
    _value = CORE;
}

dbSiteClass::dbSiteClass(Value value)
{
  _value = value;
}

dbSiteClass::dbSiteClass()
{
  _value = NONE;
}

dbSiteClass::dbSiteClass(const dbSiteClass& value)
{
  _value = value._value;
}

const char* dbSiteClass::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case PAD:
      value = "PAD";
      break;

    case CORE:
      value = "CORE";
      break;
  }

  return value;
}

///
/// dbOnOffType methods here
///
dbOnOffType::dbOnOffType(const char* instr)
{
  if (strcasecmp(instr, "ON") == 0)
    _value = ON;
  else if (strcasecmp(instr, "OFF") == 0)
    _value = OFF;
  else
    _value = OFF;
}

dbOnOffType::dbOnOffType(Value inval)
{
  _value = inval;
}

dbOnOffType::dbOnOffType(int innum)
{
  _value = (innum == 0) ? OFF : ON;
}

dbOnOffType::dbOnOffType(bool insw)
{
  _value = (insw) ? ON : OFF;
}

dbOnOffType::dbOnOffType()
{
  _value = OFF;
}

const char* dbOnOffType::getString() const
{
  const char* value = "";

  switch (_value) {
    case OFF:
      value = "OFF";
      break;

    case ON:
      value = "ON";
      break;
  }

  return value;
}

int dbOnOffType::getAsInt() const
{
  return (_value == ON) ? 1 : 0;
}

bool dbOnOffType::isSet() const
{
  return (_value == ON);
}

///
/// dbClMeasureType methods here
///
dbClMeasureType::dbClMeasureType(const char* instr)
{
  if (strcasecmp(instr, "MAXXY") == 0)
    _value = MAXXY;
  else if (strcasecmp(instr, "EUCLIDEAN") == 0)
    _value = EUCLIDEAN;
  else
    _value = EUCLIDEAN;
}

const char* dbClMeasureType::getString() const
{
  const char* value = "";

  switch (_value) {
    case EUCLIDEAN:
      value = "EUCLIDEAN";
      break;

    case MAXXY:
      value = "MAXXY";
      break;
  }

  return value;
}

//
//  dbRowDir methods here
//
dbRowDir::dbRowDir(const char* value)
{
  if (strcasecmp(value, "HORIZONTAL") == 0)
    _value = HORIZONTAL;

  else
    _value = VERTICAL;
}

dbRowDir::dbRowDir(Value value)
{
  _value = value;
}

dbRowDir::dbRowDir()
{
  _value = HORIZONTAL;
}

dbRowDir::dbRowDir(const dbRowDir& value)
{
  _value = value._value;
}

const char* dbRowDir::getString() const
{
  const char* value = "";

  switch (_value) {
    case HORIZONTAL:
      value = "HORIZONTAL";
      break;

    case VERTICAL:
      value = "VERTICAL";
      break;
  }

  return value;
}

const char* dbRegionType::getString() const
{
  const char* value = "";

  switch (_value) {
    case INCLUSIVE:
      value = "INCLUSIVE";
      break;

    case EXCLUSIVE:
      value = "EXCLUSIVE";
      break;

    case SUGGESTED:
      value = "SUGGESTED";
      break;
  }

  return value;
}

dbSourceType::dbSourceType(const char* value)
{
  if (strcasecmp(value, "NETLIST") == 0)
    _value = NETLIST;

  else if (strcasecmp(value, "DIST") == 0)
    _value = DIST;

  else if (strcasecmp(value, "USER") == 0)
    _value = USER;

  else if (strcasecmp(value, "TIMING") == 0)
    _value = TIMING;

  else
    _value = NETLIST;
}

const char* dbSourceType::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case NETLIST:
      value = "NETLIST";
      break;

    case DIST:
      value = "DIST";
      break;

    case USER:
      value = "USER";
      break;

    case TIMING:
      value = "TIMING";
      break;

    case TEST:
      value = "TEST";
      break;
  }

  return value;
}

//
//  dbJournalEntryType methods here
//
const char* dbJournalEntryType::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case OWNER:
      value = "OWNER";
      break;

    case ADD:
      value = "ADD";
      break;

    case DESTROY:
      value = "DESTROY";
      break;
  }

  return value;
}

//
//  dbDirection methods here
//
const char* dbDirection::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case NORTH:
      value = "NORTH";
      break;

    case EAST:
      value = "EAST";
      break;

    case SOUTH:
      value = "SOUTH";
      break;

    case WEST:
      value = "WEST";
      break;
  }

  return value;
}

}  // namespace odb
