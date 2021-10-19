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

#include <stdio.h>
#include <string.h>

#include "atypes.h"
#include "util.h"

bool Ath__intersect(int X1, int DX, int x1, int dx, int* ix1, int* ix2);

// TODO center line with width and length and direction

class Ath__box
{
 public:
  uint _layer : 4;
  uint _valid : 1;
  uint _id : 28;

 public:
  int _xlo;
  int _ylo;
  int _xhi;
  int _yhi;
  Ath__box* _next;

 public:
  Ath__box();
  uint getDir();
  int getYhi(int bound);
  int getXhi(int bound);
  int getXlo(int bound);
  int getYlo(int bound);
  uint getWidth(uint* dir);
  Ath__box(int x1, int y1, int x2, int y2, uint units = 1);
  void set(int x1, int y1, int x2, int y2, uint units = 1);
  uint getDX();
  uint getDY();
  uint getLength();
  void invalidateBox();
  void set(Ath__box* bb);
  bool outside(int x1, int y1, int x2, int y2);
  uint getOwner();
};
