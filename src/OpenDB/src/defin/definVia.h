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
#include "db.h"
#include "definBase.h"

namespace odb {

class dbVia;

class definVia : public definBase
{
  dbVia*       _cur_via;
  dbViaParams* _params;

 public:
  // Via interface methods
  virtual void viaBegin(const char* name);
  virtual void viaRule(const char* rule);
  virtual void viaCutSize(int xSize, int ySize);
  virtual bool viaLayers(const char* bottom, const char* cut, const char* top);
  virtual void viaCutSpacing(int xSpacing, int ySpacing);
  virtual void viaEnclosure(int xBot, int yBot, int xTop, int yTop);
  virtual void viaRowCol(int numCutRows, int numCutCols);
  virtual void viaOrigin(int xOffset, int yOffset);
  virtual void viaOffset(int xBot, int yBot, int xTop, int yTop);

  virtual void viaPattern(const char* pattern);
  virtual void viaRect(const char* layer, int x1, int y1, int x2, int y2);
  virtual void viaEnd();

  definVia();
  virtual ~definVia();
  void init();
};

}  // namespace odb


