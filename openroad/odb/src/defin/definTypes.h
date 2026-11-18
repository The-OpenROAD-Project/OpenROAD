// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

enum defOrient
{
  DEF_ORIENT_OLD_N,
  DEF_ORIENT_OLD_S,
  DEF_ORIENT_OLD_E,
  DEF_ORIENT_OLD_W,
  DEF_ORIENT_OLD_FN,
  DEF_ORIENT_OLD_FS,
  DEF_ORIENT_OLD_FE,
  DEF_ORIENT_OLD_FW
};

enum defPlacement
{
  DEF_PLACEMENT_FIXED,
  DEF_PLACEMENT_COVER,
  DEF_PLACEMENT_PLACED,
  DEF_PLACEMENT_UNPLACED
};

enum defWireType
{
  DEF_WIRE_COVER,
  DEF_WIRE_FIXED,
  DEF_WIRE_ROUTED,
  DEF_WIRE_NOSHIELD,
  DEF_WIRE_SHIELD
};

enum defSigType
{
  DEF_SIG_ANALOG,
  DEF_SIG_CLOCK,
  DEF_SIG_GROUND,
  DEF_SIG_POWER,
  DEF_SIG_RESET,
  DEF_SIG_SCAN,
  DEF_SIG_SIGNAL,
  DEF_SIG_TIEOFF
};

enum defIoType
{
  DEF_IO_INPUT,
  DEF_IO_OUTPUT,
  DEF_IO_INOUT,
  DEF_IO_FEEDTHRU
};

enum defDirection
{
  DEF_X,
  DEF_Y
};

enum defRow
{
  DEF_VERTICAL,
  DEF_HORIZONTAL
};

enum defRegionType
{
  DEF_FENCE,
  DEF_GUIDE
};

enum defSource
{
  DEF_DIST,
  DEF_NETLIST,
  DEF_TEST,
  DEF_TIMING,
  DEF_USER
};

enum defObjectType
{
  DEF_COMPONENT,
  DEF_COMPONENTPIN,
  DEF_DESIGN,
  DEF_GROUP,
  DEF_NET,
  DEF_REGION,
  DEF_ROW,
  DEF_SPECIALNET,
  DEF_NONDEFAULTRULE
};

enum defPropType
{
  DEF_INTEGER,
  DEF_REAL,
  DEF_STRING
};

struct defPoint
{
  int _x;
  int _y;

  defPoint() = default;
  defPoint(int x, int y) : _x(x), _y(y) {}
};
