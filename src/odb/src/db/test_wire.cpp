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

#include <stdio.h>

#include "db.h"
#include "dbLogger.h"
#include "dbRtTree.h"
#include "dbShape.h"
#include "dbWireCodec.h"

using namespace odb;

// static void print_wire( dbWire * wire );
static void print_encoding(dbWire* wire);

static dbTechLayer* m1;
static dbTechLayer* m2;
static dbTechLayer* m3;
static dbTechVia* v12;
static dbTechVia* v23;

static void create_tech(dbDatabase* db)
{
  dbTech* tech = dbTech::create(db);

  m1 = dbTechLayer::create(tech, "M1", (dbTechLayerType::ROUTING));
  m1->setWidth(2000);
  m2 = dbTechLayer::create(tech, "M2", (dbTechLayerType::ROUTING));
  m2->setWidth(2000);
  m3 = dbTechLayer::create(tech, "M3", (dbTechLayerType::ROUTING));
  m3->setWidth(2000);

  v12 = dbTechVia::create(tech, "VIA12");
  dbBox::create(v12, m1, -1000, -1000, 1000, 1000);
  dbBox::create(v12, m2, -1000, -1000, 1000, 1000);

  v23 = dbTechVia::create(tech, "VIA23");
  dbBox::create(v23, m2, -1000, -1000, 1000, 1000);
  dbBox::create(v23, m3, -1000, -1000, 1000, 1000);
}
/*
static void print_shape( dbShape & shape )
{
    if ( shape.isVia() )
    {
        dbTechVia * tech_via = shape.getTechVia();
        std::string vname = tech_via ->getName();
        uint dx = shape.xMax() - shape.xMin();
        uint dy = shape.yMax() - shape.yMin();

        notice(0,"VIA %s ( %d %d )\n",
               vname.c_str(),
               shape.xMin() + dx / 2,
               shape.yMin() + dy / 2
               );
    }
    else
    {
        dbTechLayer * layer = shape.getTechLayer();
        std::string lname = layer->getName();
        notice(0,"RECT %s ( %d %d ) ( %d %d )\n",
               lname.c_str(),
               shape.xMin(),
               shape.yMin(),
               shape.xMax(),
               shape.yMax() );
    }
}
*/
int db_test_wires(dbDatabase* db)
{
  create_tech(db);
  dbChip* chip = dbChip::create(db);
  dbBlock* block = dbBlock::create(chip, "chip");
  dbNet* net = dbNet::create(block, "net");
  dbWire* wire = dbWire::create(net);
  dbWireEncoder encoder;
  encoder.begin(wire);
  encoder.newPath(m1, dbWireType::ROUTED);
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
  int j4 = encoder.addPoint(3000, 10000);
  encoder.addTechVia(v12);
  encoder.addTechVia(v23);
  encoder.addPoint(3000, 10000, 4000);
  encoder.addPoint(3000, 18000, 6000);
  encoder.newPathShort(j1, m2, dbWireType::ROUTED);
  encoder.addPoint(10000, -1000);
  encoder.addPoint(10000, -4000);
  encoder.end();

  /*
      encoder.append(wire);
      encoder.newPath( m1, dbWireType::ROUTED );
      encoder.addPoint( 2000, 2000 );
      j1 = encoder.addPoint( 10000, 2000 );
      encoder.addPoint( 18000, 2000 );
      encoder.newPath(j1);
      encoder.addTechVia(v12);
      j2 = encoder.addPoint( 10000, 10000 );
      encoder.addPoint( 10000, 18000 );
      encoder.newPath(j2);
      j3 = encoder.addTechVia(v12);
      encoder.addPoint( 23000, 10000, 4000 );
      encoder.newPath(j3);
      encoder.addPoint( 3000, 10000 );
      encoder.addTechVia(v12);
      encoder.addTechVia(v23);
      encoder.addPoint( 3000, 10000, 4000 );
      encoder.addPoint( 3000, 18000, 6000 );
      encoder.newPathShort( j1, m2, dbWireType::ROUTED );
      encoder.addPoint( -1000, 10000 );
      encoder.addPoint( -4000, 10000 );
      encoder.end();
  */

  // print_wire( wire );
  notice(0, "----wire encoding ---------------\n\n");
  print_encoding(wire);

  dbRtTree G;
  G.decode(wire);
  G.encode(wire);
  notice(0, "----wire dbRt encode decode ---------------\n\n");
  print_encoding(wire);

  G.decode(wire);
  dbRtEdge* edge = G.getEdge(j4);
  dbRtNode* src = edge->getSource();
  dbRtNode* tgt = edge->getTarget();
  dbRtNode* mid = G.createNode(5000, 10000, src->getLayer());
  G.deleteEdge(edge);
  G.createSegment(src, mid);
  G.createSegment(mid, tgt);

  G.encode(wire);
  notice(0, "----wire dbRt encoding ---------------\n\n");
  print_encoding(wire);

  dbRtTree* Gcopy = G.duplicate();
  Gcopy->encode(wire);
  notice(0, "----copy wire dbRt encoding ---------------\n\n");
  print_encoding(wire);
  delete Gcopy;

  dbRtTree T;
  T.move(&G);
  T.encode(wire);
  notice(0, "----T.move wire dbRt encoding ---------------\n\n");
  print_encoding(wire);

  dbRtTree T2;
  T2.copy(&T);
  T2.encode(wire);
  notice(0, "----T.copy wire dbRt encoding ---------------\n\n");
  print_encoding(wire);

  dbBlock* child = dbBlock::create(block, "chip");
  dbBlock::copyViaTable(child, block);
  dbNet* testNet = dbNet::create(child, "test");
  dbWire* testWire = dbWire::create(testNet);
  dbWire::copy(testWire, wire, true, false);
  notice(0, "---- testWire encoding ---------------\n\n");
  print_encoding(testWire);

  return 0;
}
/*
void print_wire( dbWire * wire )
{
    notice(0,"------------------------------\n\n");

    dbShape shape;
    dbWireShapeItr sitr;
    std::vector<int> shape_id;

    for( sitr.begin(wire); sitr.next(shape); )
    {
        print_shape(shape);
        shape_id.push_back(sitr.getShapeId());
    }

    notice(0,"------------------------------\n");

    std::vector<int>::iterator itr;

    for( itr = shape_id.begin(); itr != shape_id.end(); ++itr )
    {
        int id = *itr;
        dbShape shape;
        wire->getShape( id, shape );
        print_shape(shape);
    }

    notice(0,"------------------------------\n");

    dbWirePath      path;
    dbWirePathShape pshape;

    dbWirePathItr pitr;

    for(pitr.begin(wire); pitr.getNextPath(path); )
    {
        while( pitr.getNextShape( pshape ) )
        {
            print_shape(pshape.shape);
        }
    }
}
*/
void print_encoding(dbWire* wire)
{
  dbWireDecoder decoder;

  int jct_id = -1;

  for (decoder.begin(wire);;) {
    switch (decoder.next()) {
      case dbWireDecoder::PATH: {
        notice(0, "NEW PATH\n");
        break;
      }

      case dbWireDecoder::JUNCTION: {
        jct_id = decoder.getJunctionValue();
        notice(0, "NEW JUNCTION J%d\n", jct_id);
        break;
      }

      case dbWireDecoder::SHORT: {
        int id = decoder.getJunctionValue();
        notice(0, "NEW SHORT J%d\n", id);
        break;
      }

      case dbWireDecoder::VWIRE: {
        int id = decoder.getJunctionValue();
        notice(0, "NEW VWIRE J%d\n", id);
        break;
      }

      case dbWireDecoder::POINT: {
        if (jct_id != -1) {
          jct_id = -1;
          break;
        }

        int x, y;
        decoder.getPoint(x, y);
        int j = decoder.getJunctionId();
        notice(0, "J%d (%d %d)\n", j, x, y);
        break;
      }

      case dbWireDecoder::POINT_EXT: {
        if (jct_id != -1) {
          jct_id = -1;
          break;
        }

        int x, y, e;
        decoder.getPoint(x, y, e);
        int j = decoder.getJunctionId();
        notice(0, "J%d (%d %d %d)\n", j, x, y, e);
        break;
      }

      case dbWireDecoder::VIA:
        break;

      case dbWireDecoder::TECH_VIA: {
        dbTechVia* tech_via = decoder.getTechVia();
        std::string vname = tech_via->getName();
        int j = decoder.getJunctionId();
        notice(0, "J%d VIA %s\n", j, vname.c_str());
        break;
      }

      case dbWireDecoder::RECT:
      case dbWireDecoder::ITERM:
      case dbWireDecoder::BTERM:
      case dbWireDecoder::RULE:
        break;

      case dbWireDecoder::END_DECODE:
        return;
    }
  }
}
