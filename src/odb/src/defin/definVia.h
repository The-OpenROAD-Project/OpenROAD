// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "definBase.h"
#include "odb/db.h"

namespace odb {

class dbVia;

class definVia : public definBase
{
  dbVia* _cur_via{nullptr};
  dbViaParams* _params{nullptr};

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
};

}  // namespace odb
