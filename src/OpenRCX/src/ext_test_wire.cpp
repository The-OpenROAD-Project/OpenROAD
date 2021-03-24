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
#include <dbShape.h>
#include <dbWireCodec.h>
#include <stdio.h>

#include "db.h"
#include "OpenRCX/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

static odb::dbTechLayer* m1;
static odb::dbTechLayer* m2;
static odb::dbTechLayer* m3;
static odb::dbTechVia* v12;
static odb::dbTechVia* v23;

static void create_tech(odb::dbDatabase* db)
{
  odb::dbTech* tech = odb::dbTech::create(db);

  m1 = odb::dbTechLayer::create(tech, "M1", (odb::dbTechLayerType::ROUTING));
  m1->setWidth(2000);
  m2 = odb::dbTechLayer::create(tech, "M2", (odb::dbTechLayerType::ROUTING));
  m2->setWidth(2000);
  m3 = odb::dbTechLayer::create(tech, "M3", (odb::dbTechLayerType::ROUTING));
  m3->setWidth(2000);

  v12 = odb::dbTechVia::create(tech, "VIA12");
  odb::dbBox::create(v12, m1, -1000, -1000, 1000, 1000);
  odb::dbBox::create(v12, m2, -1000, -1000, 1000, 1000);

  v23 = odb::dbTechVia::create(tech, "VIA23");
  odb::dbBox::create(v23, m2, -1000, -1000, 1000, 1000);
  odb::dbBox::create(v23, m3, -1000, -1000, 1000, 1000);
}
/*
static uint print_shape( odb::dbShape & shape, uint j1, uint j2)
{
        uint dx = shape.xMax() - shape.xMin();
        uint dy = shape.yMax() - shape.yMin();
    if ( shape.isVia() )
    {
        odb::dbTechVia * tech_via = shape.getTechVia();
        odb::dbString vname = tech_via ->getName();

        notice(0, "VIA %s ( %d %d )  jids= ( %d %d )\n",
               vname.c_str(),
               shape.xMin() + dx / 2,
               shape.yMin() + dy / 2, j1, j2
               );
    }
    else
    {
        odb::dbTechLayer * layer = shape.getTechLayer();
        odb::dbString lname = layer->getName();
        notice(0, "RECT %s ( %d %d ) ( %d %d )  jids= ( %d %d )\n",
               lname.c_str(),
               shape.xMin(),
               shape.yMin(),
               shape.xMax(),
               shape.yMax(), j1, j2 );

                if (dx<dy)
                        return dy;
                else
                        return dx;
    }
        return 0;
}
*/

int extMain::db_test_wires(odb::dbDatabase* db)
{
  if (db == NULL)
    db = _db;

  create_tech(db);
  odb::dbChip* chip = odb::dbChip::create(db);
  odb::dbBlock* block = odb::dbBlock::create(chip, "chip");
  odb::dbNet* net = odb::dbNet::create(block, "net");
  odb::dbWire* wire = odb::dbWire::create(net);
  odb::dbWireEncoder encoder;
  encoder.begin(wire);
  encoder.newPath(m1, odb::dbWireType::ROUTED);
  encoder.addPoint(2000, 2000);
  int j1 = encoder.addPoint(10000, 2000);
  encoder.addPoint(18000, 2000);
  encoder.newPath(j1);
  encoder.addTechVia(v12);
  int j2 = encoder.addPoint(10000, 10000);
  encoder.addPoint(10000, 18000);
  encoder.newPath(j2);
  int j3 = encoder.addTechVia(v12);
  encoder.addPoint(23000, 10000, 4000);
  encoder.newPath(j3);
  encoder.addPoint(3000, 10000);
  encoder.addTechVia(v12);
  encoder.addTechVia(v23);
  encoder.addPoint(3000, 10000, 4000);
  encoder.addPoint(3000, 18000, 6000);
  encoder.end();

  odb::dbShape shape;
  odb::dbWireShapeItr sitr;
  std::vector<int> shape_id;

  for (sitr.begin(wire); sitr.next(shape);) {
    print_shape(shape, 0, 0);
    shape_id.push_back(sitr.getShapeId());
  }

  logger_->info(RCX, 253, "------------------------------");

  std::vector<int>::iterator itr;

  for (itr = shape_id.begin(); itr != shape_id.end(); ++itr) {
    int id = *itr;
    odb::dbShape shape;
    wire->getShape(id, shape);
    print_shape(shape, 0, 0);
  }

  logger_->info(RCX, 254, "\nRC PATHS ------------------------------");

  odb::dbWirePath path;
  odb::dbWirePathShape pshape;

  odb::dbWirePathItr pitr;

  for (pitr.begin(wire); pitr.getNextPath(path);) {
    uint prevId = path.junction_id;
    odb::Point prevPoint = path.point;
    while (pitr.getNextShape(pshape)) {
      uint newId = pshape.junction_id;
      odb::Point newPoint = pshape.point;

      uint len = print_shape(pshape.shape, prevId, newId);
      if (len > 0)  // metal)
      {
      } else  // via
      {
      }
      prevId = newId;
      prevPoint = newPoint;
      /*
                                  int            junction_id; // junction id of
         this point odb::Point       point;       // starting point of path
          odb::dbTechLayer *  layer;       // layer of shape, or exit layer of
         via odb::dbBTerm *      bterm;       // dbBTerm connected at this
         point, otherwise NULL odb::dbITerm *      iterm;       // dbITerm
         connected at this point, otherwise NULL odb::dbShape        shape; //
         shape at this point
      */
    }
  }

  return 0;
}

}  // namespace rcx
