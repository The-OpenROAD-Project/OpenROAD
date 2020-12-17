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

#include "misc_global.h"

#define def_dbg

#define MAX32 32

//#ifdef _WIN32
typedef unsigned int uint;
//#endif

#define ath__maxInt 2100000000

#define ath__min(a, b) ((a) < (b)) ? (a) : (b);
#define ath__max(a, b) ((a) > (b)) ? (a) : (b);

enum Ath__termDirection
{
  INPUT = 0,
  OUTPUT,
  INOUT,
  ALL
};
enum Ath__signalType
{
  SIGNAL = 0,
  CLOCK,
  GROUND,
  POWER
};
enum Ath__cornerType
{
  LL,
  LR,
  UL,
  UR
};

enum Ath__hierType
{
  Ath_hier__chip = 0,
  Ath_hier__inst,
  Ath_hier__block,
  Ath_hier__macro,
  Ath_hier__cell,
  Ath_hier__power,
  Ath_hier__signal,
  Ath_hier__tnet,
  Ath_hier__tile,
  Ath_hier__groute
};

enum Ath__boxType
{
  Ath_box__zbox = 0,
  Ath_box__bbox,
  Ath_box__wire,
  Ath_box__via,
  Ath_box__obs,
  Ath_box__track,
  Ath_box__bus,
  Ath_box__pin,
  Ath_box__gcell,
  Ath_box__gpath,
  Ath_box__spots,
  Ath_box__white,
  Ath_box__tcut,
  Ath_box__bcut
};

enum Ath__zuiAction
{
  Ath_gui__self = 0,
  Ath_gui__connectivity,
  Ath_gui__connectPlus,
  Ath_gui__flightLines,
  Ath_gui__highlight,
  Ath_gui__select
};
enum Ath__zuiMark
{
  Ath_mark__none = 0,
  Ath_mark__main,
  Ath_mark__signalWire,
  Ath_mark__powerWire,
  Ath_mark__instConn,
  Ath_mark__netConn,
  Ath_mark__btermConn,
  Ath_mark__pinTarget,
  Ath_mark__pinGravity,
  Ath_mark__pinPlaced,
  Ath_mark__busPlaced,
  Ath_mark__tile,
  Ath_mark__context
};


