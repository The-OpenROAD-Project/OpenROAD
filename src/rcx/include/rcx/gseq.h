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

  void configureSlice(int slicenum,
                      int xres,
                      int yres,
                      int x0,
                      int y0,
                      int x1,
                      int y1);

  // render a rectangle
  int box(int x0, int y0, int x1, int y1, int slice);

  // set the number of slices
  int set_slices(int nslices);

  uint get_seq(int* ll,
               int* ur,
               uint order,
               uint plane,
               odb::Ath__array1D<SEQ*>* array);

  void release(SEQ* s);

  int intersect_rows(int row1, int row2, int store);
  int union_rows(int row1, int row2, int store);
  int xor_rows(int row1, int row2, int store);

  SEQ* salloc();

 private:
  using gsPixel = char;

  using pixint = std::uint64_t;
  using pixints = unsigned int;

  union pixmap
  {
    pixint lword;
    pixints word[2];
  };

  struct plconfig
  {
    int width;
    int height;
    int xres;
    int yres;
    int x0, x1, y0, y1;  // bounding box
    int pixwrem;         // how many pixels are used in the last block of a row
    int pixstride;       // how many memory blocks per row
    int pixfullblox;     // how many "full" blocks per row
                         // (equal to stride, or one less if pixwrem > 0)
    pixmap* plalloc;
    pixmap* plane;
    pixmap* plptr;
  };

  // set the size parameters
  void setSize(int pl, int xres, int yres, int x0, int x1, int y0, int y1);

  void alloc_mem();
  void free_mem();

  int check_slice(int sl);

  int get_seqrow(int y, int plane, int stpix, int& epix, int& seqcol);
  int get_seqcol(int x, int plane, int stpix, int& epix, int& seqcol);

  static constexpr int PIXMAPGRID = 64;

  int nslices_;   // max number of slices
  int maxslice_;  // maximum used slice

  int init_;

  plconfig* plc_;
  plconfig** pldata_;  // size == nslices_ when init_ == ALLOCATED

  pixint start_[PIXMAPGRID];
  pixint middle_[PIXMAPGRID];
  pixint end_[PIXMAPGRID];

  odb::AthPool<SEQ>* seqPool_;
};

}  // namespace rcx
