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

#include <stdlib.h>
#include <string.h>

#include "gseq.h"

namespace odb {

// ----------------------------------------------------------------------------
//
// gs - Methods
//
// ----------------------------------------------------------------------------
gs::gs(AthPool<SEQ>* pool)
{
  allocSEQ_ = false;
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

  if (pool != nullptr) {
    seqPool_ = pool;
  } else {
    seqPool_ = new AthPool<SEQ>(false, 1024);
    allocSEQ_ = true;
  }
}

gs::~gs()
{
  free_mem();
  if (allocSEQ_)
    delete seqPool_;
}

int gs::free_mem()
{
  if (init_ & ALLOCATED) {
    for (int s = 0; s < nslices_; s++) {
      if (pldata_[s]->plalloc) {
        free(pldata_[s]->plalloc);
      }
      free(pldata_[s]);
    }

    init_ = (init_ & ~ALLOCATED);
    return 0;
  } else {
    return -1;
  }
}

int gs::alloc_mem()
{
  if ((init_ & ALLOCATED)) {
    free_mem();
  }

  if ((init_ & SLICES)) {
    pldata_ = (plconfig**) calloc(nslices_, sizeof(plconfig*));

    for (int s = 0; s < nslices_; s++) {
      pldata_[s] = (plconfig*) calloc(1, sizeof(plconfig));
      pldata_[s]->plalloc = nullptr;
    }

    init_ |= ALLOCATED;

    return 0;
  } else {
    return -1;
  }
}

int gs::set_slices(int nslices, bool /* unused: skipMemAlloc */)
{
  free_mem();
  nslices_ = nslices;
  maxplane_ = nslices_ - 1;
  init_ |= SLICES;
  alloc_mem();

  return 0;
}

int gs::setSize(int pl,
                int xres,
                int yres,
                int x0,
                int y0,
                int x1,
                int y1,
                bool skipPixmap)
{
  plc_ = pldata_[pl];

  plc_->x0 = x0;
  plc_->x1 = x1;
  plc_->y0 = y0;
  plc_->y1 = y1;

  if (plc_->x1 <= plc_->x0) {
    // to avoid things like divide by 0, etc
    plc_->x1 = plc_->x0 + 1;
  }

  if (plc_->y1 <= plc_->y0) {
    // to avoid things like divide by 0, etc
    plc_->y1 = plc_->y0 + 1;
  }

  plc_->xres = xres;
  plc_->yres = yres;

  plc_->width = (plc_->x1 - plc_->x0 + 1) / plc_->xres;
  if (((plc_->x1 - plc_->x0 + 1) % plc_->xres) != 0) {
    plc_->width++;
  }

  plc_->height = (plc_->y1 - plc_->y0 + 1) / plc_->yres;
  if (((plc_->y1 - plc_->y0 + 1) % plc_->yres) != 0) {
    plc_->height++;
  }

  plc_->pixwrem = plc_->width % PIXMAPGRID;

  plc_->pixwidth = plc_->width;
  if (plc_->pixwrem != 0) {
    // round off to next multiple of PIXMAPGRID
    plc_->pixwidth += (PIXMAPGRID - (plc_->width % PIXMAPGRID));
  }

  plc_->pixstride = plc_->pixwidth / PIXMAPGRID;

  plc_->pixfullblox = plc_->pixstride;
  if (plc_->pixwrem != 0) {
    plc_->pixfullblox--;
  }
  plc_->pixwrem--;

  if (skipPixmap) {
    plc_->plalloc = nullptr;
    plc_->plane = nullptr;
    return 0;
  }

  pixmap* pm = (pixmap*) calloc(plc_->height * plc_->pixstride + PIXADJUST,
                                sizeof(pixmap));
  if (pm == nullptr) {
    fprintf(stderr,
            "Error: not enough memory available trying to allocate plane %d\n",
            pl);
    exit(-1);
  }

  plc_->plalloc = pm;
  plc_->plane = pm;

  return 0;
}

int gs::configureSlice(int _slicenum,
                       int _xres,
                       int _yres,
                       int _x0,
                       int _y0,
                       int _x1,
                       int _y1,
                       bool skipAlloc)
{
  if (init_ & ALLOCATED) {
    if (_slicenum < nslices_) {
      setSize(_slicenum, _xres, _yres, _x0, _y0, _x1, _y1, skipAlloc);
    }
  }

  return 0;
}

static int clip(const int p, const int min, const int max)
{
  return (p < min) ? min : (p >= max) ? (max - 1) : p;
}

int gs::box(int px0, int py0, int px1, int py1, int sl, bool checkOnly)
{
  if ((sl < 0) || (sl > nslices_)) {
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

  plc_ = pldata_[sl];

  // normalize bbox
  long a;
  if (px0 > px1) {
    a = px0;
    px0 = px1;
    px1 = a;
  }
  if (py0 > py1) {
    a = py0;
    py0 = py1;
    py1 = a;
  }

  if ((px0 < plc_->x0) && (px1 < plc_->x0))
    return -1;
  if ((px0 > plc_->x1) && (px1 > plc_->x1))
    return -1;
  if ((py0 < plc_->y0) && (py1 < plc_->y0))
    return -1;
  if ((py0 > plc_->y1) && (py1 > plc_->y1))
    return -1;

  if (checkOnly)
    return 0;

  // convert to pixel space
  int cx0 = int((px0 - plc_->x0) / plc_->xres);
  int cx1 = int((px1 - plc_->x0) / plc_->xres);
  int cy0 = int((py0 - plc_->y0) / plc_->yres);
  int cy1 = int((py1 - plc_->y0) / plc_->yres);

  // render a rectangle on the selected slice. Paint all pixels
  cx0 = clip(cx0, 0, plc_->width);
  cx1 = clip(cx1, 0, plc_->width);
  cy0 = clip(cy0, 0, plc_->height);
  cy1 = clip(cy1, 0, plc_->height);
  // now fill in planes object

  pixmap* pm;
  pixmap* pcb;

  // xbs = x block start - block the box starts in
  int xbs = cx0 / PIXMAPGRID;
  // xbs = x block end - block the box ends in
  int xbe = cx1 / PIXMAPGRID;

  int xee = cx1 % PIXMAPGRID;

  pixint smask = start_[cx0 % PIXMAPGRID];
  pixint emask = end_[xee];

  int mb;

  if (xbe == xbs) {
    smask = smask & emask;
  }

  pm = plc_->plane + plc_->pixstride * cy0 + xbs;

  for (int yb = cy0; yb <= cy1; yb++) {
    // start block
    pcb = pm;

    // for next time through loop - allow compiler time for out-of-order
    pm += plc_->pixstride;

    // do "start" block
    pcb->lword = pcb->lword | smask;

    // do "middle" block
    for (mb = xbs + 1; mb < xbe;) {
      pcb++;
      // moved here to allow for out-of-order execution
      mb++;

      // if( pcb->lword != PIXFILL )
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

int gs::check_slice(int sl)
{
  if ((sl < 0) || (sl > nslices_)) {
    return -1;
  }
  return 0;
}

int gs::union_rows(int sl1, int sl2, int store)
{
  if ((check_slice(sl1) != 0) || (check_slice(sl2) != 0)
      || (check_slice(store) != 0)) {
    return -1;
  }

  pixmap* sp1 = pldata_[sl1]->plane;
  pixmap* sp2 = pldata_[sl2]->plane;
  pixmap* ssp = pldata_[store]->plane;

  pixmap* ep = sp1 + (plc_->height * plc_->pixstride);
  while (sp1 < ep) {
    (ssp++)->lword = (sp1++)->lword | (sp2++)->lword;
  }
  return 0;
}
int gs::intersect_rows(int sl1, int sl2, int store)
{
  if ((check_slice(sl1) != 0) || (check_slice(sl2) != 0)
      || (check_slice(store) != 0)) {
    return -1;
  }

  pixmap* sp1 = pldata_[sl1]->plane;
  pixmap* sp2 = pldata_[sl2]->plane;
  pixmap* ssp = pldata_[store]->plane;

  pixmap* ep = sp1 + (plc_->height * plc_->pixstride);
  while (sp1 < ep) {
    (ssp++)->lword = (sp1++)->lword & (sp2++)->lword;
  }

  return 0;
}

int gs::xor_rows(int sl1, int sl2, int store)
{
  if ((check_slice(sl1) != 0) || (check_slice(sl2) != 0)
      || (check_slice(store) != 0)) {
    return -1;
  }

  pixmap* sp1 = pldata_[sl1]->plane;
  pixmap* sp2 = pldata_[sl2]->plane;
  pixmap* ssp = pldata_[store]->plane;

  pixmap* ep = sp1 + (plc_->height * plc_->pixstride);
  while (sp1 < ep) {
    (ssp++)->lword = (sp1++)->lword ^ (sp2++)->lword;
  }
  return 0;
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

/* get_seq - returns an integer corresponding to the longest uninterrupted
 * sequence of virtual bits found of the same type (set or unset)
 *
 * Parameters: ll - lower left array [0] = x0, [1] = y0
 *             ur - upper right array [0] = x1, [1] = y1
 *             order - search by column or by row (GS_COLUMN, GS_ROW)
 *             plane  - which plane to search
 *             array - pool of sequence pointers to get a handle from
 */

uint gs::get_seq(int* ll,
                 int* ur,
                 uint order,
                 uint plane,
                 Ath__array1D<SEQ*>* array)
{
  if (check_slice(plane) != 0) {
    return 0;
  }

  plc_ = pldata_[plane];

  // Sanity checks
  if (ur[0] < plc_->x0)
    return 0;
  if (ll[0] > plc_->x1)
    return 0;

  if (ur[1] < plc_->y0)
    return 0;
  if (ll[1] > plc_->y1)
    return 0;

  if (ll[0] < plc_->x0)
    ll[0] = plc_->x0;

  if (ur[0] > plc_->x1)
    ur[0] = plc_->x1;

  if (ll[1] < plc_->y0)
    ll[1] = plc_->y0;

  if (ur[1] > plc_->y1)
    ur[1] = plc_->y1;
  // End Sanity Checks

  SEQ* s = salloc();

  // convert into internal coordinates
  int cx0 = int((ll[0] - plc_->x0) / plc_->xres);
  int cy0 = int((ll[1] - plc_->y0) / plc_->yres);

  int cx1 = int((ur[0] - plc_->x0) / plc_->xres);
  if (((ur[0] - plc_->x0 + 1) % plc_->xres) != 0) {
    cx1++;
  }
  int cy1 = int((ur[1] - plc_->y0) / plc_->yres);
  if (((ur[1] - plc_->y0 + 1) % plc_->yres) != 0) {
    cy1++;
  }

  int row, col;
  int rs, re;
  int cs, ce;
  int blacksum = 0;
  int start, end;
  bool flag;

  if (order == GS_ROW) {
    rs = ll[1];
    re = ur[1];

    for (row = cy0; row <= cy1; row++) {
      start = cx0;
      end = cx1;
      flag = false;
      while (get_seqrow(row, plane, start, end, s->type) == 0) {
        s->_ll[0] = (int) (start * (plc_->xres) + plc_->x0);
        s->_ur[0] = (int) ((end + 1) * (plc_->xres) + (plc_->x0) - 1);
        if (s->_ur[0] >= ur[0]) {
          s->_ur[0] = ur[0];
          flag = true;
        }

        if ((row == cy0) && (s->_ll[0] < ll[0]))
          s->_ll[0] = ll[0];

        s->_ll[1] = rs;
        s->_ur[1] = re;

        if (s->type == GS_BLACK) {
          blacksum += (s->_ur[0] - s->_ll[0]);
        }

        if (array != nullptr) {
          array->add(s);
          s = salloc();
        }
        if (flag == true) {
          seqPool_->free(s);
          return blacksum;
        }
        start = end + 1;
      }
      rs += plc_->yres;
      if (rs > ur[1])
        break;
      re += plc_->yres;
    }
  } else if (order == GS_COLUMN) {
    cs = ((cx1 + cx0) / 2) * plc_->xres + plc_->x0;
    ce = cs;

    if (cs < ll[0])
      cs = ll[0];

    if (ce > ur[0])
      ce = ur[0];

    for (col = cx0; col <= cx0; col++) {
      start = cy0;
      flag = false;
      while (get_seqcol(col, plane, start, end, s->type) == 0) {
        s->_ll[1] = (int) (start * plc_->yres + plc_->y0);
        s->_ur[1] = (int) ((end + 1) * plc_->yres + plc_->y0 - 1);
        if (s->_ur[1] >= ur[1]) {
          flag = true;
          s->_ur[1] = ur[1];
        }

        if ((col == cx0) && (s->_ll[1] < ll[1]))
          s->_ll[1] = ll[1];

        s->_ll[0] = cs;
        s->_ur[0] = ce;

        if (s->type == GS_BLACK) {
          blacksum += (s->_ur[1] - s->_ll[1]);
        }

        if (array != nullptr) {
          array->add(s);
          s = salloc();
        }
        if (flag == true) {
          seqPool_->free(s);
          return blacksum;
        }
        start = end + 1;
      }
      cs += plc_->xres;
      if (cs > ur[0]) {
        break;
      }
      ce += plc_->xres;
    }
  }
  seqPool_->free(s);
  return blacksum;
}

int gs::get_seqrow(int y, int plane, int stpix, int& epix, int& seqcol)
{
  if (!(init_ & ALLOCATED)) {
    return -1;
  }

  if (y >= plc_->height)
    return -1;

  if (stpix >= plc_->width)
    return -1;

  int st;
  int bit;

  long offset;

  int sto = stpix / PIXMAPGRID;
  int str = stpix - (sto * PIXMAPGRID);

  offset = y * plc_->pixstride + sto;
  pixmap* pl = pldata_[plane]->plane + offset;

  seqcol = GS_NONE;
  pixint bc;

  // take care of start
  if ((pl->lword & (start_[str])) == start_[str]) {
    seqcol = GS_BLACK;
    sto++;
    pl++;
  } else if ((pl->lword & (start_[str])) == 0) {
    seqcol = GS_WHITE;
    sto++;
    pl++;
  } else {
    // ends here already
    bc = (pl->lword) & middle_[str];
    if (bc != 0) {
      seqcol = GS_BLACK;
    } else {
      seqcol = GS_WHITE;
    }
    epix = (sto * PIXMAPGRID) + str;
    for (bit = str + 1; bit < PIXMAPGRID; bit++) {
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

  if (sto > plc_->pixfullblox) {
    epix = plc_->width - 1;
    return 0;
  }

  for (st = sto; st < plc_->pixfullblox; st++) {
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
      for (bit = 0; bit < PIXMAPGRID; bit++) {
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
  for (bit = 0; bit <= plc_->pixwrem; bit++) {
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

int gs::get_seqcol(int x, int plane, int stpix, int& epix, int& seqcol)
{
  if (!(init_ & ALLOCATED)) {
    return -1;
  }

  if (x >= plc_->width)
    return -1;

  if (stpix >= plc_->height)
    return -1;

  long offset;

  // get proper "word" offset
  int sto = x / PIXMAPGRID;
  int stc = x - (sto * PIXMAPGRID);

  int row = stpix;
  offset = sto + row * plc_->pixstride;
  pixint bitmask = middle_[stc];
  pixmap* pl = pldata_[plane]->plane + offset;

  seqcol = GS_NONE;

  if ((pl->lword) & bitmask) {
    seqcol = GS_BLACK;
  } else {
    seqcol = GS_WHITE;
  }

  for (row = stpix + 1; row < plc_->height; row++) {
    pl += plc_->pixstride;
    if ((pl->lword) & bitmask) {
      if (seqcol == GS_BLACK) {
        continue;
      } else {
        epix = row - 1;
        return 0;
      }
    } else {
      if (seqcol == GS_BLACK) {
        epix = row - 1;
        return 0;
      } else {
        continue;
      }
    }
  }

  epix = plc_->height;
  return 0;
}

}  // namespace odb
