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
 private:
  uint _ownerId : 23;   // owner id
  uint _ownerType : 4;  // hier type
 public:
  uint _layer : 4;
  uint _valid : 1;
  uint _clip : 1;
  uint _id : 28;
  uint _type : 3;

 public:
  int       _xlo;
  int       _ylo;
  int       _xhi;
  int       _yhi;
  Ath__box* _next;

 public:
  Ath__box();
  uint getLen();
  uint getDir();
  int  getYhi(int bound);
  int  getXhi(int bound);
  int  getXlo(int bound);
  int  getYlo(int bound);
  int  getMidY(int loBound, int hiBound);
  int  getMidX(int loBound, int hiBound);
  void addExtra(uint dd);
  uint getWidth(uint* dir);
  uint getStats(uint  level,
                uint* dir,
                uint* minWidth,
                uint* maxWidth,
                uint  minLen);
  void writeCoordsDB(FILE* fp);
  void readCoordsDB(FILE* fp);
  uint boxCnt();
  void writeListDB(FILE* fp);
  void deallocList();
  Ath__box(int x1, int y1, int x2, int y2, uint units = 1);
  void set(int x1, int y1, int x2, int y2, uint units = 1);
  uint getDX();
  uint getDY();
  int  getMidX();
  int  getMidY();
  void printDefXYlo(FILE* fp);
  uint getLength();
  void setIdentity(Ath__box* bb);
  void setIdentity(uint type, uint ownId, uint ownType);
  void invalidateBox();
  void setClipFlag(bool flag);
  void resetMaxBox();
  void pinBoxDef(FILE* fp,
                 char* layerName,
                 int   x,
                 int   y,
                 char* orient,
                 int   defUnits);
  void printPoints(FILE* fp, uint defUnits);
  void setMaxBox(int x1, int y1, int x2, int y2);
  void set(Ath__box* bb);
  void getTransformed(Ath__box* bb, int origX, int origY, char* orient = NULL);
  void setMaxBox(Ath__box* bb);
  bool outside(int x1, int y1, int x2, int y2);
  bool clip(int* x1, int* y1, int* x2, int* y2);
  bool inside(int x1, int y1, int x2, int y2);
  uint getOwner();
  uint getOwnerType();
  uint getBoxType();
  void setOwner(uint id);

  friend class Ath__bterm;
};

class Ath__trans
{
 public:
  int  _origX;
  int  _origY;
  char _orient[3];

  void set(int x, int y, char* orient);
  void transform(Ath__box* bb);
  int  getX(int x);
  int  getY(int y);
};


