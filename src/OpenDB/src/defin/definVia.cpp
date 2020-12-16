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

#include "definVia.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "db.h"
#include "dbShape.h"

namespace odb {

definVia::definVia()
{
  init();
}

definVia::~definVia()
{
}

void definVia::init()
{
  definBase::init();
  _cur_via = NULL;
  _params  = NULL;
}

void definVia::viaBegin(const char* name)
{
  assert(_cur_via == NULL);

  _cur_via = dbVia::create(_block, name);

  if (_cur_via == NULL) {
    notice(0, "error: duplicate via (%s)\n", name);
    ++_errors;
  }
}

void definVia::viaRule(const char* rule)
{
  if (_cur_via == NULL)
    return;

  dbTechViaGenerateRule* viarule = _tech->findViaGenerateRule(rule);

  if (viarule == NULL) {
    notice(
        0, "error: cannot file VIA GENERATE rule in technology (%s).\n", rule);
    ++_errors;
    return;
  }

  _cur_via->setViaGenerateRule(viarule);
}

void definVia::viaCutSize(int xSize, int ySize)
{
  if (_cur_via == NULL)
    return;

  if (_params == NULL)
    _params = new dbViaParams();

  _params->setXCutSize(dbdist(xSize));
  _params->setYCutSize(dbdist(ySize));
}

bool definVia::viaLayers(const char* botName,
                        const char* cutName,
                        const char* topName)
{
  if (_cur_via == NULL)
    return false;

  if (_params == NULL)
    _params = new dbViaParams();

  dbTechLayer* bot = _tech->findLayer(botName);

  if (bot == NULL) {
    notice(0, "error: undefined layer (%s) referenced\n", botName);
    ++_errors;
    return false;
  }

  dbTechLayer* cut = _tech->findLayer(cutName);

  if (cut == NULL) {
    notice(0, "error: undefined layer (%s) referenced\n", cutName);
    ++_errors;
    return false;
  }

  dbTechLayer* top = _tech->findLayer(topName);

  if (top == NULL) {
    notice(0, "error: undefined layer (%s) referenced\n", topName);
    ++_errors;
    return false;
  }

  _params->setTopLayer(top);
  _params->setBottomLayer(bot);
  _params->setCutLayer(cut);
  return true;
}

void definVia::viaCutSpacing(int xSpacing, int ySpacing)
{
  if (_cur_via == NULL)
    return;

  if (_params == NULL)
    _params = new dbViaParams();

  _params->setXCutSpacing(dbdist(xSpacing));
  _params->setYCutSpacing(dbdist(ySpacing));
}

void definVia::viaEnclosure(int xBot, int yBot, int xTop, int yTop)
{
  if (_cur_via == NULL)
    return;

  if (_params == NULL)
    _params = new dbViaParams();

  _params->setXBottomEnclosure(dbdist(xBot));
  _params->setYBottomEnclosure(dbdist(yBot));
  _params->setXTopEnclosure(dbdist(xTop));
  _params->setYTopEnclosure(dbdist(yTop));
}

void definVia::viaRowCol(int numCutRows, int numCutCols)
{
  if (_cur_via == NULL)
    return;

  if (_params == NULL)
    _params = new dbViaParams();

  _params->setNumCutRows(numCutRows);
  _params->setNumCutCols(numCutCols);
}

void definVia::viaOrigin(int x, int y)
{
  if (_cur_via == NULL)
    return;

  if (_params == NULL)
    _params = new dbViaParams();

  _params->setXOrigin(dbdist(x));
  _params->setYOrigin(dbdist(y));
}

void definVia::viaOffset(int xBot, int yBot, int xTop, int yTop)
{
  if (_cur_via == NULL)
    return;

  if (_params == NULL)
    _params = new dbViaParams();

  _params->setXBottomOffset(dbdist(xBot));
  _params->setYBottomOffset(dbdist(yBot));
  _params->setXTopOffset(dbdist(xTop));
  _params->setYTopOffset(dbdist(yTop));
}

void definVia::viaPattern(const char* pattern)
{
  if (_cur_via == NULL)
    return;

  _cur_via->setPattern(pattern);
}

void definVia::viaRect(const char* layer_name, int x1, int y1, int x2, int y2)
{
  if (_cur_via == NULL)
    return;

  x1 = dbdist(x1);
  y1 = dbdist(y1);
  x2 = dbdist(x2);
  y2 = dbdist(y2);

  dbTechLayer* layer = _tech->findLayer(layer_name);

  if (layer == NULL) {
    notice(0, "error: undefined layer (%s) referenced\n", layer_name);
    ++_errors;
    return;
  }

  dbBox::create(_cur_via, layer, x1, y1, x2, y2);
}

void definVia::viaEnd()
{
  if (_cur_via == NULL)
    return;

  if (_params) {
    _cur_via->setViaParams(*_params);
    delete _params;
    _params = NULL;
  }

  dbSet<dbBox> boxes = _cur_via->getBoxes();

  if (boxes.reversible() && boxes.orderReversed())
    boxes.reverse();

  _cur_via = NULL;
}

}  // namespace odb
