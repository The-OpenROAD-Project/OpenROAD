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

#include <cstdlib>
#include <cstring>

#include "rcx/gseq.h"

namespace rcx {

static constexpr long long PIXFILL = 0xffffffffffffffffLL;
static constexpr long long PIXMAX = 0x8000000000000000LL;
static constexpr int PIXADJUST = 2;

/* Values for the member variable init_
 * INIT = created,
 * CONFIGURED = has reasonable values for width, height, slices, etc
 * ALLOCATED = memory has been allocated
 */
static constexpr int INIT = 0;
static constexpr int SLICES = 1;
static constexpr int ALLOCATED = 2;
static constexpr int GS_ALL = (SLICES | ALLOCATED);

static constexpr int GS_WHITE = 0;
static constexpr int GS_BLACK = 1;
static constexpr int GS_NONE = 3;

static constexpr int GS_ROW = 1;
static constexpr int GS_COLUMN = 0;

gs::gs(odb::AthPool<SEQ>* pool)
{
  init_ = INIT;

  pixint sum = PIXFILL;
  for (int i = 0; i < PIXMAPGRID; i++) {
    start_[i] = sum;
    sum = (sum >> 1);
  }

  sum = PIXMAX;
  pixint s2 = sum;
  for (int i = 0; i < PIXMAPGRID; i++) {
    end_[i] = sum;
    sum = (sum >> 1) | PIXMAX;

    middle_[i] = s2;
    s2 = (s2 >> 1);
  }

  nslices_ = -1;
  maxslice_ = -1;

  seqPool_ = pool;
}

gs::~gs()
{
  freeMem();
}

void gs::freeMem()
{
  if (init_ & ALLOCATED) {
    pldata_.clear();
    init_ = (init_ & ~ALLOCATED);
  }
}

void gs::allocMem()
{
  if (init_ & ALLOCATED) {
    freeMem();
  }

  if (init_ & SLICES) {
    pldata_.resize(nslices_);
    init_ |= ALLOCATED;
  }
}

void gs::setSlices(const int nslices)
{
  freeMem();
  nslices_ = nslices;
  init_ |= SLICES;
  allocMem();
}

void gs::setSize(const int pl,
                 const int xres,
                 const int yres,
                 const int x0,
                 const int y0,
                 const int x1,
                 const int y1)
{
  plconfig& plc = pldata_[pl];

  plc.x0 = x0;
  plc.x1 = x1;
  plc.y0 = y0;
  plc.y1 = y1;

  if (plc.x1 <= plc.x0) {
    // to avoid things like divide by 0, etc
    plc.x1 = plc.x0 + 1;
  }

  if (plc.y1 <= plc.y0) {
    // to avoid things like divide by 0, etc
    plc.y1 = plc.y0 + 1;
  }

  plc.xres = xres;
  plc.yres = yres;

  plc.width = (plc.x1 - plc.x0 + 1) / plc.xres;
  if (((plc.x1 - plc.x0 + 1) % plc.xres) != 0) {
    plc.width++;
  }

  plc.height = (plc.y1 - plc.y0 + 1) / plc.yres;
  if (((plc.y1 - plc.y0 + 1) % plc.yres) != 0) {
    plc.height++;
  }

  plc.pixwrem = plc.width % PIXMAPGRID;

  // how many pixels pixmap is wide, upped to multiple of PIXMAPGRID
  int pixwidth = plc.width;
  if (plc.pixwrem != 0) {
    pixwidth += (PIXMAPGRID - plc.pixwrem);
  }

  plc.pixstride = pixwidth / PIXMAPGRID;

  plc.pixfullblox = plc.pixstride;
  if (plc.pixwrem != 0) {
    plc.pixfullblox--;
  }

  pixmap* pm = (pixmap*) calloc(plc.height * plc.pixstride + PIXADJUST,
                                sizeof(pixmap));
  if (pm == nullptr) {
    fprintf(stderr,
            "Error: not enough memory available trying to allocate plane %d\n",
            pl);
    exit(-1);
  }

  plc.plalloc = pm;
  plc.plane = pm;
}

void gs::configureSlice(const int _slicenum,
                        const int _xres,
                        const int _yres,
                        const int _x0,
                        const int _y0,
                        const int _x1,
                        const int _y1)
{
  if ((init_ & ALLOCATED) && _slicenum < nslices_) {
    setSize(_slicenum, _xres, _yres, _x0, _y0, _x1, _y1);
  }
}

static int clip(const int p, const int min, const int max)
{
  return (p < min) ? min : (p >= max) ? (max - 1) : p;
}

int gs::box(int px0, int py0, int px1, int py1, const int sl)
{
  if (!checkSlice(sl)) {
    fprintf(stderr,
            "Box in slice %d exceeds maximum configured slice count %d - "
            "ignored!\n",
            sl,
            nslices_);
    return -1;
  }

  if (!(init_ & GS_ALL)) {
    return -1;
  }

  if (sl > maxslice_) {
    maxslice_ = sl;
  }

  const plconfig& plc = pldata_[sl];

  // normalize bbox
  if (px0 > px1) {
    std::swap(px0, px1);
  }
  if (py0 > py1) {
    std::swap(py0, py1);
  }

  if (px1 < plc.x0) {
    return -1;
  }
  if (px0 > plc.x1) {
    return -1;
  }
  if (py1 < plc.y0) {
    return -1;
  }
  if (py0 > plc.y1) {
    return -1;
  }

  // convert to pixel space
  int cx0 = int((px0 - plc.x0) / plc.xres);
  int cx1 = int((px1 - plc.x0) / plc.xres);
  int cy0 = int((py0 - plc.y0) / plc.yres);
  int cy1 = int((py1 - plc.y0) / plc.yres);

  // render a rectangle on the selected slice. Paint all pixels
  cx0 = clip(cx0, 0, plc.width);
  cx1 = clip(cx1, 0, plc.width);
  cy0 = clip(cy0, 0, plc.height);
  cy1 = clip(cy1, 0, plc.height);
  // now fill in planes object

  // xbs = x block start - block the box starts in
  const int xbs = cx0 / PIXMAPGRID;
  // xbe = x block end - block the box ends in
  const int xbe = cx1 / PIXMAPGRID;

  pixint smask = start_[cx0 % PIXMAPGRID];
  const pixint emask = end_[cx1 % PIXMAPGRID];

  if (xbe == xbs) {
    smask &= emask;
  }

  pixmap* pm = plc.plane + plc.pixstride * cy0 + xbs;

  for (int yb = cy0; yb <= cy1; yb++) {
    // start block
    pixmap* pcb = pm;

    // for next time through loop - allow compiler time for out-of-order
    pm += plc.pixstride;

    // do "start" block
    pcb->lword = pcb->lword | smask;

    // do "middle" block
    for (int mb = xbs + 1; mb < xbe;) {
      pcb++;
      // moved here to allow for out-of-order execution
      mb++;

      pcb->lword = PIXFILL;
    }

    // do "end" block
    if (xbe > xbs) {
      pcb++;
      pcb->lword = pcb->lword | emask;
    }
  }

  return 0;
}

bool gs::checkSlice(const int sl)
{
  return 0 <= sl && sl < nslices_;
}

SEQ* gs::salloc()
{
  SEQ* s = seqPool_->alloc();
  return s;
}

void gs::release(SEQ* s)
{
  seqPool_->free(s);
}

/* getSeq - returns an integer corresponding to the longest uninterrupted
 * sequence of virtual bits found of the same type (set or unset)
 *
 * Parameters: ll - lower left array [0] = x0, [1] = y0
 *             ur - upper right array [0] = x1, [1] = y1
 *             order - search by column or by row (GS_COLUMN, GS_ROW)
 *             plane  - which plane to search
 *             array - pool of sequence pointers to get a handle from
 */

uint gs::getSeq(int* ll,
                int* ur,
                const uint order,
                const uint plane,
                odb::Ath__array1D<SEQ*>* array)
{
  if (!checkSlice(plane)) {
    return 0;
  }

  const plconfig& plc = pldata_[plane];

  // Sanity checks
  if (ur[0] < plc.x0) {
    return 0;
  }
  if (ll[0] > plc.x1) {
    return 0;
  }

  if (ur[1] < plc.y0) {
    return 0;
  }
  if (ll[1] > plc.y1) {
    return 0;
  }

  if (ll[0] < plc.x0) {
    ll[0] = plc.x0;
  }

  if (ur[0] > plc.x1) {
    ur[0] = plc.x1;
  }

  if (ll[1] < plc.y0) {
    ll[1] = plc.y0;
  }

  if (ur[1] > plc.y1) {
    ur[1] = plc.y1;
  }
  // End Sanity Checks

  SEQ* s = salloc();

  // convert into internal coordinates
  const int cx0 = int((ll[0] - plc.x0) / plc.xres);
  const int cy0 = int((ll[1] - plc.y0) / plc.yres);

  int cx1 = int((ur[0] - plc.x0) / plc.xres);
  if (((ur[0] - plc.x0 + 1) % plc.xres) != 0) {
    cx1++;
  }
  int cy1 = int((ur[1] - plc.y0) / plc.yres);
  if (((ur[1] - plc.y0 + 1) % plc.yres) != 0) {
    cy1++;
  }

  int blacksum = 0;

  if (order == GS_ROW) {
    int rs = ll[1];
    int re = ur[1];

    for (int row = cy0; row <= cy1; row++) {
      int start = cx0;
      int end = cx1;
      bool flag = false;
      while (getSeqRow(row, plane, start, end, s->type) == 0) {
        s->_ll[0] = (int) (start * (plc.xres) + plc.x0);
        s->_ur[0] = (int) ((end + 1) * (plc.xres) + (plc.x0) - 1);
        if (s->_ur[0] >= ur[0]) {
          s->_ur[0] = ur[0];
          flag = true;
        }

        if ((row == cy0) && (s->_ll[0] < ll[0])) {
          s->_ll[0] = ll[0];
        }

        s->_ll[1] = rs;
        s->_ur[1] = re;

        if (s->type == GS_BLACK) {
          blacksum += (s->_ur[0] - s->_ll[0]);
        }

        if (array != nullptr) {
          array->add(s);
          s = salloc();
        }
        if (flag) {
          seqPool_->free(s);
          return blacksum;
        }
        start = end + 1;
      }
      rs += plc.yres;
      if (rs > ur[1]) {
        break;
      }
      re += plc.yres;
    }
  } else if (order == GS_COLUMN) {
    int cs = ((cx1 + cx0) / 2) * plc.xres + plc.x0;
    int ce = cs;

    if (cs < ll[0]) {
      cs = ll[0];
    }

    if (ce > ur[0]) {
      ce = ur[0];
    }

    for (int col = cx0; col <= cx0; col++) {
      int start = cy0;
      int end;
      bool flag = false;
      while (getSeqCol(col, plane, start, end, s->type) == 0) {
        s->_ll[1] = (int) (start * plc.yres + plc.y0);
        s->_ur[1] = (int) ((end + 1) * plc.yres + plc.y0 - 1);
        if (s->_ur[1] >= ur[1]) {
          flag = true;
          s->_ur[1] = ur[1];
        }

        if ((col == cx0) && (s->_ll[1] < ll[1])) {
          s->_ll[1] = ll[1];
        }

        s->_ll[0] = cs;
        s->_ur[0] = ce;

        if (s->type == GS_BLACK) {
          blacksum += (s->_ur[1] - s->_ll[1]);
        }

        if (array != nullptr) {
          array->add(s);
          s = salloc();
        }
        if (flag) {
          seqPool_->free(s);
          return blacksum;
        }
        start = end + 1;
      }
      cs += plc.xres;
      if (cs > ur[0]) {
        break;
      }
      ce += plc.xres;
    }
  }
  seqPool_->free(s);
  return blacksum;
}

int gs::getSeqRow(const int y,
                  const int plane,
                  const int stpix,
                  int& epix,
                  int& seqcol)
{
  if (!(init_ & ALLOCATED)) {
    return -1;
  }

  const plconfig& plc = pldata_[plane];
  if (y >= plc.height) {
    return -1;
  }

  if (stpix >= plc.width) {
    return -1;
  }

  int sto = stpix / PIXMAPGRID;
  const int str = stpix - (sto * PIXMAPGRID);

  const long offset = y * plc.pixstride + sto;
  pixmap* pl = pldata_[plane].plane + offset;

  seqcol = GS_NONE;

  // take care of start
  if ((pl->lword & start_[str]) == start_[str]) {
    seqcol = GS_BLACK;
    sto++;
    pl++;
  } else if ((pl->lword & start_[str]) == 0) {
    seqcol = GS_WHITE;
    sto++;
    pl++;
  } else {
    // ends here already
    const pixint bc = (pl->lword) & middle_[str];
    if (bc != 0) {
      seqcol = GS_BLACK;
    } else {
      seqcol = GS_WHITE;
    }
    epix = (sto * PIXMAPGRID) + str;
    for (int bit = str + 1; bit < PIXMAPGRID; bit++) {
      if ((pl->lword & middle_[bit]) == 0) {
        if (seqcol == GS_BLACK) {
          break;
        }
      } else {
        if (seqcol == GS_WHITE) {
          break;
        }
      }

      epix++;
    }
    return 0;
  }

  if (sto > plc.pixfullblox) {
    epix = plc.width - 1;
    return 0;
  }

  int st;
  for (st = sto; st < plc.pixfullblox; st++) {
    if ((pl->lword) == PIXFILL) {
      if (seqcol != GS_BLACK) {
        epix = st * PIXMAPGRID - 1;
        return 0;
      }
    } else if (pl->lword == 0) {
      if (seqcol != GS_WHITE) {
        epix = st * PIXMAPGRID - 1;
        return 0;
      }
    } else {
      // ends here
      epix = st * PIXMAPGRID - 1;
      for (int bit = 0; bit < PIXMAPGRID; bit++) {
        if (((pl->lword) & middle_[bit]) == 0) {
          if (seqcol == GS_BLACK) {
            break;
          }
        } else {
          if (seqcol == GS_WHITE) {
            break;
          }
        }

        epix++;
      }
      return 0;
    }
    pl++;
  }
  // handle non-even case
  epix = st * PIXMAPGRID - 1;
  for (int bit = 0; bit < plc.pixwrem; bit++) {
    if (((pl->lword) & middle_[bit]) == 0) {
      if (seqcol == GS_BLACK) {
        break;
      }
    } else {
      if (seqcol == GS_WHITE) {
        break;
      }
    }
    epix++;
  }
  return 0;
}

int gs::getSeqCol(const int x,
                  const int plane,
                  const int stpix,
                  int& epix,
                  int& seqcol)
{
  if (!(init_ & ALLOCATED)) {
    return -1;
  }

  const plconfig& plc = pldata_[plane];
  if (x >= plc.width) {
    return -1;
  }

  if (stpix >= plc.height) {
    return -1;
  }

  // get proper "word" offset
  const int sto = x / PIXMAPGRID;
  const int stc = x - (sto * PIXMAPGRID);

  const long offset = sto + stpix * plc.pixstride;
  const pixint bitmask = middle_[stc];
  pixmap* pl = pldata_[plane].plane + offset;

  seqcol = GS_NONE;

  if (pl->lword & bitmask) {
    seqcol = GS_BLACK;
  } else {
    seqcol = GS_WHITE;
  }

  for (int row = stpix + 1; row < plc.height; row++) {
    pl += plc.pixstride;
    if (pl->lword & bitmask) {
      if (seqcol == GS_BLACK) {
        continue;
      }
      epix = row - 1;
      return 0;
    }
    if (seqcol == GS_BLACK) {
      epix = row - 1;
      return 0;
    }
    continue;
  }

  epix = plc.height;
  return 0;
}

}  // namespace rcx
