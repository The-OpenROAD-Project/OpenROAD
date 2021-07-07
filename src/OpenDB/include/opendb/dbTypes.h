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

#pragma once

#include "odb.h"

namespace odb {

///
/// This file declares the non-persistant database objects and misc.
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

  ///
  /// Create a dbOrientType instance with an explicit orientation..
  /// The explicit orientation must be a string of one of the
  ///  following values: "R0", "R90", "R180", "270", "MY", "MX", "MYR90",
  ///  or "MXR90".
  ///
  dbOrientType(const char* orient);

  ///
  /// Create a dbOrientType instance with an explicit orientation..
  ///
  dbOrientType(Value orient);

  ///
  /// Create a dbOrientType instance with orientation "R0".
  ///
  dbOrientType();

  ///
  /// Copy constructor.
  ///
  dbOrientType(const dbOrientType& orient);

  ///
  /// Returns the orientation
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the orientation as a string
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

  ///
  /// Returns the orientation after flipping about the x-axis
  ///
  dbOrientType flipX() const;

  ///
  /// Returns the orientation after flipping about the y-axis
  ///
  dbOrientType flipY() const;

 private:
  Value _value;
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
  /// Create a dbSigType instance with an explicit signal value..
  ///
  dbSigType(Value value);

  ///
  /// Create a dbSigType instance with value "signal".
  ///
  dbSigType();

  ///
  /// Copy constructor.
  ///
  dbSigType(const dbSigType& value);

  ///
  /// Returns the signal-value
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the signal-value as a string.
  ///
  const char* getString() const;

  ///
  ///  True Iff value corresponds to POWER or GROUND
  ///
  bool isSupply() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value;
};

///
/// Specifies the functional electical behavior of a terminal.
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
  /// Create a dbIoType instance with an explicit io-direction..
  /// The explicit io-direction must be a string of one of the
  ///  following values: "input", "output", "inout"
  ///
  dbIoType(const char* value);

  ///
  /// Create a dbIoType instance with an explicit IO direction..
  ///
  dbIoType(Value value);

  ///
  /// Create a dbIoType instance with value "input".
  ///
  dbIoType();

  ///
  /// Copy constructor.
  ///
  dbIoType(const dbIoType& value);

  ///
  /// Returns the direction of IO of an element.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the direction of IO of an element as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value;
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
  /// Create a dbPlacementStatus instance with an explicit placement status..
  /// The explicit status must be a string of one of the
  ///  following values: "none", "unplaced", "suggested", "placed",
  ///  "locked", "firm", "cover".
  ///
  dbPlacementStatus(const char* value);

  ///
  /// Create a dbPlacementStatus instance with an explicit status.
  ///
  dbPlacementStatus(Value value);

  ///
  /// Create a dbPlacementStatus instance with status = "none".
  ///
  dbPlacementStatus();

  ///
  /// Copy constructor.
  ///
  dbPlacementStatus(const dbPlacementStatus& value);

  ///
  /// Returns the placement status.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the placement status as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

  ///
  ///  True Iff value corresponds to a PLACED, LOCKED, FIRM, or COVER
  ///
  bool isPlaced() const;

  ///
  ///  True Iff value corresponds to LOCKED, FIRM, or COVER
  ///
  bool isFixed() const;

 private:
  Value _value;
};

///
/// Defines the value of cell a master represents.
///
class dbMasterType
{
 public:
  enum Value
  {
    NONE,              /** */
    COVER,             /** */
    COVER_BUMP,        /** */
    RING,              /** */
    BLOCK,             /** */
    BLOCK_BLACKBOX,    /** */
    BLOCK_SOFT,        /** */
    PAD,               /** */
    PAD_INPUT,         /** */
    PAD_OUTPUT,        /** */
    PAD_INOUT,         /** */
    PAD_POWER,         /** */
    PAD_SPACER,        /** */
    PAD_AREAIO,        /** */
    CORE,              /** */
    CORE_FEEDTHRU,     /** */
    CORE_TIEHIGH,      /** */
    CORE_TIELOW,       /** */
    CORE_SPACER,       /** */
    CORE_ANTENNACELL,  /** */
    CORE_WELLTAP,      /** */
    ENDCAP,            /** */
    ENDCAP_PRE,        /** */
    ENDCAP_POST,       /** */
    ENDCAP_TOPLEFT,    /** */
    ENDCAP_TOPRIGHT,   /** */
    ENDCAP_BOTTOMLEFT, /** */
    ENDCAP_BOTTOMRIGHT /** */
  };

  ///
  /// Create a dbMasterType instance with an explicit placement status..
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
  dbMasterType(Value value);

  ///
  /// Create a dbMasterType instance with value = "none".
  ///
  dbMasterType();

  ///
  /// Copy constructor.
  ///
  dbMasterType(const dbMasterType& value);

  ///
  /// Returns the master-value.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the master-value as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

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

 private:
  Value _value;
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

  ///
  /// Create a dbTechLayerType instance with an explicit placement status..
  /// The explicit status must be a string of one of the
  ///  following values: "routing", "cut", "masterslice", "overlap"
  ///
  dbTechLayerType(const char* value);

  ///
  /// Create a dbTechLayerType instance with an explicit value.
  ///
  dbTechLayerType(Value value);

  ///
  /// Create a dbTechLayerType instance with value = "none".
  ///
  dbTechLayerType();

  ///
  /// Copy constructor.
  ///
  dbTechLayerType(const dbTechLayerType& value);

  ///
  /// Returns the layer-value.
  ///
  Value getValue() const { return _value; }

  ///
  /// Returns the layer-value as a string.
  ///
  const char* getString() const;

  ///
  /// Cast operator
  ///
  operator Value() const { return _value; }

 private:
  Value _value;
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
  /// Create a dbTechLayerDir instance with an explicit placement status..
  /// The explicit status must be a string of one of the
  ///  following values: "none", "horizontal", "vertical"
  ///
  dbTechLayerDir(const char* direction);

  ///
  /// Create a dbTechLayerDir instance with an explicit direction.
  ///
  dbTechLayerDir(Value value);

  ///
  /// Create a dbTechLayerDir instance with direction = "none".
  ///
  dbTechLayerDir();

  ///
  /// Copy constructor.
  ///
  dbTechLayerDir(const dbTechLayerDir& value);

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
  Value _value;
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
  dbTechLayerMinStepType(Value value);

  ///
  /// Create a dbTechLayerMinStepType instance with value = "OUTSIDE_CORNER".
  ///
  dbTechLayerMinStepType();

  ///
  /// Copy constructor.
  ///
  dbTechLayerMinStepType(const dbTechLayerMinStepType& value);

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
  Value _value;
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
  /// Create a dbTechLayerDir instance with an explicit placement status..
  /// The explicit status must be a string of one of the
  ///  following values: "horizontal" or "vertical"
  ///
  dbRowDir(const char* direction);

  ///
  /// Create a dbTechLayerDir instance with an explicit direction.
  ///
  dbRowDir(Value value);

  ///
  /// Create a dbTechLayerDir instance with direction = "none".
  ///
  dbRowDir();

  ///
  /// Copy constructor.
  ///
  dbRowDir(const dbRowDir& value);

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
  Value _value;
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
    BPIN
  };

  ///
  /// Create a dbBoxOwner instance with an explicit value.
  ///
  dbBoxOwner(const char* value);

  ///
  /// Create a dbBoxOwner instance with an explicit value.
  ///
  dbBoxOwner(Value value);

  ///
  /// Create a dbBoxOwner instance with value = UNKNOWN.
  ///
  dbBoxOwner();

  ///
  /// Copy constructor.
  ///
  dbBoxOwner(const dbBoxOwner& value);

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
  Value _value;
};

///
/// Defines the type of objects which own a dbBox.
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
  /// Create a dbBoxOwner instance with an explicit value.
  ///
  dbPolygonOwner(Value value);

  ///
  /// Create a dbBoxOwner instance with value = UNKNOWN.
  ///
  dbPolygonOwner();

  ///
  /// Copy constructor.
  ///
  dbPolygonOwner(const dbBoxOwner& value);

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
  Value _value;
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
  ///  Create a wire type of one of the following
  ///  values "cover", "fixed", "routed"
  ///
  dbWireType(const char* value);

  ///
  /// Create a dbBoxOwner instance with an explicit value.
  ///
  dbWireType(Value value);

  ///
  /// Create a dbWireType instance with value = NONE
  ///
  dbWireType();

  ///
  /// Copy constructor.
  ///
  dbWireType(const dbWireType& value);

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
  Value _value;
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
  ///  Create a wire type of one of the following
  ///  values "ring", "padring", "blockring", "stripe",
  ///  "followpin", "iowire", "corewire", "blockwire", "blockagewire",
  ///  "fillwire", "drcfill"
  ///
  dbWireShapeType(const char* value);

  ///
  /// Create a dbBoxOwner instance with an explicit value.
  ///
  dbWireShapeType(Value value);

  ///
  /// Create a dbWireType instance with value = RING
  ///
  dbWireShapeType();

  ///
  /// Copy constructor.
  ///
  dbWireShapeType(const dbWireShapeType& value);

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
  Value _value;
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
  ///  Create a wire type of one of the following
  ///  values "pad", "core"
  ///
  dbSiteClass(const char* value);

  ///
  /// Create a dbSiteClass instance with an explicit value.
  ///
  dbSiteClass(Value value);

  ///
  /// Create a dbSiteClass instance with value = NONE
  ///
  dbSiteClass();

  ///
  /// Copy constructor.
  ///
  dbSiteClass(const dbSiteClass& value);

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
  Value _value;
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
  dbOnOffType(int innum);
  dbOnOffType(bool insw);
  dbOnOffType(Value inval);
  dbOnOffType();

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
  Value _value;
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
  dbClMeasureType(Value inval) { _value = inval; }
  dbClMeasureType() { _value = EUCLIDEAN; }

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
  Value _value;
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
  dbJournalEntryType(Value inval) { _value = inval; }
  dbJournalEntryType() { _value = NONE; }

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
  Value _value;
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
    WEST
  };

  ///
  /// Construction may take a type value, or default ("NONE")
  ///
  dbDirection(Value inval) { _value = inval; }
  dbDirection() { _value = NONE; }

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
  Value _value;
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
    EXCLUSIVE,  // corrsponds to DEF FENCE type.
    SUGGESTED   // corrsponds to DEF GUIDE type.
  };

  dbRegionType(Value inval) { _value = inval; }
  dbRegionType() { _value = INCLUSIVE; }
  Value getValue() const { return _value; }
  const char* getString() const;
  operator Value() const { return _value; }

 private:
  Value _value;
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

  dbSourceType(const char* value);
  dbSourceType(Value inval) { _value = inval; }
  dbSourceType() { _value = NONE; }
  Value getValue() const { return _value; }
  const char* getString() const;
  operator Value() const { return _value; }

 private:
  Value _value;
};

const uint64 MAX_UINT64 = 0xffffffffffffffffLL;
const uint64 MIN_UINT64 = 0;
const uint MAX_UINT = 0xffffffff;
const uint MIN_UINT = 0;

const int64 MAX_INT64 = 0x7fffffffffffffffLL;
const int64 MIN_INT64 = 0x8000000000000000LL;
const int MAX_INT = 0x7fffffff;
const int MIN_INT = 0x80000000;

//
// Adding this to the inheritance list of your class causes your class (and
// its descendants) to be non-copy.
//
class Ads_NoCopy
{
 public:
  Ads_NoCopy() {}
  Ads_NoCopy(const Ads_NoCopy& q);
  Ads_NoCopy& operator=(const Ads_NoCopy& q);
};

}  // namespace odb
