// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "odb/dbStream.h"
#include "odb/geom.h"

namespace odb {

///
/// This file declares the non-persistent database objects and misc.
/// type-definitions.
///

///
/// The orientation define the rotation and axis mirroring for the
/// placement of various database objects. The values conform the orient
/// definitions defined in the LEF/DEF formats.
///
class dbOrientType
{
 public:
  enum Value
  {
    R0,    /** rotate object 0 degrees */
    R90,   /** rotate object 90 degrees */
    R180,  /** rotate object 180 degrees */
    R270,  /** rotate object 270 degrees */
    MY,    /** mirror about the "Y" axis */
    MYR90, /** mirror about the "Y" axis and rotate 90 degrees */
    MX,    /** mirror about the "X" axis */
    MXR90  /** mirror about the "X" axis and rotate 90 degrees */
  };

  static constexpr Value DEFAULT = Value::R0;

  static std::optional<Value> fromString(const char* orient);

  ///
  /// Create a dbOrientType instance with an explicit orientation.
  /// The explicit orientation must be a string of one of the
  ///  following values: "R0", "R90", "R180", "270", "MY", "MX", "MYR90",
  ///  or "MXR90".
  ///
  dbOrientType(const char* orient);

  ///
  /// Create a dbOrientType instance with an explicit orientation.
  ///
  dbOrientType(Value orient) : value_(orient) {}

  ///
  /// Create a dbOrientType instance with orientation "R0".
  ///
  dbOrientType() = default;

  ///
  /// Copy constructor.
  ///
  dbOrientType(const dbOrientType& orient) = default;

  ///
  /// Returns the orientation
  ///
  Value getValue() const { return value_; }

  ///
  /// Returns the orientation as a string
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return value_; }

  ///
  /// Returns the orientation after flipping about the x-axis
  ///
  dbOrientType flipX() const;

  ///
  /// Returns the orientation after flipping about the y-axis
  ///
  dbOrientType flipY() const;

  ///
  /// Returns true if the orientation is any kind of 90 degrees rotation
  ///
  bool isRightAngleRotation() const;

 private:
  Value value_ = DEFAULT;
};

class dbOrientType3D
{
 public:
  static std::optional<dbOrientType3D> fromString(const std::string& orient);

  dbOrientType3D(const std::string& orient);

  ///
  /// Create a dbOrientType3D instance with an explicit orientation.
  ///
  dbOrientType3D(const dbOrientType& orient, bool mirror_z);

  dbOrientType3D() = default;

  ///
  /// Copy constructor.
  ///
  dbOrientType3D(const dbOrientType3D& orient) = default;

  ///
  /// Returns the orientation as a string
  ///
  std::string getString() const;

  dbOrientType getOrientType2D() const;

  bool isMirrorZ() const;

  friend dbIStream& operator>>(dbIStream& stream, dbOrientType3D& t);
  friend dbOStream& operator<<(dbOStream& stream, const dbOrientType3D& t);

 private:
  dbOrientType::Value value_{dbOrientType::R0};
  bool mirror_z_{false};
};

class dbGDSSTrans
{
 public:
  dbGDSSTrans() = default;

  dbGDSSTrans(bool flipX, double mag, double angle);

  bool operator==(const dbGDSSTrans& rhs) const;

  std::string to_string() const;

  bool identity() const;

  bool flipX_ = false;
  double mag_ = 1.0;
  double angle_ = 0.0;
};

dbIStream& operator>>(dbIStream& stream, dbGDSSTrans& t);
dbOStream& operator<<(dbOStream& stream, const dbGDSSTrans& t);

class dbGDSTextPres
{
 public:
  enum VPres
  {
    TOP = 0,
    MIDDLE = 1,
    BOTTOM = 2
  };
  enum HPres
  {
    LEFT = 0,
    CENTER = 1,
    RIGHT = 2
  };

  dbGDSTextPres() = default;

  dbGDSTextPres(VPres vPres, HPres hPres);

  bool operator==(const dbGDSTextPres& rhs) const;

  bool identity() const;

  std::string to_string() const;

  VPres v_pres_ = VPres::TOP;
  HPres h_pres_ = HPres::LEFT;
};

dbIStream& operator>>(dbIStream& stream, dbGDSTextPres& t);
dbOStream& operator<<(dbOStream& stream, const dbGDSTextPres& t);

///
/// The dbGroup's basis.
///
class dbGroupType
{
 public:
  enum Value
  {
    PHYSICAL_CLUSTER,
    VOLTAGE_DOMAIN,
    POWER_DOMAIN,
    VISUAL_DEBUG
  };

  ///
  /// Create a dbGroupType instance with an explicit type.
  /// The explicit orientation must be a string of one of the
  ///  following values: "PHYSICAL_CLUSTER", "VOLTAGE_DOMAIN".
  ///
  dbGroupType(const char* type);

  ///
  /// Create a dbGroupType instance with an explicit type.
  ///
  dbGroupType(Value type) : value_(type) {}

  ///
  /// Create a dbGroupType instance with type "PHYSICAL_CLUSTER".
  ///
  dbGroupType() = default;

  ///
  /// Copy constructor.
  ///
  dbGroupType(const dbGroupType& type) = default;

  ///
  /// Returns the orientation
  ///
  Value getValue() const { return value_; }

  ///
  /// Returns the orientation as a string
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return value_; }

 private:
  Value value_ = Value::PHYSICAL_CLUSTER;
};

///
/// Electrical signals are classified according to the role of the signal.
///
class dbSigType
{
 public:
  enum Value
  {
    SIGNAL, /** */
    POWER,  /** */
    GROUND, /** */
    CLOCK,  /** */
    ANALOG, /** */
    RESET,  /** */
    SCAN,   /** */
    TIEOFF  /** */
  };

  ///
  /// Create a dbSigType instance with an explicit signal value.
  /// The explicit signal value must be a string of one of the
  ///  following values: "signal", "power", "ground", "clock"
  ///
  dbSigType(const char* value);

  ///
  /// Create a dbSigType instance with an explicit signal value.
  ///
  dbSigType(Value value) : value_(value) {}

  ///
  /// Create a dbSigType instance with value "signal".
  ///
  dbSigType() = default;

  ///
  /// Copy constructor.
  ///
  dbSigType(const dbSigType& value) = default;

  ///
  /// Returns the signal-value
  ///
  Value getValue() const { return value_; }

  ///
  /// Returns the signal-value as a string.
  ///
  const char* getString() const;

  ///
  ///  True if value corresponds to POWER or GROUND
  ///
  bool isSupply() const;

  ///
  /// Comparison operators for type safe dbSigType
  ///
  bool operator==(const dbSigType& v) const { return value_ == v.value_; };
  bool operator!=(const dbSigType& v) const { return value_ != v.value_; };
  bool operator==(const Value v) const { return value_ == v; };
  bool operator!=(const Value v) const { return value_ != v; };

 private:
  Value value_ = Value::SIGNAL;
};

///
/// Specifies the functional electrical behavior of a terminal.
///
class dbIoType
{
 public:
  enum Value
  {
    INPUT,
    OUTPUT,
    INOUT,
    FEEDTHRU
  };

  ///
  /// Create a dbIoType instance with an explicit io-direction.
  /// The explicit io-direction must be a string of one of the
  ///  following values: "input", "output", "inout"
  ///
  dbIoType(const char* value);

  ///
  /// Create a dbIoType instance with an explicit IO direction.
  ///
  dbIoType(Value value) : value_(value) {}

  ///
  /// Create a dbIoType instance with value "input".
  ///
  dbIoType() = default;

  ///
  /// Copy constructor.
  ///
  dbIoType(const dbIoType& value) = default;

  ///
  /// Returns the direction of IO of an element.
  ///
  Value getValue() const { return value_; }

  ///
  /// Returns the direction of IO of an element as a string.
  ///
  const char* getString() const;

  ///
  /// Comparison operators for type safe dbIoType
  ///
  bool operator==(const dbIoType& v) const { return value_ == v.value_; };
  bool operator!=(const dbIoType& v) const { return value_ != v.value_; };
  bool operator==(const Value v) const { return value_ == v; };
  bool operator!=(const Value v) const { return value_ != v; };

 private:
  Value value_ = Value::INPUT;
};

///
/// Specifies the placement status of an element.
///
class dbPlacementStatus
{
 public:
  enum Value
  {
    NONE,      /** element has not been placed */
    UNPLACED,  /** element has an arbitrary placement */
    SUGGESTED, /** element has a suggested placement */
    PLACED,    /** element has been placed */
    LOCKED,    /** element cannot be moved */
    FIRM,      /** element cannot be moved by a placer*/
    COVER      /** cover element cannot be moved */
  };

  ///
  /// Create a dbPlacementStatus instance with an explicit placement status.
  /// The explicit status must be a string of one of the
  ///  following values: "none", "unplaced", "suggested", "placed",
  ///  "locked", "firm", "cover".
  ///
  dbPlacementStatus(const char* value);

  ///
  /// Create a dbPlacementStatus instance with an explicit status.
  ///
  dbPlacementStatus(Value value) : value_(value) {}

  ///
  /// Create a dbPlacementStatus instance with status = "none".
  ///
  dbPlacementStatus() = default;

  ///
  /// Copy constructor.
  ///
  dbPlacementStatus(const dbPlacementStatus& value) = default;

  ///
  /// Returns the placement status.
  ///
  Value getValue() const { return value_; }

  ///
  /// Returns the placement status as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return value_; }

  ///
  ///  True if value corresponds to a PLACED, LOCKED, FIRM, or COVER
  ///
  bool isPlaced() const;

  ///
  ///  True if value corresponds to LOCKED, FIRM, or COVER
  ///
  bool isFixed() const;

 private:
  Value value_ = Value::NONE;
};

///
/// Defines the value of cell a master represents.
///
class dbMasterType
{
 public:
  enum Value
  {
    COVER,                          /** */
    COVER_BUMP,                     /** */
    RING,                           /** */
    BLOCK,                          /** */
    BLOCK_BLACKBOX,                 /** */
    BLOCK_SOFT,                     /** */
    PAD,                            /** */
    PAD_INPUT,                      /** */
    PAD_OUTPUT,                     /** */
    PAD_INOUT,                      /** */
    PAD_POWER,                      /** */
    PAD_SPACER,                     /** */
    PAD_AREAIO,                     /** */
    CORE,                           /** */
    CORE_FEEDTHRU,                  /** */
    CORE_TIEHIGH,                   /** */
    CORE_TIELOW,                    /** */
    CORE_SPACER,                    /** */
    CORE_ANTENNACELL,               /** */
    CORE_WELLTAP,                   /** */
    ENDCAP,                         /** */
    ENDCAP_PRE,                     /** */
    ENDCAP_POST,                    /** */
    ENDCAP_TOPLEFT,                 /** */
    ENDCAP_TOPRIGHT,                /** */
    ENDCAP_BOTTOMLEFT,              /** */
    ENDCAP_BOTTOMRIGHT,             /** */
    ENDCAP_LEF58_BOTTOMEDGE,        /** */
    ENDCAP_LEF58_TOPEDGE,           /** */
    ENDCAP_LEF58_LEFTEDGE,          /** */
    ENDCAP_LEF58_RIGHTEDGE,         /** */
    ENDCAP_LEF58_RIGHTBOTTOMEDGE,   /** */
    ENDCAP_LEF58_LEFTBOTTOMEDGE,    /** */
    ENDCAP_LEF58_RIGHTTOPEDGE,      /** */
    ENDCAP_LEF58_LEFTTOPEDGE,       /** */
    ENDCAP_LEF58_RIGHTBOTTOMCORNER, /** */
    ENDCAP_LEF58_LEFTBOTTOMCORNER,  /** */
    ENDCAP_LEF58_RIGHTTOPCORNER,    /** */
    ENDCAP_LEF58_LEFTTOPCORNER,     /** */
  };

  ///
  /// Create a dbMasterType instance with an explicit placement status.
  /// The explicit status must be a string of one of the
  ///  following values: "none", "cover", "ring", "block",
  ///  "block blackbox", "block soft",
  ///  "pad", "pad input", "pad output", "pad inout",
  ///  "pad power", "pad spacer", "pad areaio",
  ///  "core", "core feedthru", "core tielow",
  ///  "core tiehigh", "core antennacell", "core welltap",
  ///  "endcap", "endcap pre", "endcap post",
  ///  "endcap topright", "endcap topleft",
  ///  "endcap bottomright", "endcap bottomleft"
  ///
  dbMasterType(const char* value);

  ///
  /// Create a dbMasterType instance with an explicit value.
  ///
  dbMasterType(Value value) : value_(value) {}

  ///
  /// Create a dbMasterType instance with value = "none".
  ///
  dbMasterType() = default;

  ///
  /// Copy constructor.
  ///
  dbMasterType(const dbMasterType& value) = default;

  ///
  /// Returns the master-value.
  ///
  Value getValue() const { return value_; }

  ///
  /// Returns the master-value as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return value_; }

  ///
  /// Is the type BLOCK or any of its subtypes
  ///
  bool isBlock() const;

  ///
  /// Is the type CORE or any of its subtypes
  ///
  bool isCore() const;

  ///
  /// Is the type PAD or any of its subtypes
  ///
  bool isPad() const;

  ///
  /// Is the type ENDCAP or any of its subtypes
  ///
  bool isEndCap() const;

  ///
  /// Is the type COVER or any of its subtypes
  ///
  bool isCover() const;

 private:
  Value value_ = Value::CORE;
};

///
/// Defines the layer type
///
class dbTechLayerType
{
 public:
  enum Value
  {
    ROUTING,     /** */
    CUT,         /** */
    MASTERSLICE, /** */
    OVERLAP,     /** */
    IMPLANT,     /** */
    NONE         /** */
  };

  static constexpr Value DEFAULT = Value::ROUTING;

  static std::optional<Value> fromString(const char* value);

  ///
  /// Create a dbTechLayerType instance with an explicit placement status.
  /// The explicit status must be a string of one of the
  ///  following values: "routing", "cut", "masterslice", "overlap"
  ///
  dbTechLayerType(const char* value);

  ///
  /// Create a dbTechLayerType instance with an explicit value.
  ///
  dbTechLayerType(Value value) : value_(value) {}

  ///
  /// Create a dbTechLayerType instance with value = "routing".
  ///
  dbTechLayerType() = default;

  ///
  /// Copy constructor.
  ///
  dbTechLayerType(const dbTechLayerType& value) = default;

  ///
  /// Returns the layer-value.
  ///
  Value getValue() const { return value_; }

  ///
  /// Returns the layer-value as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return value_; }

 private:
  Value value_ = DEFAULT;
};

///
/// Defines the type of cell a master represents.
///
class dbTechLayerDir
{
 public:
  enum Value
  {
    NONE,       /** */
    HORIZONTAL, /** */
    VERTICAL    /** */
  };

  ///
  /// Create a dbTechLayerDir instance with an explicit placement status.
  /// The explicit status must be a string of one of the
  ///  following values: "none", "horizontal", "vertical"
  ///
  dbTechLayerDir(const char* direction);

  ///
  /// Create a dbTechLayerDir instance with an explicit direction.
  ///
  dbTechLayerDir(Value value) : value_(value) {}

  ///
  /// Create a dbTechLayerDir instance with direction = "none".
  ///
  dbTechLayerDir() = default;

  ///
  /// Copy constructor.
  ///
  dbTechLayerDir(const dbTechLayerDir& value) = default;

  ///
  /// Returns the layer-direction.
  ///
  Value getValue() const { return value_; }

  ///
  /// Returns the layer-direction as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return value_; }

 private:
  Value value_ = Value::NONE;
};

///
/// Defines the type of min step rule
///
class dbTechLayerMinStepType
{
 public:
  enum Value
  {
    INSIDE_CORNER,
    OUTSIDE_CORNER,
    STEP
  };

  ///
  /// Create a dbTechLayerMinStepType instance with an explicit type.
  /// The explicit type must be a string of one of the
  ///  following values: "INSIDECORNER", "OUTSIDECORNER", "STEP"
  ///
  dbTechLayerMinStepType(const char* type);

  ///
  /// Create a dbTechLayerMinStepType instance with an explicit type.
  ///
  dbTechLayerMinStepType(Value value) : value_(value) {}

  ///
  /// Create a dbTechLayerMinStepType instance with value = "OUTSIDE_CORNER".
  ///
  dbTechLayerMinStepType() = default;

  ///
  /// Copy constructor.
  ///
  dbTechLayerMinStepType(const dbTechLayerMinStepType& value) = default;

  ///
  /// Returns the layer-direction.
  ///
  Value getValue() const { return value_; }

  ///
  /// Returns the layer-direction as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return value_; }

 private:
  Value value_ = Value::OUTSIDE_CORNER;
};

///
/// Defines the type of cell a master represents.
///
class dbRowDir
{
 public:
  enum Value
  {
    HORIZONTAL, /** */
    VERTICAL    /** */
  };

  ///
  /// Create a dbRowDir instance with an explicit placement status.
  /// The explicit status must be a string of one of the
  ///  following values: "horizontal" or "vertical"
  ///
  dbRowDir(const char* direction);

  ///
  /// Create a dbRowDir instance with an explicit direction.
  ///
  dbRowDir(Value value) : _value(value) {}

  ///
  /// Create a dbRowDir instance with direction = "horizontal".
  ///
  dbRowDir() = default;

  ///
  /// Copy constructor.
  ///
  dbRowDir(const dbRowDir& value) = default;

  ///
  /// Returns the layer-direction.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the layer-direction as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::HORIZONTAL;
};

///
/// Defines the type of objects which own a dbBox.
///
class dbBoxOwner
{
 public:
  enum Value
  {
    UNKNOWN,
    BLOCK,
    INST,
    BTERM,
    VIA,
    OBSTRUCTION,
    SWIRE,
    BLOCKAGE,
    MASTER,
    MPIN,
    TECH_VIA,
    REGION,
    BPIN,
    PBOX
  };

  ///
  /// Create a dbBoxOwner instance with an explicit value.
  ///
  dbBoxOwner(const char* value);

  ///
  /// Create a dbBoxOwner instance with an explicit value.
  ///
  dbBoxOwner(Value value) : _value(value) {}

  ///
  /// Create a dbBoxOwner instance with value = BLOCK.
  ///
  dbBoxOwner() = default;

  ///
  /// Copy constructor.
  ///
  dbBoxOwner(const dbBoxOwner& value) = default;

  ///
  /// Returns the owner-value.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the owner-value as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::BLOCK;
};

///
/// Defines the type of objects which own a dbPolygon.
///
class dbPolygonOwner
{
 public:
  enum Value
  {
    UNKNOWN,
    BPIN,
    OBSTRUCTION,
    SWIRE,
  };

  ///
  /// Create a dbPolygonOwner instance with an explicit value.
  ///
  dbPolygonOwner(const char* value);

  ///
  /// Create a dbPolygonOwner instance with an explicit value.
  ///
  dbPolygonOwner(Value value) : _value(value) {}

  ///
  /// Create a dbPolygonOwner instance with value = UNKNOWN.
  ///
  dbPolygonOwner() = default;

  ///
  /// Copy constructor.
  ///
  dbPolygonOwner(const dbPolygonOwner& value) = default;

  ///
  /// Returns the owner-value.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the value as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::UNKNOWN;
};

///
/// Defines the type of wires.
///
class dbWireType
{
 public:
  enum Value
  {
    NONE,
    COVER,
    FIXED,
    ROUTED,
    SHIELD,
    NOSHIELD
  };

  ///
  /// Create a dbWireType instance with an explicit value.
  ///  Create a wire type of one of the following
  ///  values "cover", "fixed", "routed"
  ///
  dbWireType(const char* value);

  ///
  /// Create a dbWireType instance with an explicit value.
  ///
  dbWireType(Value value) : _value(value) {}

  ///
  /// Create a dbWireType instance with value = NONE
  ///
  dbWireType() = default;

  ///
  /// Copy constructor.
  ///
  dbWireType(const dbWireType& value) = default;

  ///
  /// Returns the type-value.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the owner-value as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::NONE;
};

///
/// Defines the type of shapes.
///
class dbWireShapeType
{
 public:
  enum Value
  {
    NONE,
    RING,
    PADRING,
    BLOCKRING,
    STRIPE,
    FOLLOWPIN,
    IOWIRE,
    COREWIRE,
    BLOCKWIRE,
    BLOCKAGEWIRE,
    FILLWIRE,
    DRCFILL
  };

  ///
  /// Create a dbWireShapeType instance with an explicit value.
  ///  Create a wire type of one of the following
  ///  values "ring", "padring", "blockring", "stripe",
  ///  "followpin", "iowire", "corewire", "blockwire", "blockagewire",
  ///  "fillwire", "drcfill"
  ///
  dbWireShapeType(const char* value);

  ///
  /// Create a dbWireShapeType instance with an explicit value.
  ///
  dbWireShapeType(Value value) : _value(value) {}

  ///
  /// Create a dbWireShapeType instance with value = RING
  ///
  dbWireShapeType() = default;

  ///
  /// Copy constructor.
  ///
  dbWireShapeType(const dbWireShapeType& value) = default;

  ///
  /// Returns the type-value.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the owner-value as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::NONE;
};

///
/// Defines the class of sites.
///
class dbSiteClass
{
 public:
  enum Value
  {
    NONE,
    PAD,
    CORE
  };

  ///
  /// Create a dbSiteClass instance with an explicit value.
  ///  Create a wire type of one of the following
  ///  values "pad", "core"
  ///
  dbSiteClass(const char* value);

  ///
  /// Create a dbSiteClass instance with an explicit value.
  ///
  dbSiteClass(Value value) : _value(value) {}

  ///
  /// Create a dbSiteClass instance with value = NONE
  ///
  dbSiteClass() = default;

  ///
  /// Copy constructor.
  ///
  dbSiteClass(const dbSiteClass& value) = default;

  ///
  /// Returns the type-value.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the owner-value as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::NONE;
};

//
//  For LEF Constructs comprising the values "ON" and "OFF"
//  These may be output by the LEF parser as a string or an int.
//
class dbOnOffType
{
 public:
  enum Value
  {
    OFF = 0,
    ON
  };

  ///
  /// Construction may take a string ("ON", "OFF"), an int (0 vs not 0),
  /// a bool, a type value, or default ("OFF")
  ///
  dbOnOffType(const char* instr);
  dbOnOffType(int innum) : _value((innum == 0) ? OFF : ON) {}
  dbOnOffType(bool insw) : _value(insw ? ON : OFF) {}
  dbOnOffType(Value inval) : _value(inval) {}
  dbOnOffType(const dbOnOffType& value) = default;
  dbOnOffType() = default;

  ///
  /// Returns the orientation as type, string, int (0,1), bool
  ///
  Value getValue() const { return _value; }
  const char* getString() const;
  int getAsInt() const;
  bool isSet() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::OFF;
};

//
//  For CLEARANCEMEASURE LEF construct
//
class dbClMeasureType
{
 public:
  enum Value
  {
    EUCLIDEAN = 0,
    MAXXY
  };

  ///
  /// Construction may take a string, a type value, or default ("EUCLIDEAN")
  ///
  dbClMeasureType(const char* instr);
  dbClMeasureType(Value inval) : _value(inval) {}
  dbClMeasureType(const dbClMeasureType& value) = default;
  dbClMeasureType() = default;

  ///
  /// Returns the orientation as type or string
  ///
  Value getValue() const { return _value; }
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::EUCLIDEAN;
};

//
// For DB journal entries, need to record actions (add/delete objects) and
// owner information
//
class dbJournalEntryType
{
 public:
  enum Value
  {
    NONE = 0,
    OWNER,
    ADD,
    DESTROY
  };

  ///
  /// Construction may take a type value, or default ("NONE")
  ///
  dbJournalEntryType(const char* instr);
  dbJournalEntryType(Value inval) : _value(inval) {}
  dbJournalEntryType(const dbJournalEntryType& value) = default;
  dbJournalEntryType() = default;

  ///
  /// Returns as type or string
  ///
  Value getValue() const { return _value; }
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::NONE;
};

//
//  Class to denote the four directions, in CW order
//
class dbDirection
{
 public:
  enum Value
  {
    NONE = 0,
    NORTH,
    EAST,
    SOUTH,
    WEST,
    UP,
    DOWN
  };

  ///
  /// Construction may take a type value, or default ("NONE")
  ///
  dbDirection(const char* instr);
  dbDirection(Value inval) : _value(inval) {}
  dbDirection(const dbDirection& value) = default;
  dbDirection() = default;

  ///
  /// Returns as type or string
  ///
  Value getValue() const { return _value; }
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::NONE;
};

//
//  Class to denote a region type
//
class dbRegionType
{
 public:
  enum Value
  {
    INCLUSIVE,
    EXCLUSIVE,  // corresponds to DEF FENCE type.
    SUGGESTED   // corresponds to DEF GUIDE type.
  };

  ///
  /// Construction may take a type value, or default ("INCLUSIVE")
  ///
  dbRegionType(const char* instr);
  dbRegionType(Value inval) : _value(inval) {}
  dbRegionType(const dbRegionType& value) = default;
  dbRegionType() = default;
  Value getValue() const { return _value; }
  const char* getString() const;
  operator Value() const { return _value; }

 private:
  Value _value = Value::INCLUSIVE;
};

//
//  DEF Source Type
//
class dbSourceType
{
 public:
  enum Value
  {
    NONE,
    NETLIST,
    DIST,
    USER,
    TIMING,
    TEST
  };

  ///
  /// Construction may take a type value, or default ("NONE")
  ///
  dbSourceType(const char* value);
  dbSourceType(Value inval) : _value(inval) {}
  dbSourceType(const dbSourceType& value) = default;
  dbSourceType() = default;
  Value getValue() const { return _value; }
  const char* getString() const;
  operator Value() const { return _value; }

 private:
  Value _value = Value::NONE;
};

// TODO: shouldn't these come from <climits> ?
inline constexpr uint64_t MAX_UINT64 = 0xffffffffffffffffLL;
inline constexpr uint64_t MIN_UINT64 = 0;
inline constexpr uint32_t MAX_UINT = 0xffffffff;
inline constexpr uint32_t MIN_UINT = 0;

inline constexpr int64_t MAX_INT64 = 0x7fffffffffffffffLL;
inline constexpr int64_t MIN_INT64 = 0x8000000000000000LL;
inline constexpr int MAX_INT = 0x7fffffff;
inline constexpr int MIN_INT = 0x80000000;

///
/// Defines the type of shapes.
///
class dbMTermShapeType
{
 public:
  enum Value
  {
    NONE,
    RING,
    FEEDTHRU,
    ABUTMENT
  };

  ///
  ///  Create a dbMterm type of one of the following
  ///  values "ring", "feedthru", "abutment"
  ///
  dbMTermShapeType(const char* value);

  ///
  /// Create a dbMTermShapeType with an explicit value.
  ///
  dbMTermShapeType(Value value) : _value(value) {}

  ///
  /// Create a dbMTermShapeType instance with value = NONE
  ///
  dbMTermShapeType() = default;

  ///
  /// Copy constructor.
  ///
  dbMTermShapeType(const dbMTermShapeType& value) = default;

  ///
  /// Returns the type-value.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the owner-value as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::NONE;
};

///
/// The orientation define the rotation and axis mirroring for the
/// placement of various database objects. The values conform the orient
/// definitions defined in the LEF/DEF formats.
///
class dbAccessType
{
 public:
  enum Value : int8_t
  {
    OnGrid,
    HalfGrid,
    Center,
    EncOpt,
    NearbyGrid
  };

  ///
  /// Create a dbAccessType instance with an explicit type.
  /// The explicit type must be a string of one of the
  ///  following values: "OnGrid", "HalfGrid", "Center", "EncOpt",
  ///  "NearbyGrid"
  dbAccessType(const char* type);

  ///
  /// Create a dbAccessType instance with an explicit type.
  ///
  dbAccessType(Value type) : _value(type) {}

  ///
  /// Create a dbAccessType instance with type "OnGrid".
  ///
  dbAccessType() = default;

  ///
  /// Copy constructor.
  ///
  dbAccessType(const dbAccessType& value) = default;

  ///
  /// Returns the type
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the type as a string
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value = Value::OnGrid;
};

//
//  Class to denote a name uniquify type
//
class dbNameUniquifyType
{
 public:
  enum Value
  {
    ALWAYS,                     // Add unique suffix always
    ALWAYS_WITH_UNDERSCORE,     // Add unique suffix with underscore always
    IF_NEEDED,                  // Add unique suffix if needed
    IF_NEEDED_WITH_UNDERSCORE,  // Add unique suffix with underscore if needed
  };

  ///
  /// Construction may take a type value, or default ("ALWAYS")
  ///
  dbNameUniquifyType(const char* instr);
  dbNameUniquifyType(Value inval) : _value(inval) {}
  dbNameUniquifyType(const dbNameUniquifyType& value) = default;
  dbNameUniquifyType() = default;
  Value getValue() const { return _value; }
  const char* getString() const;
  operator Value() const { return _value; }

 private:
  Value _value = Value::ALWAYS;
};

//
//  Class to denote hierarchical search direction
//
class dbHierSearchDir
{
 public:
  enum Value
  {
    FANIN,
    FANOUT
  };

  ///
  /// Construction may take a type value, or default ("FANOUT")
  ///
  dbHierSearchDir(Value inval) : _value(inval) {}
  dbHierSearchDir(const dbHierSearchDir& value) = default;
  dbHierSearchDir() = default;
  Value getValue() const { return _value; }
  const char* getString() const;
  operator Value() const { return _value; }

 private:
  Value _value = Value::FANOUT;
};

}  // namespace odb
