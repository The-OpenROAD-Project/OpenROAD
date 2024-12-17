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

#include "odb/dbTypes.h"

#include <cctype>
#include <cstring>

#include "odb/dbId.h"

namespace odb {

//
// dbIdValidation methods here
//
bool dbIdValidation::isId(const char* inid)
{
  if (!inid) {
    return false;
  }

  for (; *inid; inid++) {
    if (isdigit(*inid) == 0) {
      return false;
    }
  }

  return true;
}

std::optional<dbOrientType::Value> dbOrientType::fromString(const char* orient)
{
  std::optional<dbOrientType::Value> ret;
  if (strcasecmp(orient, "R0") == 0) {
    ret = R0;
  } else if (strcasecmp(orient, "R90") == 0) {
    ret = R90;
  } else if (strcasecmp(orient, "R180") == 0) {
    ret = R180;
  } else if (strcasecmp(orient, "R270") == 0) {
    ret = R270;
  } else if (strcasecmp(orient, "MY") == 0) {
    ret = MY;
  } else if (strcasecmp(orient, "MYR90") == 0) {
    ret = MYR90;
  } else if (strcasecmp(orient, "MX") == 0) {
    ret = MX;
  } else if (strcasecmp(orient, "MXR90") == 0) {
    ret = MXR90;
  } else if (strcasecmp(orient, "N") == 0) {  // LEF/DEF style names
    ret = R0;
  } else if (strcasecmp(orient, "W") == 0) {
    ret = R90;
  } else if (strcasecmp(orient, "S") == 0) {
    ret = R180;
  } else if (strcasecmp(orient, "E") == 0) {
    ret = R270;
  } else if (strcasecmp(orient, "FN") == 0) {
    ret = MY;
  } else if (strcasecmp(orient, "FE") == 0) {
    ret = MYR90;
  } else if (strcasecmp(orient, "FS") == 0) {
    ret = MX;
  } else if (strcasecmp(orient, "FW") == 0) {
    ret = MXR90;
  } else {
    ret = {};
  }
  return ret;
}

const dbOrientType::Value dbOrientType::DEFAULT;

dbOrientType::dbOrientType(const char* orient)
{
  auto opt = fromString(orient);
  _value = opt.value_or(dbOrientType::DEFAULT);
}

dbOrientType::dbOrientType(Value orient)
{
  _value = orient;
}

dbOrientType::dbOrientType()
{
  _value = dbOrientType::DEFAULT;
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

dbOrientType dbOrientType::flipX() const
{
  switch (_value) {
    case R0:
      return MX;
    case R90:
      return MYR90;
    case R180:
      return MY;
    case R270:
      return MXR90;
    case MY:
      return R180;
    case MYR90:
      return R90;
    case MX:
      return R0;
    case MXR90:
      return R270;
  }
  return R0;
}

dbOrientType dbOrientType::flipY() const
{
  switch (_value) {
    case R0:
      return MY;
    case R90:
      return MXR90;
    case R180:
      return MX;
    case R270:
      return MYR90;
    case MY:
      return R0;
    case MYR90:
      return R270;
    case MX:
      return R180;
    case MXR90:
      return R90;
  }
  return R0;
}

bool dbOrientType::isRightAngleRotation() const
{
  switch (_value) {
    case R90:
    case R270:
    case MYR90:
    case MXR90:
      return true;
    case R0:
    case R180:
    case MY:
    case MX:
      return false;
  }
  assert(false);
  return false;
}

dbGDSSTrans::dbGDSSTrans()
{
  _flipX = false;
  _absMag = false;
  _absAngle = false;
  _mag = 1.0;
  _angle = 0.0;
}

dbGDSSTrans::dbGDSSTrans(bool flipX,
                         bool absMag,
                         bool absAngle,
                         double mag,
                         double angle)
{
  _flipX = flipX;
  _absMag = absMag;
  _absAngle = absAngle;
  _mag = mag;
  _angle = angle;
}

bool dbGDSSTrans::operator==(const dbGDSSTrans& rhs) const
{
  return (_flipX == rhs._flipX) && (_absMag == rhs._absMag)
         && (_absAngle == rhs._absAngle) && (_mag == rhs._mag)
         && (_angle == rhs._angle);
}

std::string dbGDSSTrans::to_string() const
{
  std::string s;
  if (_flipX) {
    s += std::string("FLIP_X ");
  }
  s += (_absMag) ? std::string("ABS_MAG ") : std::string("MAG ");
  s += std::to_string(_mag) + " ";
  s += (_absAngle) ? std::string("ABS_ANGLE ") : std::string("ANGLE ");
  s += std::to_string(_angle);
  s += " ";
  return s;
}

bool dbGDSSTrans::identity() const
{
  return (!_flipX) && (!_absMag) && (!_absAngle) && (_mag == 1.0)
         && (_angle == 0.0);
}

dbGDSTextPres::dbGDSTextPres()
{
  _vPres = dbGDSTextPres::VPres::TOP;
  _hPres = dbGDSTextPres::HPres::LEFT;
}

dbGDSTextPres::dbGDSTextPres(dbGDSTextPres::VPres vPres,
                             dbGDSTextPres::HPres hPres)
{
  _vPres = vPres;
  _hPres = hPres;
}

bool dbGDSTextPres::operator==(const dbGDSTextPres& rhs) const
{
  return (_vPres == rhs._vPres) && (_hPres == rhs._hPres);
}

std::string dbGDSTextPres::to_string() const
{
  std::string s;
  s += "FONT ";
  s += (_vPres == dbGDSTextPres::VPres::TOP) ? std::string("TOP ")
                                             : std::string("BOTTOM ");
  s += (_hPres == dbGDSTextPres::HPres::LEFT) ? std::string("LEFT ")
                                              : std::string("RIGHT ");
  return s;
}

dbIStream& operator>>(dbIStream& stream, dbGDSSTrans& t)
{
  stream >> t._flipX;
  stream >> t._absMag;
  stream >> t._absAngle;
  stream >> t._mag;
  stream >> t._angle;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const dbGDSSTrans t)
{
  stream << t._flipX;
  stream << t._absMag;
  stream << t._absAngle;
  stream << t._mag;
  stream << t._angle;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, dbGDSTextPres& t)
{
  uint8_t vPresTemp, hPresTemp;
  stream >> vPresTemp;
  stream >> hPresTemp;
  t._vPres = static_cast<dbGDSTextPres::VPres>(vPresTemp);
  t._hPres = static_cast<dbGDSTextPres::HPres>(hPresTemp);
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const dbGDSTextPres t)
{
  stream << static_cast<uint8_t>(t._vPres);
  stream << static_cast<uint8_t>(t._hPres);
  return stream;
}

dbGroupType::dbGroupType(const char* orient)
{
  if (strcasecmp(orient, "PHYSICAL_CLUSTER") == 0) {
    _value = PHYSICAL_CLUSTER;

  } else if (strcasecmp(orient, "VOLTAGE_DOMAIN") == 0) {
    _value = VOLTAGE_DOMAIN;

  } else if (strcasecmp(orient, "POWER_DOMAIN") == 0) {
    _value = POWER_DOMAIN;

  } else {
    _value = PHYSICAL_CLUSTER;
  }
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

    case POWER_DOMAIN:
      value = "POWER_DOMAIN";
      break;
  }

  return value;
}

dbSigType::dbSigType(const char* value)
{
  if (strcasecmp(value, "SIGNAL") == 0) {
    _value = SIGNAL;

  } else if (strcasecmp(value, "POWER") == 0) {
    _value = POWER;

  } else if (strcasecmp(value, "GROUND") == 0) {
    _value = GROUND;

  } else if (strcasecmp(value, "CLOCK") == 0) {
    _value = CLOCK;

  } else if (strcasecmp(value, "ANALOG") == 0) {
    _value = ANALOG;

  } else if (strcasecmp(value, "RESET") == 0) {
    _value = RESET;

  } else if (strcasecmp(value, "SCAN") == 0) {
    _value = SCAN;

  } else if (strcasecmp(value, "TIEOFF") == 0) {
    _value = TIEOFF;

  } else {
    _value = SIGNAL;
  }
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
  if (strcasecmp(value, "INPUT") == 0) {
    _value = INPUT;

  } else if (strcasecmp(value, "OUTPUT") == 0) {
    _value = OUTPUT;

  } else if (strcasecmp(value, "INOUT") == 0) {
    _value = INOUT;

  } else if (strcasecmp(value, "FEEDTHRU") == 0) {
    _value = FEEDTHRU;

  } else {
    _value = INPUT;
  }
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
  if (strcasecmp(value, "NONE") == 0) {
    _value = NONE;

  } else if (strcasecmp(value, "UNPLACED") == 0) {
    _value = UNPLACED;

  } else if (strcasecmp(value, "SUGGESTED") == 0) {
    _value = SUGGESTED;

  } else if (strcasecmp(value, "PLACED") == 0) {
    _value = PLACED;

  } else if (strcasecmp(value, "LOCKED") == 0) {
    _value = LOCKED;

  } else if (strcasecmp(value, "FIRM") == 0) {
    _value = FIRM;

  } else if (strcasecmp(value, "COVER") == 0) {
    _value = COVER;

  } else {
    _value = NONE;
  }
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
  _value = CORE;

  if (strcasecmp(value, "COVER") == 0) {
    _value = COVER;

  } else if (strcasecmp(value, "COVER BUMP") == 0) {
    _value = COVER_BUMP;

  } else if (strcasecmp(value, "RING") == 0) {
    _value = RING;

  } else if (strcasecmp(value, "BLOCK") == 0) {
    _value = BLOCK;

  } else if (strcasecmp(value, "BLOCK BLACKBOX") == 0) {
    _value = BLOCK_BLACKBOX;

  } else if (strcasecmp(value, "BLOCK SOFT") == 0) {
    _value = BLOCK_SOFT;

  } else if (strcasecmp(value, "PAD") == 0) {
    _value = PAD;

  } else if (strcasecmp(value, "PAD INPUT") == 0) {
    _value = PAD_INPUT;

  } else if (strcasecmp(value, "PAD OUTPUT") == 0) {
    _value = PAD_OUTPUT;

  } else if (strcasecmp(value, "PAD INOUT") == 0) {
    _value = PAD_INOUT;

  } else if (strcasecmp(value, "PAD POWER") == 0) {
    _value = PAD_POWER;

  } else if (strcasecmp(value, "PAD SPACER") == 0) {
    _value = PAD_SPACER;

  } else if (strcasecmp(value, "PAD AREAIO") == 0) {
    _value = PAD_AREAIO;

  } else if (strcasecmp(value, "CORE") == 0) {
    _value = CORE;

  } else if (strcasecmp(value, "CORE FEEDTHRU") == 0) {
    _value = CORE_FEEDTHRU;

  } else if (strcasecmp(value, "CORE TIEHIGH") == 0) {
    _value = CORE_TIEHIGH;

  } else if (strcasecmp(value, "CORE TIELOW") == 0) {
    _value = CORE_TIELOW;

  } else if (strcasecmp(value, "CORE SPACER") == 0) {
    _value = CORE_SPACER;

  } else if (strcasecmp(value, "CORE ANTENNACELL") == 0) {
    _value = CORE_ANTENNACELL;

  } else if (strcasecmp(value, "CORE WELLTAP") == 0) {
    _value = CORE_WELLTAP;

  } else if (strcasecmp(value, "ENDCAP") == 0) {
    _value = ENDCAP;

  } else if (strcasecmp(value, "ENDCAP PRE") == 0) {
    _value = ENDCAP_PRE;

  } else if (strcasecmp(value, "ENDCAP POST") == 0) {
    _value = ENDCAP_POST;

  } else if (strcasecmp(value, "ENDCAP TOPLEFT") == 0) {
    _value = ENDCAP_TOPLEFT;

  } else if (strcasecmp(value, "ENDCAP TOPRIGHT") == 0) {
    _value = ENDCAP_TOPRIGHT;

  } else if (strcasecmp(value, "ENDCAP BOTTOMLEFT") == 0) {
    _value = ENDCAP_BOTTOMLEFT;

  } else if (strcasecmp(value, "ENDCAP BOTTOMRIGHT") == 0) {
    _value = ENDCAP_BOTTOMRIGHT;

  } else if (strcasecmp(value, "ENDCAP BOTTOMEDGE") == 0) {
    _value = ENDCAP_LEF58_BOTTOMEDGE;

  } else if (strcasecmp(value, "ENDCAP TOPEDGE") == 0) {
    _value = ENDCAP_LEF58_TOPEDGE;

  } else if (strcasecmp(value, "ENDCAP RIGHTEDGE") == 0) {
    _value = ENDCAP_LEF58_RIGHTEDGE;

  } else if (strcasecmp(value, "ENDCAP LEFTEDGE") == 0) {
    _value = ENDCAP_LEF58_LEFTEDGE;

  } else if (strcasecmp(value, "ENDCAP RIGHTBOTTOMEDGE") == 0) {
    _value = ENDCAP_LEF58_RIGHTBOTTOMEDGE;

  } else if (strcasecmp(value, "ENDCAP LEFTBOTTOMEDGE") == 0) {
    _value = ENDCAP_LEF58_LEFTBOTTOMEDGE;

  } else if (strcasecmp(value, "ENDCAP RIGHTTOPEDGE") == 0) {
    _value = ENDCAP_LEF58_RIGHTTOPEDGE;

  } else if (strcasecmp(value, "ENDCAP LEFTTOPEDGE") == 0) {
    _value = ENDCAP_LEF58_LEFTTOPEDGE;

  } else if (strcasecmp(value, "ENDCAP RIGHTBOTTOMCORNER") == 0) {
    _value = ENDCAP_LEF58_RIGHTBOTTOMCORNER;

  } else if (strcasecmp(value, "ENDCAP LEFTBOTTOMCORNER") == 0) {
    _value = ENDCAP_LEF58_LEFTBOTTOMCORNER;

  } else if (strcasecmp(value, "ENDCAP RIGHTTOPCORNER") == 0) {
    _value = ENDCAP_LEF58_RIGHTTOPCORNER;

  } else if (strcasecmp(value, "ENDCAP LEFTTOPCORNER") == 0) {
    _value = ENDCAP_LEF58_LEFTTOPCORNER;
  }
}

dbMasterType::dbMasterType(Value value)
{
  _value = value;
}

dbMasterType::dbMasterType()
{
  _value = CORE;
}

dbMasterType::dbMasterType(const dbMasterType& value)
{
  _value = value._value;
}

const char* dbMasterType::getString() const
{
  const char* value = "";

  switch (_value) {
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

    case ENDCAP_LEF58_BOTTOMEDGE:
      value = "ENDCAP BOTTOMEDGE";
      break;

    case ENDCAP_LEF58_TOPEDGE:
      value = "ENDCAP TOPEDGE";
      break;

    case ENDCAP_LEF58_RIGHTEDGE:
      value = "ENDCAP RIGHTEDGE";
      break;

    case ENDCAP_LEF58_LEFTEDGE:
      value = "ENDCAP LEFTEDGE";
      break;

    case ENDCAP_LEF58_RIGHTBOTTOMEDGE:
      value = "ENDCAP RIGHTBOTTOMEDGE";
      break;

    case ENDCAP_LEF58_LEFTBOTTOMEDGE:
      value = "ENDCAP LEFTBOTTOMEDGE";
      break;

    case ENDCAP_LEF58_RIGHTTOPEDGE:
      value = "ENDCAP RIGHTTOPEDGE";
      break;

    case ENDCAP_LEF58_LEFTTOPEDGE:
      value = "ENDCAP LEFTTOPEDGE";
      break;

    case ENDCAP_LEF58_RIGHTBOTTOMCORNER:
      value = "ENDCAP RIGHTBOTTOMCORNER";
      break;

    case ENDCAP_LEF58_LEFTBOTTOMCORNER:
      value = "ENDCAP LEFTBOTTOMCORNER";
      break;

    case ENDCAP_LEF58_RIGHTTOPCORNER:
      value = "ENDCAP RIGHTTOPCORNER";
      break;

    case ENDCAP_LEF58_LEFTTOPCORNER:
      value = "ENDCAP LEFTTOPCORNER";
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
    case ENDCAP_LEF58_BOTTOMEDGE:
    case ENDCAP_LEF58_TOPEDGE:
    case ENDCAP_LEF58_RIGHTEDGE:
    case ENDCAP_LEF58_LEFTEDGE:
    case ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case ENDCAP_LEF58_RIGHTTOPEDGE:
    case ENDCAP_LEF58_LEFTTOPEDGE:
    case ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case ENDCAP_LEF58_RIGHTTOPCORNER:
    case ENDCAP_LEF58_LEFTTOPCORNER:
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
    case ENDCAP_LEF58_BOTTOMEDGE:
    case ENDCAP_LEF58_TOPEDGE:
    case ENDCAP_LEF58_RIGHTEDGE:
    case ENDCAP_LEF58_LEFTEDGE:
    case ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case ENDCAP_LEF58_RIGHTTOPEDGE:
    case ENDCAP_LEF58_LEFTTOPEDGE:
    case ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case ENDCAP_LEF58_RIGHTTOPCORNER:
    case ENDCAP_LEF58_LEFTTOPCORNER:
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
    case ENDCAP_LEF58_BOTTOMEDGE:
    case ENDCAP_LEF58_TOPEDGE:
    case ENDCAP_LEF58_RIGHTEDGE:
    case ENDCAP_LEF58_LEFTEDGE:
    case ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case ENDCAP_LEF58_RIGHTTOPEDGE:
    case ENDCAP_LEF58_LEFTTOPEDGE:
    case ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case ENDCAP_LEF58_RIGHTTOPCORNER:
    case ENDCAP_LEF58_LEFTTOPCORNER:
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
    case ENDCAP_LEF58_BOTTOMEDGE:
    case ENDCAP_LEF58_TOPEDGE:
    case ENDCAP_LEF58_RIGHTEDGE:
    case ENDCAP_LEF58_LEFTEDGE:
    case ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case ENDCAP_LEF58_RIGHTTOPEDGE:
    case ENDCAP_LEF58_LEFTTOPEDGE:
    case ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case ENDCAP_LEF58_RIGHTTOPCORNER:
    case ENDCAP_LEF58_LEFTTOPCORNER:
      return true;
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

bool dbMasterType::isCover() const
{
  switch (_value) {
    case COVER:
    case COVER_BUMP:
      return true;
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
    case ENDCAP:
    case ENDCAP_PRE:
    case ENDCAP_POST:
    case ENDCAP_TOPLEFT:
    case ENDCAP_TOPRIGHT:
    case ENDCAP_BOTTOMLEFT:
    case ENDCAP_BOTTOMRIGHT:
    case ENDCAP_LEF58_BOTTOMEDGE:
    case ENDCAP_LEF58_TOPEDGE:
    case ENDCAP_LEF58_RIGHTEDGE:
    case ENDCAP_LEF58_LEFTEDGE:
    case ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case ENDCAP_LEF58_RIGHTTOPEDGE:
    case ENDCAP_LEF58_LEFTTOPEDGE:
    case ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case ENDCAP_LEF58_RIGHTTOPCORNER:
    case ENDCAP_LEF58_LEFTTOPCORNER:
      return false;
  }
  assert(false);
  return false;
}

std::optional<dbTechLayerType::Value> dbTechLayerType::fromString(
    const char* value)
{
  std::optional<dbTechLayerType::Value> ret;
  if (strcasecmp(value, "ROUTING") == 0) {
    ret = ROUTING;

  } else if (strcasecmp(value, "CUT") == 0) {
    ret = CUT;

  } else if (strcasecmp(value, "MASTERSLICE") == 0) {
    ret = MASTERSLICE;

  } else if (strcasecmp(value, "OVERLAP") == 0) {
    ret = OVERLAP;

  } else if (strcasecmp(value, "IMPLANT") == 0) {
    ret = IMPLANT;

  } else {
    ret = {};  // NONE;     // mismatch with noarg constructor: ROUTING
  }

  return ret;
}

const dbTechLayerType::Value dbTechLayerType::DEFAULT;

dbTechLayerType::dbTechLayerType(const char* value)
{
  auto opt = fromString(value);
  _value = opt.value_or(dbTechLayerType::DEFAULT);
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
  if (strcasecmp(value, "NONE") == 0) {
    _value = NONE;

  } else if (strcasecmp(value, "HORIZONTAL") == 0) {
    _value = HORIZONTAL;

  } else if (strcasecmp(value, "VERTICAL") == 0) {
    _value = VERTICAL;

  } else {
    _value = NONE;
  }
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
  if (strcasecmp(value, "INSIDECORNER") == 0) {
    _value = INSIDE_CORNER;

  } else if (strcasecmp(value, "OUTSIDECORNER") == 0) {
    _value = OUTSIDE_CORNER;

  } else if (strcasecmp(value, "STEP") == 0) {
    _value = STEP;

  } else {
    _value = OUTSIDE_CORNER;
  }
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
  if (strcasecmp(value, "UNKNOWN") == 0) {
    _value = UNKNOWN;

  } else if (strcasecmp(value, "BLOCK") == 0) {
    _value = BLOCK;

  } else if (strcasecmp(value, "INST") == 0) {
    _value = INST;

  } else if (strcasecmp(value, "BTERM") == 0) {
    _value = BTERM;

  } else if (strcasecmp(value, "BPIN") == 0) {
    _value = BPIN;

  } else if (strcasecmp(value, "VIA") == 0) {
    _value = VIA;

  } else if (strcasecmp(value, "OBSTRUCTION") == 0) {
    _value = OBSTRUCTION;

  } else if (strcasecmp(value, "BLOCKAGE") == 0) {
    _value = BLOCKAGE;

  } else if (strcasecmp(value, "MASTER") == 0) {
    _value = MASTER;

  } else if (strcasecmp(value, "MPIN") == 0) {
    _value = MPIN;

  } else if (strcasecmp(value, "TECH VIA") == 0) {
    _value = TECH_VIA;

  } else if (strcasecmp(value, "SWIRE") == 0) {
    _value = SWIRE;

  } else if (strcasecmp(value, "REGION") == 0) {
    _value = REGION;

  } else if (strcasecmp(value, "PBOX") == 0) {
    _value = PBOX;

  } else {
    // mismatch with noarg constructor: BLOCK
    _value = UNKNOWN;
  }
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
      value = "TECH VIA";
      break;

    case SWIRE:
      value = "SWIRE";
      break;

    case REGION:
      value = "REGION";
      break;

    case PBOX:
      value = "PBOX";
      break;
  }

  return value;
}

dbPolygonOwner::dbPolygonOwner(const char* value)
{
  if (strcasecmp(value, "UNKNOWN") == 0) {
    _value = UNKNOWN;

  } else if (strcasecmp(value, "BPIN") == 0) {
    _value = BPIN;

  } else if (strcasecmp(value, "OBSTRUCTION") == 0) {
    _value = OBSTRUCTION;

  } else if (strcasecmp(value, "SWIRE") == 0) {
    _value = SWIRE;

  } else {
    _value = UNKNOWN;
  }
}

dbPolygonOwner::dbPolygonOwner(Value value)
{
  _value = value;
}

dbPolygonOwner::dbPolygonOwner()
{
  _value = UNKNOWN;
}

dbPolygonOwner::dbPolygonOwner(const dbPolygonOwner& value)
{
  _value = value._value;
}

const char* dbPolygonOwner::getString() const
{
  const char* value = "";

  switch (_value) {
    case UNKNOWN:
      value = "UNKNOWN";
      break;

    case BPIN:
      value = "BPIN";
      break;

    case OBSTRUCTION:
      value = "OBSTRUCTION";
      break;

    case SWIRE:
      value = "SWIRE";
      break;
  }

  return value;
}

dbWireType::dbWireType(const char* value)
{
  _value = NONE;
  if (strcasecmp(value, "NONE") == 0) {
    _value = NONE;

  } else if (strcasecmp(value, "COVER") == 0) {
    _value = COVER;

  } else if (strcasecmp(value, "FIXED") == 0) {
    _value = FIXED;

  } else if (strcasecmp(value, "ROUTED") == 0) {
    _value = ROUTED;

  } else if (strcasecmp(value, "SHIELD") == 0) {
    _value = SHIELD;

  } else if (strcasecmp(value, "NOSHIELD") == 0) {
    _value = NOSHIELD;
  }
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
  _value = NONE;
  if (strcasecmp(value, "NONE") == 0) {
    _value = NONE;

  } else if (strcasecmp(value, "RING") == 0) {
    _value = RING;

  } else if (strcasecmp(value, "PADRING") == 0) {
    _value = PADRING;

  } else if (strcasecmp(value, "BLOCKRING") == 0) {
    _value = BLOCKRING;

  } else if (strcasecmp(value, "STRIPE") == 0) {
    _value = STRIPE;

  } else if (strcasecmp(value, "FOLLOWPIN") == 0) {
    _value = FOLLOWPIN;

  } else if (strcasecmp(value, "IOWIRE") == 0) {
    _value = IOWIRE;

  } else if (strcasecmp(value, "COREWIRE") == 0) {
    _value = COREWIRE;

  } else if (strcasecmp(value, "BLOCKWIRE") == 0) {
    _value = BLOCKWIRE;

  } else if (strcasecmp(value, "BLOCKAGEWIRE") == 0) {
    _value = BLOCKAGEWIRE;

  } else if (strcasecmp(value, "FILLWIRE") == 0) {
    _value = FILLWIRE;

  } else if (strcasecmp(value, "DRCFILL") == 0) {
    _value = DRCFILL;
  }
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
  if (strcasecmp(value, "NONE") == 0) {
    _value = NONE;

  } else if (strcasecmp(value, "PAD") == 0) {
    _value = PAD;

  } else if (strcasecmp(value, "CORE") == 0) {
    _value = CORE;

  } else {
    _value = NONE;
  }
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
  if (strcasecmp(instr, "ON") == 0) {
    _value = ON;
  } else if (strcasecmp(instr, "OFF") == 0) {
    _value = OFF;
  } else {
    _value = OFF;
  }
}

dbOnOffType::dbOnOffType(Value inval)
{
  _value = inval;
}

dbOnOffType::dbOnOffType(const dbOnOffType& value)
{
  _value = value._value;
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
  if (strcasecmp(instr, "MAXXY") == 0) {
    _value = MAXXY;
  } else if (strcasecmp(instr, "EUCLIDEAN") == 0) {
    _value = EUCLIDEAN;
  } else {
    _value = EUCLIDEAN;
  }
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
  if (strcasecmp(value, "HORIZONTAL") == 0) {
    _value = HORIZONTAL;

  } else if (strcasecmp(value, "VERTICAL") == 0) {
    _value = VERTICAL;

  } else {
    // mismatch with noarg constructor: HORIZONTAL
    _value = VERTICAL;
  }
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

dbRegionType::dbRegionType(const char* instr)
{
  if (strcasecmp(instr, "INCLUSIVE") == 0) {
    _value = INCLUSIVE;

  } else if (strcasecmp(instr, "EXCLUSIVE") == 0) {
    _value = EXCLUSIVE;

  } else if (strcasecmp(instr, "SUGGESTED") == 0) {
    _value = SUGGESTED;

  } else {
    _value = INCLUSIVE;
  }
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
  if (strcasecmp(value, "NETLIST") == 0) {
    _value = NETLIST;

  } else if (strcasecmp(value, "DIST") == 0) {
    _value = DIST;

  } else if (strcasecmp(value, "USER") == 0) {
    _value = USER;

  } else if (strcasecmp(value, "TIMING") == 0) {
    _value = TIMING;

  } else if (strcasecmp(value, "TEST") == 0) {
    _value = TEST;

  } else if (strcasecmp(value, "NONE") == 0) {
    _value = NONE;

  } else {
    // mismatch with noarg constructor: NONE
    _value = NETLIST;
  }
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
dbJournalEntryType::dbJournalEntryType(const char* instr)
{
  if (strcasecmp(instr, "NONE") == 0) {
    _value = NONE;

  } else if (strcasecmp(instr, "OWNER") == 0) {
    _value = OWNER;

  } else if (strcasecmp(instr, "ADD") == 0) {
    _value = ADD;

  } else if (strcasecmp(instr, "DESTROY") == 0) {
    _value = DESTROY;

  } else {
    _value = NONE;
  }
}

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
dbDirection::dbDirection(const char* instr)
{
  if (strcasecmp(instr, "NONE") == 0) {
    _value = NONE;

  } else if (strcasecmp(instr, "NORTH") == 0) {
    _value = NORTH;

  } else if (strcasecmp(instr, "EAST") == 0) {
    _value = EAST;

  } else if (strcasecmp(instr, "SOUTH") == 0) {
    _value = SOUTH;

  } else if (strcasecmp(instr, "WEST") == 0) {
    _value = WEST;

  } else if (strcasecmp(instr, "UP") == 0) {
    _value = UP;

  } else if (strcasecmp(instr, "DOWN") == 0) {
    _value = DOWN;

  } else {
    _value = NONE;
  }
}

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
    case UP:
      value = "UP";
      break;
    case DOWN:
      value = "DOWN";
      break;
  }

  return value;
}

dbMTermShapeType::dbMTermShapeType(const char* value)
{
  if (strcasecmp(value, "NONE") == 0) {
    _value = NONE;

  } else if (strcasecmp(value, "RING") == 0) {
    _value = RING;

  } else if (strcasecmp(value, "ABUTMENT") == 0) {
    _value = ABUTMENT;

  } else if (strcasecmp(value, "FEEDTHRU") == 0) {
    _value = FEEDTHRU;

  } else {
    _value = NONE;
  }
}

dbMTermShapeType::dbMTermShapeType(Value value)
{
  _value = value;
}

dbMTermShapeType::dbMTermShapeType()
{
  _value = NONE;
}

dbMTermShapeType::dbMTermShapeType(const dbMTermShapeType& value)
{
  _value = value._value;
}

const char* dbMTermShapeType::getString() const
{
  const char* value = "";

  switch (_value) {
    case NONE:
      value = "NONE";
      break;

    case RING:
      value = "RING";
      break;

    case FEEDTHRU:
      value = "FEEDTHRU";
      break;

    case ABUTMENT:
      value = "ABUTMENT";
      break;
  }

  return value;
}

dbAccessType::dbAccessType(const char* type)
{
  if (strcasecmp(type, "OnGrid") == 0) {
    _value = OnGrid;

  } else if (strcasecmp(type, "HalfGrid") == 0) {
    _value = HalfGrid;

  } else if (strcasecmp(type, "Center") == 0) {
    _value = Center;

  } else if (strcasecmp(type, "EncOpt") == 0) {
    _value = EncOpt;

  } else if (strcasecmp(type, "NearbyGrid") == 0) {
    _value = NearbyGrid;

  } else {
    _value = OnGrid;
  }
}

dbAccessType::dbAccessType(Value type)
{
  _value = type;
}

dbAccessType::dbAccessType()
{
  _value = OnGrid;
}

const char* dbAccessType::getString() const
{
  const char* value = "";

  switch (_value) {
    case OnGrid:
      value = "OnGrid";
      break;

    case HalfGrid:
      value = "HalfGrid";
      break;

    case Center:
      value = "Center";
      break;

    case EncOpt:
      value = "EncOpt";
      break;

    case NearbyGrid:
      value = "NearbyGrid";
      break;
  }

  return value;
}

}  // namespace odb
