// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbTypes.h"

#include <strings.h>

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>

#include "odb/dbId.h"
#include "odb/dbStream.h"

namespace odb {

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
  } else if (strcasecmp(orient, "MYR90") == 0
             || strcasecmp(orient, "MY_R90") == 0) {
    ret = MYR90;
  } else if (strcasecmp(orient, "MX") == 0) {
    ret = MX;
  } else if (strcasecmp(orient, "MXR90") == 0
             || strcasecmp(orient, "MX_R90") == 0) {
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

dbOrientType::dbOrientType(const char* orient)
{
  auto opt = fromString(orient);
  value_ = opt.value_or(dbOrientType::DEFAULT);
}

const char* dbOrientType::getString() const
{
  switch (value_) {
    case R0:
      return "R0";

    case R90:
      return "R90";

    case R180:
      return "R180";

    case R270:
      return "R270";

    case MY:
      return "MY";

    case MYR90:
      return "MYR90";

    case MX:
      return "MX";

    case MXR90:
      return "MXR90";
  }

  return "";
}

dbOrientType dbOrientType::flipX() const
{
  switch (value_) {
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
  switch (value_) {
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
  switch (value_) {
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

std::optional<dbOrientType3D> dbOrientType3D::fromString(
    const std::string& orient)
{
  std::string orient_str = orient;
  bool mirror_z = false;
  // check if the orient string contains "MZ"
  if (orient_str == "MZ") {
    return dbOrientType3D(dbOrientType::R0, true);
  }
  if (orient_str.find("MZ_") != std::string::npos) {
    mirror_z = true;
    orient_str = orient_str.erase(orient_str.find("MZ_"), 3);
  }
  auto opt = dbOrientType::fromString(orient_str.c_str());
  if (!opt.has_value()) {
    return std::nullopt;
  }
  return dbOrientType3D(opt.value(), mirror_z);
}

dbOrientType3D::dbOrientType3D(const std::string& orient)
{
  auto opt = fromString(orient);
  if (opt.has_value()) {
    value_ = opt.value().value_;
    mirror_z_ = opt.value().mirror_z_;
  } else {
    value_ = dbOrientType::DEFAULT;
    mirror_z_ = false;
  }
}

dbOrientType3D::dbOrientType3D(const dbOrientType& orient, bool mirror_z)
{
  value_ = orient.getValue();
  mirror_z_ = mirror_z;
}

std::string dbOrientType3D::getString() const
{
  if (mirror_z_ && getOrientType2D() == dbOrientType::R0) {
    return "MZ";
  }
  std::string orient_2d_str = getOrientType2D().getString();
  if (orient_2d_str == "MXR90") {
    orient_2d_str = "MX_R90";
  } else if (orient_2d_str == "MYR90") {
    orient_2d_str = "MY_R90";
  }
  return (mirror_z_ ? "MZ_" : "") + orient_2d_str;
}

dbOrientType dbOrientType3D::getOrientType2D() const
{
  return value_;
}

bool dbOrientType3D::isMirrorZ() const
{
  return mirror_z_;
}

dbGDSSTrans::dbGDSSTrans(bool flipX, double mag, double angle)
    : flipX_(flipX), mag_(mag), angle_(angle)
{
}

bool dbGDSSTrans::operator==(const dbGDSSTrans& rhs) const
{
  return (flipX_ == rhs.flipX_) && (mag_ == rhs.mag_) && (angle_ == rhs.angle_);
}

std::string dbGDSSTrans::to_string() const
{
  std::string s;
  if (flipX_) {
    s += std::string("FLIP_X ");
  }
  s += "MAG ";
  s += std::to_string(mag_);
  s += " ANGLE ";
  s += std::to_string(angle_);
  s += " ";
  return s;
}

bool dbGDSSTrans::identity() const
{
  return (!flipX_) && (mag_ == 1.0) && (angle_ == 0.0);
}

dbGDSTextPres::dbGDSTextPres(dbGDSTextPres::VPres vPres,
                             dbGDSTextPres::HPres hPres)
    : v_pres_(vPres), h_pres_(hPres)
{
}

bool dbGDSTextPres::operator==(const dbGDSTextPres& rhs) const
{
  return (v_pres_ == rhs.v_pres_) && (h_pres_ == rhs.h_pres_);
}

std::string dbGDSTextPres::to_string() const
{
  std::string s;
  s += "FONT ";
  s += (v_pres_ == dbGDSTextPres::VPres::TOP) ? std::string("TOP ")
                                              : std::string("BOTTOM ");
  s += (h_pres_ == dbGDSTextPres::HPres::LEFT) ? std::string("LEFT ")
                                               : std::string("RIGHT ");
  return s;
}

dbIStream& operator>>(dbIStream& stream, dbGDSSTrans& t)
{
  stream >> t.flipX_;
  stream >> t.mag_;
  stream >> t.angle_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const dbGDSSTrans& t)
{
  stream << t.flipX_;
  stream << t.mag_;
  stream << t.angle_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, dbOrientType3D& t)
{
  uint8_t value;
  stream >> value;
  t.value_ = static_cast<dbOrientType::Value>(value);
  stream >> t.mirror_z_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const dbOrientType3D& t)
{
  stream << static_cast<uint8_t>(t.value_);
  stream << t.mirror_z_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, dbGDSTextPres& t)
{
  uint8_t vPresTemp, hPresTemp;
  stream >> vPresTemp;
  stream >> hPresTemp;
  t.v_pres_ = static_cast<dbGDSTextPres::VPres>(vPresTemp);
  t.h_pres_ = static_cast<dbGDSTextPres::HPres>(hPresTemp);
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const dbGDSTextPres& t)
{
  stream << static_cast<uint8_t>(t.v_pres_);
  stream << static_cast<uint8_t>(t.h_pres_);
  return stream;
}

dbGroupType::dbGroupType(const char* type)
{
  if (strcasecmp(type, "PHYSICAL_CLUSTER") == 0) {
    value_ = PHYSICAL_CLUSTER;

  } else if (strcasecmp(type, "VOLTAGE_DOMAIN") == 0) {
    value_ = VOLTAGE_DOMAIN;

  } else if (strcasecmp(type, "POWER_DOMAIN") == 0) {
    value_ = POWER_DOMAIN;

  } else {
    value_ = PHYSICAL_CLUSTER;
  }
}

const char* dbGroupType::getString() const
{
  switch (value_) {
    case PHYSICAL_CLUSTER:
      return "PHYSICAL_CLUSTER";

    case VOLTAGE_DOMAIN:
      return "VOLTAGE_DOMAIN";

    case POWER_DOMAIN:
      return "POWER_DOMAIN";

    case VISUAL_DEBUG:
      return "VISUAL_DEBUG";
  }

  return "";
}

dbSigType::dbSigType(const char* value)
{
  if (strcasecmp(value, "SIGNAL") == 0) {
    value_ = SIGNAL;

  } else if (strcasecmp(value, "POWER") == 0) {
    value_ = POWER;

  } else if (strcasecmp(value, "GROUND") == 0) {
    value_ = GROUND;

  } else if (strcasecmp(value, "CLOCK") == 0) {
    value_ = CLOCK;

  } else if (strcasecmp(value, "ANALOG") == 0) {
    value_ = ANALOG;

  } else if (strcasecmp(value, "RESET") == 0) {
    value_ = RESET;

  } else if (strcasecmp(value, "SCAN") == 0) {
    value_ = SCAN;

  } else if (strcasecmp(value, "TIEOFF") == 0) {
    value_ = TIEOFF;

  } else {
    value_ = SIGNAL;
  }
}

bool dbSigType::isSupply() const
{
  switch (value_) {
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
  switch (value_) {
    case SIGNAL:
      return "SIGNAL";

    case POWER:
      return "POWER";

    case GROUND:
      return "GROUND";

    case CLOCK:
      return "CLOCK";

    case ANALOG:
      return "ANALOG";

    case RESET:
      return "RESET";

    case SCAN:
      return "SCAN";

    case TIEOFF:
      return "TIEOFF";
  }

  return "";
}

dbIoType::dbIoType(const char* value)
{
  if (strcasecmp(value, "INPUT") == 0) {
    value_ = INPUT;

  } else if (strcasecmp(value, "OUTPUT") == 0) {
    value_ = OUTPUT;

  } else if (strcasecmp(value, "INOUT") == 0) {
    value_ = INOUT;

  } else if (strcasecmp(value, "FEEDTHRU") == 0) {
    value_ = FEEDTHRU;

  } else {
    value_ = INPUT;
  }
}

const char* dbIoType::getString() const
{
  switch (value_) {
    case INPUT:
      return "INPUT";

    case OUTPUT:
      return "OUTPUT";

    case INOUT:
      return "INOUT";

    case FEEDTHRU:
      return "FEEDTHRU";
  }

  return "";
}

dbPlacementStatus::dbPlacementStatus(const char* value)
{
  if (strcasecmp(value, "NONE") == 0) {
    value_ = NONE;

  } else if (strcasecmp(value, "UNPLACED") == 0) {
    value_ = UNPLACED;

  } else if (strcasecmp(value, "SUGGESTED") == 0) {
    value_ = SUGGESTED;

  } else if (strcasecmp(value, "PLACED") == 0) {
    value_ = PLACED;

  } else if (strcasecmp(value, "LOCKED") == 0) {
    value_ = LOCKED;

  } else if (strcasecmp(value, "FIRM") == 0) {
    value_ = FIRM;

  } else if (strcasecmp(value, "COVER") == 0) {
    value_ = COVER;

  } else {
    value_ = NONE;
  }
}

const char* dbPlacementStatus::getString() const
{
  switch (value_) {
    case NONE:
      return "NONE";

    case UNPLACED:
      return "UNPLACED";

    case SUGGESTED:
      return "SUGGESTED";

    case PLACED:
      return "PLACED";

    case LOCKED:
      return "LOCKED";

    case FIRM:
      return "FIRM";

    case COVER:
      return "COVER";
  }

  return "";
}

bool dbPlacementStatus::isPlaced() const
{
  switch (value_) {
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
  switch (value_) {
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
  value_ = CORE;

  if (strcasecmp(value, "COVER") == 0) {
    value_ = COVER;

  } else if (strcasecmp(value, "COVER BUMP") == 0) {
    value_ = COVER_BUMP;

  } else if (strcasecmp(value, "RING") == 0) {
    value_ = RING;

  } else if (strcasecmp(value, "BLOCK") == 0) {
    value_ = BLOCK;

  } else if (strcasecmp(value, "BLOCK BLACKBOX") == 0) {
    value_ = BLOCK_BLACKBOX;

  } else if (strcasecmp(value, "BLOCK SOFT") == 0) {
    value_ = BLOCK_SOFT;

  } else if (strcasecmp(value, "PAD") == 0) {
    value_ = PAD;

  } else if (strcasecmp(value, "PAD INPUT") == 0) {
    value_ = PAD_INPUT;

  } else if (strcasecmp(value, "PAD OUTPUT") == 0) {
    value_ = PAD_OUTPUT;

  } else if (strcasecmp(value, "PAD INOUT") == 0) {
    value_ = PAD_INOUT;

  } else if (strcasecmp(value, "PAD POWER") == 0) {
    value_ = PAD_POWER;

  } else if (strcasecmp(value, "PAD SPACER") == 0) {
    value_ = PAD_SPACER;

  } else if (strcasecmp(value, "PAD AREAIO") == 0) {
    value_ = PAD_AREAIO;

  } else if (strcasecmp(value, "CORE") == 0) {
    value_ = CORE;

  } else if (strcasecmp(value, "CORE FEEDTHRU") == 0) {
    value_ = CORE_FEEDTHRU;

  } else if (strcasecmp(value, "CORE TIEHIGH") == 0) {
    value_ = CORE_TIEHIGH;

  } else if (strcasecmp(value, "CORE TIELOW") == 0) {
    value_ = CORE_TIELOW;

  } else if (strcasecmp(value, "CORE SPACER") == 0) {
    value_ = CORE_SPACER;

  } else if (strcasecmp(value, "CORE ANTENNACELL") == 0) {
    value_ = CORE_ANTENNACELL;

  } else if (strcasecmp(value, "CORE WELLTAP") == 0) {
    value_ = CORE_WELLTAP;

  } else if (strcasecmp(value, "ENDCAP") == 0) {
    value_ = ENDCAP;

  } else if (strcasecmp(value, "ENDCAP PRE") == 0) {
    value_ = ENDCAP_PRE;

  } else if (strcasecmp(value, "ENDCAP POST") == 0) {
    value_ = ENDCAP_POST;

  } else if (strcasecmp(value, "ENDCAP TOPLEFT") == 0) {
    value_ = ENDCAP_TOPLEFT;

  } else if (strcasecmp(value, "ENDCAP TOPRIGHT") == 0) {
    value_ = ENDCAP_TOPRIGHT;

  } else if (strcasecmp(value, "ENDCAP BOTTOMLEFT") == 0) {
    value_ = ENDCAP_BOTTOMLEFT;

  } else if (strcasecmp(value, "ENDCAP BOTTOMRIGHT") == 0) {
    value_ = ENDCAP_BOTTOMRIGHT;

  } else if (strcasecmp(value, "ENDCAP BOTTOMEDGE") == 0) {
    value_ = ENDCAP_LEF58_BOTTOMEDGE;

  } else if (strcasecmp(value, "ENDCAP TOPEDGE") == 0) {
    value_ = ENDCAP_LEF58_TOPEDGE;

  } else if (strcasecmp(value, "ENDCAP RIGHTEDGE") == 0) {
    value_ = ENDCAP_LEF58_RIGHTEDGE;

  } else if (strcasecmp(value, "ENDCAP LEFTEDGE") == 0) {
    value_ = ENDCAP_LEF58_LEFTEDGE;

  } else if (strcasecmp(value, "ENDCAP RIGHTBOTTOMEDGE") == 0) {
    value_ = ENDCAP_LEF58_RIGHTBOTTOMEDGE;

  } else if (strcasecmp(value, "ENDCAP LEFTBOTTOMEDGE") == 0) {
    value_ = ENDCAP_LEF58_LEFTBOTTOMEDGE;

  } else if (strcasecmp(value, "ENDCAP RIGHTTOPEDGE") == 0) {
    value_ = ENDCAP_LEF58_RIGHTTOPEDGE;

  } else if (strcasecmp(value, "ENDCAP LEFTTOPEDGE") == 0) {
    value_ = ENDCAP_LEF58_LEFTTOPEDGE;

  } else if (strcasecmp(value, "ENDCAP RIGHTBOTTOMCORNER") == 0) {
    value_ = ENDCAP_LEF58_RIGHTBOTTOMCORNER;

  } else if (strcasecmp(value, "ENDCAP LEFTBOTTOMCORNER") == 0) {
    value_ = ENDCAP_LEF58_LEFTBOTTOMCORNER;

  } else if (strcasecmp(value, "ENDCAP RIGHTTOPCORNER") == 0) {
    value_ = ENDCAP_LEF58_RIGHTTOPCORNER;

  } else if (strcasecmp(value, "ENDCAP LEFTTOPCORNER") == 0) {
    value_ = ENDCAP_LEF58_LEFTTOPCORNER;
  }
}

const char* dbMasterType::getString() const
{
  switch (value_) {
    case COVER:
      return "COVER";

    case COVER_BUMP:
      return "COVER BUMP";

    case RING:
      return "RING";

    case BLOCK:
      return "BLOCK";

    case BLOCK_BLACKBOX:
      return "BLOCK BLACKBOX";

    case BLOCK_SOFT:
      return "BLOCK SOFT";

    case PAD:
      return "PAD";

    case PAD_INPUT:
      return "PAD INPUT";

    case PAD_OUTPUT:
      return "PAD OUTPUT";

    case PAD_INOUT:
      return "PAD INOUT";

    case PAD_POWER:
      return "PAD POWER";

    case PAD_SPACER:
      return "PAD SPACER";

    case PAD_AREAIO:
      return "PAD AREAIO";

    case CORE:
      return "CORE";

    case CORE_FEEDTHRU:
      return "CORE FEEDTHRU";

    case CORE_TIEHIGH:
      return "CORE TIEHIGH";

    case CORE_TIELOW:
      return "CORE TIELOW";

    case CORE_SPACER:
      return "CORE SPACER";

    case CORE_ANTENNACELL:
      return "CORE ANTENNACELL";

    case CORE_WELLTAP:
      return "CORE WELLTAP";

    case ENDCAP:
      return "ENDCAP";

    case ENDCAP_PRE:
      return "ENDCAP PRE";

    case ENDCAP_POST:
      return "ENDCAP POST";

    case ENDCAP_TOPLEFT:
      return "ENDCAP TOPLEFT";

    case ENDCAP_TOPRIGHT:
      return "ENDCAP TOPRIGHT";

    case ENDCAP_BOTTOMLEFT:
      return "ENDCAP BOTTOMLEFT";

    case ENDCAP_BOTTOMRIGHT:
      return "ENDCAP BOTTOMRIGHT";

    case ENDCAP_LEF58_BOTTOMEDGE:
      return "ENDCAP BOTTOMEDGE";

    case ENDCAP_LEF58_TOPEDGE:
      return "ENDCAP TOPEDGE";

    case ENDCAP_LEF58_RIGHTEDGE:
      return "ENDCAP RIGHTEDGE";

    case ENDCAP_LEF58_LEFTEDGE:
      return "ENDCAP LEFTEDGE";

    case ENDCAP_LEF58_RIGHTBOTTOMEDGE:
      return "ENDCAP RIGHTBOTTOMEDGE";

    case ENDCAP_LEF58_LEFTBOTTOMEDGE:
      return "ENDCAP LEFTBOTTOMEDGE";

    case ENDCAP_LEF58_RIGHTTOPEDGE:
      return "ENDCAP RIGHTTOPEDGE";

    case ENDCAP_LEF58_LEFTTOPEDGE:
      return "ENDCAP LEFTTOPEDGE";

    case ENDCAP_LEF58_RIGHTBOTTOMCORNER:
      return "ENDCAP RIGHTBOTTOMCORNER";

    case ENDCAP_LEF58_LEFTBOTTOMCORNER:
      return "ENDCAP LEFTBOTTOMCORNER";

    case ENDCAP_LEF58_RIGHTTOPCORNER:
      return "ENDCAP RIGHTTOPCORNER";

    case ENDCAP_LEF58_LEFTTOPCORNER:
      return "ENDCAP LEFTTOPCORNER";
  }

  return "";
}

bool dbMasterType::isBlock() const
{
  switch (value_) {
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
  switch (value_) {
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
  switch (value_) {
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
  switch (value_) {
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
  switch (value_) {
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

dbTechLayerType::dbTechLayerType(const char* value)
{
  auto opt = fromString(value);
  value_ = opt.value_or(dbTechLayerType::DEFAULT);
}

const char* dbTechLayerType::getString() const
{
  switch (value_) {
    case NONE:
      return "NONE";

    case ROUTING:
      return "ROUTING";

    case CUT:
      return "CUT";

    case MASTERSLICE:
      return "MASTERSLICE";

    case OVERLAP:
      return "OVERLAP";

    case IMPLANT:
      return "IMPLANT";
  }

  return "";
}

dbTechLayerDir::dbTechLayerDir(const char* value)
{
  if (strcasecmp(value, "NONE") == 0) {
    value_ = NONE;

  } else if (strcasecmp(value, "HORIZONTAL") == 0) {
    value_ = HORIZONTAL;

  } else if (strcasecmp(value, "VERTICAL") == 0) {
    value_ = VERTICAL;

  } else {
    value_ = NONE;
  }
}

const char* dbTechLayerDir::getString() const
{
  switch (value_) {
    case NONE:
      return "NONE";

    case HORIZONTAL:
      return "HORIZONTAL";

    case VERTICAL:
      return "VERTICAL";
  }

  return "";
}

dbTechLayerMinStepType::dbTechLayerMinStepType(const char* value)
{
  if (strcasecmp(value, "INSIDECORNER") == 0) {
    value_ = INSIDE_CORNER;

  } else if (strcasecmp(value, "OUTSIDECORNER") == 0) {
    value_ = OUTSIDE_CORNER;

  } else if (strcasecmp(value, "STEP") == 0) {
    value_ = STEP;

  } else {
    value_ = OUTSIDE_CORNER;
  }
}

const char* dbTechLayerMinStepType::getString() const
{
  switch (value_) {
    case INSIDE_CORNER:
      return "INSIDECORNER";

    case OUTSIDE_CORNER:
      return "OUTSIDECORNER";

    case STEP:
      return "STEP";
  }

  return "";
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

const char* dbBoxOwner::getString() const
{
  switch (_value) {
    case UNKNOWN:
      return "UNKNOWN";

    case BLOCK:
      return "BLOCK";

    case INST:
      return "INST";

    case BTERM:
      return "BTERM";

    case BPIN:
      return "BPIN";

    case VIA:
      return "VIA";

    case OBSTRUCTION:
      return "OBSTRUCTION";

    case BLOCKAGE:
      return "BLOCKAGE";

    case MASTER:
      return "MASTER";

    case MPIN:
      return "MPIN";

    case TECH_VIA:
      return "TECH VIA";

    case SWIRE:
      return "SWIRE";

    case REGION:
      return "REGION";

    case PBOX:
      return "PBOX";
  }

  return "";
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

const char* dbPolygonOwner::getString() const
{
  switch (_value) {
    case UNKNOWN:
      return "UNKNOWN";

    case BPIN:
      return "BPIN";

    case OBSTRUCTION:
      return "OBSTRUCTION";

    case SWIRE:
      return "SWIRE";
  }

  return "";
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

const char* dbWireType::getString() const
{
  switch (_value) {
    case NONE:
      return "NONE";

    case COVER:
      return "COVER";

    case FIXED:
      return "FIXED";

    case ROUTED:
      return "ROUTED";

    case SHIELD:
      return "SHIELD";

    case NOSHIELD:
      return "NOSHIELD";
  }

  return "";
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

const char* dbWireShapeType::getString() const
{
  switch (_value) {
    case NONE:
      return "NONE";

    case RING:
      return "RING";

    case PADRING:
      return "PADRING";

    case BLOCKRING:
      return "BLOCKRING";

    case STRIPE:
      return "STRIPE";

    case FOLLOWPIN:
      return "FOLLOWPIN";

    case IOWIRE:
      return "IOWIRE";

    case COREWIRE:
      return "COREWIRE";

    case BLOCKWIRE:
      return "BLOCKWIRE";

    case BLOCKAGEWIRE:
      return "BLOCKAGEWIRE";

    case FILLWIRE:
      return "FILLWIRE";

    case DRCFILL:
      return "DRCFILL";
  }

  return "";
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

const char* dbSiteClass::getString() const
{
  switch (_value) {
    case NONE:
      return "NONE";

    case PAD:
      return "PAD";

    case CORE:
      return "CORE";
  }

  return "";
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

const char* dbOnOffType::getString() const
{
  switch (_value) {
    case OFF:
      return "OFF";

    case ON:
      return "ON";
  }

  return "";
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
  switch (_value) {
    case EUCLIDEAN:
      return "EUCLIDEAN";

    case MAXXY:
      return "MAXXY";
  }

  return "";
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

const char* dbRowDir::getString() const
{
  switch (_value) {
    case HORIZONTAL:
      return "HORIZONTAL";

    case VERTICAL:
      return "VERTICAL";
  }

  return "";
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
  switch (_value) {
    case INCLUSIVE:
      return "INCLUSIVE";

    case EXCLUSIVE:
      return "EXCLUSIVE";

    case SUGGESTED:
      return "SUGGESTED";
  }

  return "";
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
  switch (_value) {
    case NONE:
      return "NONE";

    case NETLIST:
      return "NETLIST";

    case DIST:
      return "DIST";

    case USER:
      return "USER";

    case TIMING:
      return "TIMING";

    case TEST:
      return "TEST";
  }

  return "";
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
  switch (_value) {
    case NONE:
      return "NONE";

    case OWNER:
      return "OWNER";

    case ADD:
      return "ADD";

    case DESTROY:
      return "DESTROY";
  }

  return "";
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
  switch (_value) {
    case NONE:
      return "NONE";

    case NORTH:
      return "NORTH";

    case EAST:
      return "EAST";

    case SOUTH:
      return "SOUTH";

    case WEST:
      return "WEST";
    case UP:
      return "UP";
    case DOWN:
      return "DOWN";
  }

  return "";
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

const char* dbMTermShapeType::getString() const
{
  switch (_value) {
    case NONE:
      return "NONE";

    case RING:
      return "RING";

    case FEEDTHRU:
      return "FEEDTHRU";

    case ABUTMENT:
      return "ABUTMENT";
  }

  return "";
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

const char* dbAccessType::getString() const
{
  switch (_value) {
    case OnGrid:
      return "OnGrid";

    case HalfGrid:
      return "HalfGrid";

    case Center:
      return "Center";

    case EncOpt:
      return "EncOpt";

    case NearbyGrid:
      return "NearbyGrid";
  }

  return "";
}

const char* dbNameUniquifyType::getString() const
{
  switch (_value) {
    case ALWAYS:
      return "ALWAYS";

    case ALWAYS_WITH_UNDERSCORE:
      return "ALWAYS_WITH_UNDERSCORE";

    case IF_NEEDED:
      return "IF_NEEDED";

    case IF_NEEDED_WITH_UNDERSCORE:
      return "IF_NEEDED_WITH_UNDERSCORE";
  }

  return "";
}

const char* dbHierSearchDir::getString() const
{
  switch (_value) {
    case FANIN:
      return "FANIN";
    case FANOUT:
      return "FANOUT";
  }
  return "";
}

}  // namespace odb
