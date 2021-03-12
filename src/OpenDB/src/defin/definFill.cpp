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

#include "definFill.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "dbShape.h"
#include "utility/Logger.h"
namespace odb {

definFill::definFill()
{
  _cur_layer = NULL;
}

definFill::~definFill()
{
}

void definFill::init()
{
  definBase::init();
  _cur_layer = NULL;
}

void definFill::fillBegin(const char* layer, bool needs_opc, int mask_number)
{
  _cur_layer = _tech->findLayer(layer);

  if (_cur_layer == NULL) {
    _logger->warn(
        utl::ODB, 95, "error: undefined layer ({}) referenced", layer);
    ++_errors;
  }
  _mask_number = mask_number;
  _needs_opc = needs_opc;
}

void definFill::fillRect(int x1, int y1, int x2, int y2)
{
  x1 = dbdist(x1);
  y1 = dbdist(y1);
  x2 = dbdist(x2);
  y2 = dbdist(y2);
  dbFill::create(_block, _needs_opc, _mask_number, _cur_layer, x1, y1, x2, y2);
}

void definFill::fillPolygon(std::vector<Point>& /* unused: points */)
{
}

void definFill::fillEnd()
{
}

}  // namespace odb
