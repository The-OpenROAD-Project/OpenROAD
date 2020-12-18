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

  defPoint() {}

  defPoint(int x, int y) : _x(x), _y(y) {}
};


