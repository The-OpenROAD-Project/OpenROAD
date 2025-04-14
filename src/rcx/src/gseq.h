// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <vector>

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

// Holds a set of bitmaps per plane covering the given area.
class gs
{
 public:
  gs(odb::AthPool<SEQ>* seqPool);
  ~gs();

  // set the number of planes
  void setPlanes(int nplanes);

  // Set the bounds and resolution for one plane.
  void configurePlane(int plane,
                      int x_resolution,
                      int y_resolution,
                      int x0,
                      int y0,
                      int x1,
                      int y1);

  // add a rectangle to a plane
  int box(int x0, int y0, int x1, int y1, int plane);

  // Returns an integer corresponding to the longest uninterrupted
  // sequence of virtual bits found of the same type (set or unset)
  //
  // Parameters: ll - lower left array [0] = x0, [1] = y0
  //             ur - upper right array [0] = x1, [1] = y1
  //             order - search by column or by row (GS_COLUMN, GS_ROW)
  //             plane  - which plane to search
  //             array - pool of sequence pointers to get a handle from
  uint getSeq(int* ll,
              int* ur,
              uint order,
              uint plane,
              odb::Ath__array1D<SEQ*>* array);

  // Allocate a SEQ
  SEQ* salloc();

  // Deallocate a SEQ
  void release(SEQ* s);

 private:
  using pixint = std::uint64_t;

  struct pixmap
  {
    pixint lword;
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
    pixmap* plane;
  };

  // set the size parameters
  void setSize(int plane, int xres, int yres, int x0, int x1, int y0, int y1);

  void allocMem();
  void freeMem();

  bool checkPlane(int plane);

  bool getSeqRow(int y, int plane, int stpix, int& epix, int& seqcol);
  bool getSeqCol(int x, int plane, int stpix, int& epix, int& seqcol);

  int nplanes_;   // max number of planes
  int maxplane_;  // maximum used plane

  int init_;

  std::vector<plconfig> pldata_;  // size == nplanes_ when init_ == ALLOCATED

  static constexpr int PIXMAPGRID = 64;
  pixint start_[PIXMAPGRID];
  pixint middle_[PIXMAPGRID];
  pixint end_[PIXMAPGRID];

  odb::AthPool<SEQ>* seqPool_;
};

}  // namespace rcx
