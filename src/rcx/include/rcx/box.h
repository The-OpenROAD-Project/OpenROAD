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

#include <cstdio>
#include <cstring>

#include "odb/geom.h"
#include "odb/util.h"

namespace rcx {

using uint = unsigned int;

class Box
{
 public:
  Box();
  Box(int x1, int y1, int x2, int y2, int units = 1);

  uint getDir() const;
  int getYhi(int bound) const;
  int getXhi(int bound) const;
  int getXlo(int bound) const;
  int getYlo(int bound) const;
  uint getWidth(uint* dir) const;
  uint getDX() const;
  uint getDY() const;
  uint getLength() const;
  uint getOwner() const;
  uint getLayer() const { return _layer; }
  uint getId() const { return _id; }
  odb::Rect getRect() const { return _rect; }
  void setRect(const odb::Rect& rect) { _rect = rect; }

  void invalidateBox();
  void set(Box* bb);
  void set(int x1, int y1, int x2, int y2, int units = 1);
  void setLayer(uint layer) { _layer = layer; }

 private:
  uint _layer : 4;
  uint _valid : 1;
  uint _id : 27;

  odb::Rect _rect;
};

}  // namespace rcx
