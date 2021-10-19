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
#ifndef WIN32
#include <sys/time.h>
#endif
#include "db.h"
#include "dbLogger.h"
#include "dbRtTree.h"
#include "dbShape.h"
#include "dbWireCodec.h"

using namespace odb;

#ifdef WIN32
class Timer
{
 public:
  Timer() {}

  void start() {}

  void stop() {}

  double result_usec() { return 0.0; }

  double result_sec() { return 0.0; }
};
#else
class Timer
{
  struct timeval _start;
  struct timeval _end;

 public:
  Timer() {}

  void start()
  {
    struct timezone z;
    gettimeofday(&_start, &z);
  }

  void stop()
  {
    struct timezone z;
    gettimeofday(&_end, &z);
  }

  double result_usec()
  {
    double ds = (double) (_end.tv_sec - _start.tv_sec);
    double du = (double) (_end.tv_usec - _start.tv_usec);
    return (ds / 1000000.0) + du;
  }

  double result_sec()
  {
    double ds = (double) (_end.tv_sec - _start.tv_sec);
    double du = (double) (_end.tv_usec - _start.tv_usec);
    return ds + du / 1000000.0;
  }
};
#endif

struct WireShape
{
  dbWire* wire;
  dbTechLayer* layer;
  int id;

  WireShape() {}

  WireShape(dbWire* w, dbTechLayer* l, int i) : wire(w), layer(l), id(i) {}
};

void db_test_wires2(dbBlock* block)
{
  dbShape shape;
  dbSet<dbNet> nets = block->getNets();
  dbSet<dbNet>::iterator itr;
  std::vector<WireShape> shapes;
  double total = 0.0;
  int count = 0;

  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    dbNet* net = *itr;

    dbWire* wire = net->getWire();

    if (wire == NULL)
      continue;

    Timer t;
    dbWireShapeItr sitr;
    t.start();

    for (sitr.begin(wire); sitr.next(shape); ++count) {
      if (shape.getType() == dbShape::SEGMENT)
        shapes.push_back(
            WireShape(wire, shape.getTechLayer(), sitr.getShapeId()));
    }
    t.stop();
    total += t.result_sec();
  }

  notice(0, "shape-itr time (%d): %fs \n", count, total);

  std::vector<WireShape>::iterator witr;

  for (witr = shapes.begin(); witr != shapes.end(); ++witr) {
    WireShape s = *witr;
    dbShape s1, s2;
    s.wire->getShape(s.id, s1);
    s.wire->getSegment(s.id, s.layer, s2);

    if (s1 != s2) {
      notice(0, "compare failed\n");
    }
  }

  Timer t;
  t.start();
  for (witr = shapes.begin(); witr != shapes.end(); ++witr) {
    WireShape s = *witr;
    //        s.wire->getShape(s.id,s1);
    s.wire->getSegment(s.id, s.layer, shape);
  }
  t.stop();
  notice(
      0, "get-segment-itr time (%lu): %fs \n", shapes.size(), t.result_sec());

  t.start();
  for (witr = shapes.begin(); witr != shapes.end(); ++witr) {
    WireShape s = *witr;
    s.wire->getShape(s.id, shape);
  }
  t.stop();
  notice(0, "get-shape-itr time (%lu): %fs \n", shapes.size(), t.result_sec());

  notice(0, "done\n");

  /*
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
  */
}
