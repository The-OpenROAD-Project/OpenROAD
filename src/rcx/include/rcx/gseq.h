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

#include "odb/array1.h"
#include "odb/odb.h"
#include "odb/util.h"

namespace rcx {

struct SEQ
{
  int _ll[2];
  int _ur[2];
  int type;
};

class gs
{
 public:
  gs(odb::AthPool<SEQ>* seqPool);
  ~gs();

  void
  configurePlane(int plane, int xres, int yres, int x0, int y0, int x1, int y1);

  // render a rectangle
  int box(int x0, int y0, int x1, int y1, int plane);

  // set the number of planes
  void setPlanes(int nplanes);

  uint getSeq(int* ll,
              int* ur,
              uint order,
              uint plane,
              odb::Ath__array1D<SEQ*>* array);

  void release(SEQ* s);

  SEQ* salloc();

 private:
  using pixint = std::uint64_t;
  using pixints = std::uint32_t;

  union pixmap
  {
    pixint lword;
    pixints word[2];
  };

  struct plconfig
  {
    int width;           // width in pixels
    int height;          // height in pixels
    int x_resolution;    // x dbu per pixel
    int y_resolution;    // y dbu per pixel
    int x0, x1, y0, y1;  // bounding box in dbu
    int pixwrem;         // how many pixels are used in the last block of a row
    int pixstride;       // how many memory blocks per row
    int pixfullblox;     // how many "full" blocks per row
                         // (equal to stride, or one less if pixwrem > 0)
    pixmap* plalloc;
    pixmap* plane;
    pixmap* plptr;
  };

  // set the size parameters
  void setSize(int plane, int xres, int yres, int x0, int x1, int y0, int y1);

  void allocMem();
  void freeMem();

  bool checkPlane(int plane);

  bool getSeqRow(int y, int plane, int stpix, int& epix, int& seqcol);
  bool getSeqCol(int x, int plane, int stpix, int& epix, int& seqcol);

  static constexpr int PIXMAPGRID = 64;

  int nplanes_;   // max number of planes
  int maxplane_;  // maximum used plane

  int init_;

  std::vector<plconfig> pldata_;  // size == nplanes_ when init_ == ALLOCATED

  pixint start_[PIXMAPGRID];
  pixint middle_[PIXMAPGRID];
  pixint end_[PIXMAPGRID];

  odb::AthPool<SEQ>* seqPool_;
};

}  // namespace rcx
