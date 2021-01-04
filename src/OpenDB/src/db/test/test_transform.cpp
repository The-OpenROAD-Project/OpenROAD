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
#include "../../include/dbTransform.h"
#include "../../include/geom.h"

using namespace odb

    int
    main(int argc, char** argv)
{
  Point p(30, 30);
  Point origin(0, 0);

  Rect      r(10, 10, 15, 20);
  dbOrientType orient_R0(dbOrientType::R0);
  dbTransform  r0(orient_R0, origin, 20, 40);
  r0.transformInstanceBBox(p, r);
  printf("R0 (%d %d) (%d %d)\n", r.xMin(), r.yMin(), r.xMax(), r.yMax());

  r = Rect(10, 10, 15, 20);
  dbOrientType orient_R90(dbOrientType::R90);
  dbTransform  r90(orient_R90, origin, 20, 40);
  r90.transformInstanceBBox(p, r);
  printf("R90 (%d %d) (%d %d)\n", r.xMin(), r.yMin(), r.xMax(), r.yMax());

  r = Rect(10, 10, 15, 20);
  dbOrientType orient_R180(dbOrientType::R180);
  dbTransform  r180(orient_R180, origin, 20, 40);
  r180.transformInstanceBBox(p, r);
  printf("R180 (%d %d) (%d %d)\n", r.xMin(), r.yMin(), r.xMax(), r.yMax());

  r = Rect(10, 10, 15, 20);
  dbOrientType orient_R270(dbOrientType::R270);
  dbTransform  r270(orient_R270, origin, 20, 40);
  r270.transformInstanceBBox(p, r);
  printf("R270 (%d %d) (%d %d)\n", r.xMin(), r.yMin(), r.xMax(), r.yMax());

  r = Rect(10, 10, 15, 20);
  dbOrientType orient_MY(dbOrientType::MY);
  dbTransform  my(orient_MY, origin, 20, 40);
  my.transformInstanceBBox(p, r);
  printf("MY (%d %d) (%d %d)\n", r.xMin(), r.yMin(), r.xMax(), r.yMax());

  r = Rect(10, 10, 15, 20);
  dbOrientType orient_MYR90(dbOrientType::MYR90);
  dbTransform  myr90(orient_MYR90, origin, 20, 40);
  myr90.transformInstanceBBox(p, r);
  printf("MYR90 (%d %d) (%d %d)\n", r.xMin(), r.yMin(), r.xMax(), r.yMax());

  r = Rect(10, 10, 15, 20);
  dbOrientType orient_MX(dbOrientType::MX);
  dbTransform  mx(orient_MX, origin, 20, 40);
  mx.transformInstanceBBox(p, r);
  printf("MX (%d %d) (%d %d)\n", r.xMin(), r.yMin(), r.xMax(), r.yMax());

  r = Rect(10, 10, 15, 20);
  dbOrientType orient_MXR90(dbOrientType::MXR90);
  dbTransform  mxr90(orient_MXR90, origin, 20, 40);
  mxr90.transformInstanceBBox(p, r);
  printf("MXR90 (%d %d) (%d %d)\n", r.xMin(), r.yMin(), r.xMax(), r.yMax());
}
