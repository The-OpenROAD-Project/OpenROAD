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
  _allocSEQ = false;
  _init     = INIT;

  pixint sum = PIXFILL;
  int    i;
  for (i = 0; i < PIXMAPGRID; i++) {
    start[i] = sum;
    sum      = (sum >> 1);
  }

  sum       = PIXMAX;
  pixint s2 = sum;
  for (i = 0; i < PIXMAPGRID; i++) {
    end[i] = sum;
    sum    = (sum >> 1) | PIXMAX;

    middle[i] = s2;
    s2        = (s2 >> 1);
  }

  nslices  = -1;
  maxslice = -1;

  precolor[0]  = 0xff0000;
  precolor[1]  = 0x00ff00;
  precolor[2]  = 0x20cfff;
  precolor[3]  = 0xffff00;
  precolor[4]  = 0x880000;
  precolor[5]  = 0x007700;
  precolor[6]  = 0x0000ff;
  precolor[7]  = 0xff9000;
  precolor[8]  = 0xff8080;
  precolor[9]  = 0x607710;
  precolor[10] = 0x602090;
  precolor[11] = 0x808080;
  precolor[12] = 0xe040e0;
  precolor[13] = 0x88ff88;
  precolor[14] = 0x000080;
  precolor[15] = 0x805010;

  if (pool != NULL) {
    _seqPool = pool;
  } else {
    _seqPool  = new AthPool<SEQ>(false, 1024);
    _allocSEQ = true;
  }
}

gs::~gs()
{
  free_mem();
  if (_allocSEQ)
    delete _seqPool;
}

void gs::init_pixcol()
{
  for (int i = 0; i < nslices; i++) {
    colorize(i, precolor[i]);
    if (i == MAXPRECOLOR)
      break;
  }
}

void gs::init_pixbuff()
{
  pixbuff[0] = 0x00;
  pixbuff[1] = 0x00;
  pixbuff[2] = 0x00;
  pixbuff[3] = '\0';
}

void gs::dump_mem(pixmap* pm, int size)
{
  int i;
  fprintf(stderr, "DM: %d\n", size);
  for (i = 0; i < size; i++) {
    fprintf(stderr, "%x ", (unsigned int) pm[i].lword);
    if ((i % 1000) == 0) {
      fprintf(stderr, "\n");
    }
  }
  fprintf(stderr, "End of DM: %d\n", i);
}
void gs::check_mem()
{
  if ((_init & ALLOCATED)) {
    int y;
    int pl;
    for (y = 0; y < plc->height; y++) {
      fprintf(stderr, "Row: %d\n", y);
      for (pl = 0; pl < nslices; pl++) {
        fprintf(stderr, "Plane: %d at %ld\n", pl, (long int) pldata[pl]->plane);
        dump_row(y, pl);
      }
    }
  }
}

int gs::free_mem()
{
  if ((_init & ALLOCATED)) {
    for (int s = 0; s < nslices; s++) {
      if (((plconfig*) pldata[s])->plalloc)
        free(((plconfig*) pldata[s])->plalloc);
      free((plconfig*) pldata[s]);
    }
    free((struct rgb*) pixcol);

    _init = (_init & (~ALLOCATED));
    return 0;
  } else {
    return -1;
  }
}

int gs::alloc_mem()
{
  if ((_init & ALLOCATED)) {
    free_mem();
  }

  if ((_init & SLICES)) {
    pixcol = (struct rgb*) calloc(nslices, sizeof(struct rgb));
    plptr  = (pixmap**) calloc(nslices, sizeof(pixmap*));
    pldata = (plconfig**) calloc(nslices, sizeof(plconfig*));
    // fprintf(stderr,"Allocing pixcol at: %x (%d)\n",pixcol,
    // nslices*sizeof(struct rgb));

    for (int s = 0; s < nslices; s++) {
      pldata[s]          = (plconfig*) calloc(1, sizeof(plconfig));
      pldata[s]->plalloc = NULL;
    }

    _init |= ALLOCATED;

    // check_mem();

    init_pixbuff();
    init_pixcol();
    return 0;
  } else {
    return -1;
  }
}

int gs::clear_layer(int layer)
{
  if ((layer < 0) || (layer >= nslices)) {
    return -1;
  }
  plc = pldata[layer];
  if (plc->plalloc != NULL) {
    memset(plc->plalloc,
           0,
           (plc->height * plc->pixstride + PIXADJUST) * sizeof(pixmap));
  }
  return 0;
}

void gs::init_headers(int width, int height)
{
  // find maximum size

  // Initialize headers - does NOT depend on memory allocation
  sprintf(ppmheader, "P6\n%d %d\n255\n", width, height);
}

int gs::setSlices(int _nslices, bool /* unused: skipMemAlloc */)
{
  // fprintf(stderr,"setSlices: %d\n",_nslices);
  free_mem();
  nslices  = _nslices;
  maxplane = nslices - 1;
  _init |= SLICES;
  // alloc_mem(skipMemAlloc);
  alloc_mem();

  return 0;
}

int gs::setSize(int  pl,
                int  _xres,
                int  _yres,
                int  _x0,
                int  _y0,
                int  _x1,
                int  _y1,
                bool skipPixmap)
{
  plc = pldata[pl];

  plc->x0 = _x0;
  plc->x1 = _x1;
  plc->y0 = _y0;
  plc->y1 = _y1;

  if (plc->x1 <= plc->x0) {
    // to avoid things like divide by 0, etc
    plc->x1 = plc->x0 + 1;
  }

  if (plc->y1 <= plc->y0) {
    // to avoid things like divide by 0, etc
    plc->y1 = plc->y0 + 1;
  }

  plc->xres = _xres;
  plc->yres = _yres;

  plc->width = (plc->x1 - plc->x0 + 1) / plc->xres;
  if (((plc->x1 - plc->x0 + 1) % plc->xres) != 0) {
    plc->width++;
  }

  plc->height = (plc->y1 - plc->y0 + 1) / plc->yres;
  if (((plc->y1 - plc->y0 + 1) % plc->yres) != 0) {
    plc->height++;
  }

  // fprintf(stderr,"Slice number: %d\n",pl);
  // fprintf(stderr,"xres/yres: %d, %d\n",plc->xres,plc->yres);
  // fprintf(stderr,"x0,x1 y0,y1: %d,%d
  // %d,%d\n",plc->x0,plc->x1,plc->y0,plc->y1); fprintf(stderr,"Height/Width:
  // %d, %d\n",plc->height,plc->width);

  plc->pixwrem = plc->width % PIXMAPGRID;

  plc->pixwidth = plc->width;
  if (plc->pixwrem != 0) {
    // round off to next multiple of PIXMAPGRID
    plc->pixwidth += (PIXMAPGRID - (plc->width % PIXMAPGRID));
  }

  plc->pixstride = plc->pixwidth / PIXMAPGRID;

  plc->pixfullblox = plc->pixstride;
  if (plc->pixwrem != 0) {
    plc->pixfullblox--;
  }
  plc->pixwrem--;

  if (skipPixmap) {
    plc->plalloc = NULL;
    plc->plane   = NULL;
    return 0;
  }

  // fprintf(stderr,"pixwrem: %d, plc->pixstride: %d\n",pixwrem,plc->pixstride);
  // fprintf(stderr,"pixwidth: %d, plc->pixfullblox:
  // %d\n",pixwidth,plc->pixfullblox);

  // fprintf(stderr,"Sizes: %d, %d, %d,
  // %d\n",sizeof(pixint),sizeof(pixints),sizeof(unsigned short int),
  // sizeof(unsigned char));

  //    uint CNT = plc->height*plc->pixstride+PIXADJUST;
  pixmap* pm = (pixmap*) calloc(plc->height * plc->pixstride + PIXADJUST,
                                sizeof(pixmap));
  if (pm == NULL) {
    fprintf(stderr,
            "Error: not enough memory available trying to allocate plane %d\n",
            pl);
    exit(-1);
  }

  plc->plalloc = pm;
  // align on a 16-byte boundary
  // pm = (pixmap*)(((char*)pm)+15);
  // pm = (pixmap*) ((long)pm >> 4);
  // pm = (pixmap*) ((long)pm << 4);
  plc->plane = pm;

  return 0;
}

int gs::configureSlice(int  _slicenum,
                       int  _xres,
                       int  _yres,
                       int  _x0,
                       int  _y0,
                       int  _x1,
                       int  _y1,
                       bool skipAlloc)
{
  if (_init & ALLOCATED) {
    if (_slicenum < nslices) {
      setSize(_slicenum, _xres, _yres, _x0, _y0, _x1, _y1, skipAlloc);
    }
  }

  return 0;
}

// CLIP MACRO
#define CLIP(p, min, max) p = (p < min) ? min : (p >= max) ? (max - 1) : p

int gs::box(int px0, int py0, int px1, int py1, int sl, bool checkOnly)
{
  if ((sl < 0) || (sl > nslices)) {
    fprintf(stderr,
            "Box in slice %d exceeds maximum configured slice count %d - "
            "ignored!\n",
            sl,
            nslices);
    return -1;
  }

  if (!(_init & GS_ALL)) {
    return -1;
  }

  if (sl > maxslice) {
    maxslice = sl;
  }

  plc = pldata[sl];

  // fprintf(stderr,"Box from: %d,%d to %d,%d\n",px0,py0,px1,py1);

  // normalize bbox
  long a;
  if (px0 > px1) {
    a   = px0;
    px0 = px1;
    px1 = a;
  }
  if (py0 > py1) {
    a   = py0;
    py0 = py1;
    py1 = a;
  }

  if ((px0 < plc->x0) && (px1 < plc->x0))
    return -1;
  if ((px0 > plc->x1) && (px1 > plc->x1))
    return -1;
  if ((py0 < plc->y0) && (py1 < plc->y0))
    return -1;
  if ((py0 > plc->y1) && (py1 > plc->y1))
    return -1;

  if (checkOnly)
    return 0;

  // convert to pixel space
  int cx0 = int((px0 - plc->x0) / plc->xres);
  int cx1 = int((px1 - plc->x0) / plc->xres);
  int cy0 = int((py0 - plc->y0) / plc->yres);
  int cy1 = int((py1 - plc->y0) / plc->yres);

  // fprintf(stderr,"Starting new box: (%d,%d) to (%d,%d)!\n",cx0,cy0,cx1,cy1);
  // render a rectangle on the selected slice. Paint all pixels
  CLIP(cx0, 0, plc->width);
  CLIP(cx1, 0, plc->width);
  CLIP(cy0, 0, plc->height);
  CLIP(cy1, 0, plc->height);
  // fprintf(stderr,"Clipped Box from: %d,%d to %d,%d\n",cx0,cy0,cx1,cy1);
  // now fill in planes object

  pixmap* pm;
  pixmap* pcb;

  // xbs = x block start - block the box starts in
  int xbs = cx0 / PIXMAPGRID;
  // xbs = x block end - block the box ends in
  int xbe = cx1 / PIXMAPGRID;

  // int xss = cx0 % PIXMAPGRID;
  int xee = cx1 % PIXMAPGRID;

  pixint smask = start[cx0 % PIXMAPGRID];
  pixint emask = end[xee];

  int mb;

  if (xbe == xbs) {
    smask = smask & emask;
  }

  pm = plc->plane + plc->pixstride * cy0 + xbs;

  for (int yb = cy0; yb <= cy1; yb++) {
    // start block
    pcb = pm;

    // for next time through loop - allow compiler time for out-of-order
    pm += plc->pixstride;

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

    // fprintf(stderr,"before: %x, after: %x (%d)\n",pm, pm+plc->pixstride,
    // plc->pixstride);
  }

  return 0;
}

// generate an RGB image based on the colors
int gs::create_image(FILE*  fp,
                     char** src,
                     int    output,
                     int    encoding,
                     int    width,
                     int    height,
                     int*   ll,
                     int*   ur)
{
  init_headers(width, height);

  int rc = 0;
  // file-based output

  if (output == GS_FILE) {
    rc += write_ppm_file(fp, encoding, width, height, ll, ur);
    fflush(fp);
  }
  // string-based output
  else {
    rc += write_ppm_string(src, encoding, width, height, ll, ur);
  }

  return rc;
}

int gs::check_slice(int sl)
{
  if ((sl < 0) || (sl > nslices)) {
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

  pixmap* sp1 = pldata[sl1]->plane;
  pixmap* sp2 = pldata[sl2]->plane;
  pixmap* ssp = pldata[store]->plane;

  pixmap* ep = sp1 + (plc->height * plc->pixstride);
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

  pixmap* sp1 = pldata[sl1]->plane;
  pixmap* sp2 = pldata[sl2]->plane;
  pixmap* ssp = pldata[store]->plane;

  pixmap* ep = sp1 + (plc->height * plc->pixstride);
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

  pixmap* sp1 = pldata[sl1]->plane;
  pixmap* sp2 = pldata[sl2]->plane;
  pixmap* ssp = pldata[store]->plane;

  pixmap* ep = sp1 + (plc->height * plc->pixstride);
  while (sp1 < ep) {
    (ssp++)->lword = (sp1++)->lword ^ (sp2++)->lword;
  }
  return 0;
}

SEQ* gs::salloc()
{
  SEQ* s = _seqPool->alloc();
#ifdef DEBUG_SEQ_MEM
  fprintf(stderr, "Alloced sequence address %x\n", s);
#endif
  return s;
}

void gs::show_seq(SEQ* s)
{
  fprintf(stderr,
          "Sequence from %d,%d to %d,%d type %d\n",
          s->_ll[0],
          s->_ll[1],
          s->_ur[0],
          s->_ur[1],
          s->type);
}
void gs::release(SEQ* s)
{
  _seqPool->free(s);
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

uint gs::get_seq(int*                ll,
                 int*                ur,
                 uint                order,
                 uint                plane,
                 Ath__array1D<SEQ*>* array)
{
  if (check_slice(plane) != 0) {
    return 0;
  }

  plc = pldata[plane];

  // Sanity checks
  if (ur[0] < plc->x0)
    return 0;
  if (ll[0] > plc->x1)
    return 0;

  if (ur[1] < plc->y0)
    return 0;
  if (ll[1] > plc->y1)
    return 0;

  if (ll[0] < plc->x0)
    ll[0] = plc->x0;

  if (ur[0] > plc->x1)
    ur[0] = plc->x1;

  if (ll[1] < plc->y0)
    ll[1] = plc->y0;

  if (ur[1] > plc->y1)
    ur[1] = plc->y1;
  // End Sanity Checks

  SEQ* s = salloc();

  // convert into internal coordinates
  int cx0 = int((ll[0] - plc->x0) / plc->xres);
  int cy0 = int((ll[1] - plc->y0) / plc->yres);

  int cx1 = int((ur[0] - plc->x0) / plc->xres);
  if (((ur[0] - plc->x0 + 1) % plc->xres) != 0) {
    cx1++;
    // fprintf(stderr,"incrementing (row)!\n");
  } else {
    // fprintf(stderr,"Not incrementing (row): %d, %d,
    // %d\n",ur[0],plc->x0,plc->xres);
  }
  int cy1 = int((ur[1] - plc->y0) / plc->yres);
  if (((ur[1] - plc->y0 + 1) % plc->yres) != 0) {
    cy1++;
    // fprintf(stderr,"Incrementing (col)!\n");
  } else {
    // fprintf(stderr,"Not incrementing (col): %d, %d,
    // %d\n",ur[1],plc->y0,plc->yres);
  }
  // fprintf(stderr,"Converted: %d,%d to %d,%d\n",cx0,cy0,cx1,cy1);

  int  row, col;
  int  rs, re;
  int  cs, ce;
  int  blacksum = 0;
  int  start, end;
  bool flag;

  if (order == GS_ROW) {
    // rs = cy0*plc->yres + plc->y0;
    // re = rs + plc->yres - 1;

    rs = ll[1];
    re = ur[1];

    for (row = cy0; row <= cy1; row++) {
      // fprintf(stderr,"row: %d\n",row);
      // if ( re > ur[1] )
      //{
      // re = ur[1];
      //}

      start = cx0;
      end   = cx1;
      flag  = false;
      while (get_seqrow(row, plane, start, end, s->type) == 0) {
        // fprintf(stderr,"Raw Sequence results: start = %d, end =
        // %d\n",start,end);
        s->_ll[0] = (int) (start * (plc->xres) + plc->x0);
        s->_ur[0] = (int) ((end + 1) * (plc->xres) + (plc->x0) - 1);
        if (s->_ur[0] >= ur[0]) {
          s->_ur[0] = ur[0];
          flag      = true;
        }

        if ((row == cy0) && (s->_ll[0] < ll[0]))
          s->_ll[0] = ll[0];

        s->_ll[1] = rs;
        s->_ur[1] = re;

        // show_seq(s);

        if (s->type == GS_BLACK) {
          // fprintf(stderr,"Blacksum before: %d\n",blacksum);
          // blacksum += ((re-rs+1) * (s->_ur[0] - s->_ll[0] + 1));
          blacksum += (s->_ur[0] - s->_ll[0]);
          // fprintf(stderr,"Blacksum afer  : %d\n",blacksum);
        }

        if (array != NULL) {
          array->add(s);
          s = salloc();
        }
        if (flag == true) {
          _seqPool->free(s);
          return blacksum;
        }
        start = end + 1;
      }
      rs += plc->yres;
      if (rs > ur[1])
        break;
      re += plc->yres;
    }
  } else if (order == GS_COLUMN) {
    cs = ((cx1 + cx0) / 2) * plc->xres + plc->x0;
    ce = cs;
    // cs = cx0*plc->xres + plc->x0;
    // ce = cs + plc->xres -1;

    if (cs < ll[0])
      cs = ll[0];

    if (ce > ur[0])
      ce = ur[0];

    for (col = cx0; col <= cx0; col++) {
      start = cy0;
      flag  = false;
      while (get_seqcol(col, plane, start, end, s->type) == 0) {
        s->_ll[1] = (int) (start * plc->yres + plc->y0);
        s->_ur[1] = (int) ((end + 1) * plc->yres + plc->y0 - 1);
        if (s->_ur[1] >= ur[1]) {
          flag      = true;
          s->_ur[1] = ur[1];
        }

        if ((col == cx0) && (s->_ll[1] < ll[1]))
          s->_ll[1] = ll[1];

        s->_ll[0] = cs;
        s->_ur[0] = ce;

        // show_seq(s);

        if (s->type == GS_BLACK) {
          blacksum += (s->_ur[1] - s->_ll[1]);
        }

        if (array != NULL) {
          array->add(s);
          s = salloc();
        }
        if (flag == true) {
          _seqPool->free(s);
          return blacksum;
        }
        start = end + 1;
      }
      cs += plc->xres;
      if (cs > ur[0]) {
        // fprintf(stderr,"breaking out early: %d, %d\n",cs, ur[0]);
        break;
      }
      ce += plc->xres;
    }
  }
  _seqPool->free(s);
  return blacksum;
}

//#define DEBUG
int gs::get_seqrow(int y, int plane, int stpix, int& epix, int& seqcol)
{
  if (!(_init & ALLOCATED)) {
    return -1;
  }

  if (y >= plc->height)
    return -1;

  if (stpix >= plc->width)
    return -1;

#ifdef DEBUG
  fprintf(stderr, "GSR: %d/%d, %d, %d\n", y, plane, stpix, epix);
#endif
  // pixint *bmask;
  int st;
  int bit;

  long offset;

  int sto = stpix / PIXMAPGRID;
  int str = stpix - (sto * PIXMAPGRID);

#ifdef DEBUG
  fprintf(stderr, "GSR2: %d/%d\n", sto, str);
#endif
  offset     = y * plc->pixstride + sto;
  pixmap* pl = pldata[plane]->plane + offset;

  seqcol = GS_NONE;
  pixint bc;

  // take care of start
#ifdef DEBUG
  fprintf(
      stderr, "%x, %x, %x\n", pl->lword, start[str], pl->lword & start[str]);
#endif
  if ((pl->lword & (start[str])) == start[str]) {
    seqcol = GS_BLACK;
    sto++;
    pl++;
  } else if ((pl->lword & (start[str])) == 0) {
    seqcol = GS_WHITE;
    sto++;
    pl++;
  } else {
    // ends here already
    bc = (pl->lword) & middle[str];
    if (bc != 0) {
      seqcol = GS_BLACK;
    } else {
      seqcol = GS_WHITE;
    }
    epix = (sto * PIXMAPGRID) + str;
    for (bit = str + 1; bit < PIXMAPGRID; bit++) {
#ifdef DEBUG
      fprintf(stderr, "HERE (%d/%d)!\n", bit, seqcol);
      fprintf(stderr,
              "%x, %x, %d\n",
              pl->lword,
              middle[bit],
              (pl->lword) & (middle[bit]));
#endif
      if (((pl->lword) & middle[bit]) == 0) {
#ifdef DEBUG
        fprintf(stderr, "0\n");
#endif

        if (seqcol == GS_BLACK) {
          break;
        }
      } else {
#ifdef DEBUG
        fprintf(stderr, "1\n");
#endif
        if (seqcol == GS_WHITE) {
          break;
        }
      }

      epix++;
    }
    return 0;
  }

  if (sto > plc->pixfullblox) {
    epix = plc->width - 1;
    return 0;
  }

  for (st = sto; st < plc->pixfullblox; st++) {
#ifdef DEBUG
    fprintf(stderr, "Continuing with st = %d, end = %d\n", sto, epix);
#endif

    if ((pl->lword) == PIXFILL) {
#ifdef DEBUG
      fprintf(stderr, "PM!\n");
#endif
      if (seqcol != GS_BLACK) {
        epix = st * PIXMAPGRID - 1;
        return 0;
      }
    } else if (pl->lword == 0) {
#ifdef DEBUG
      fprintf(stderr, "ZERO!\n");
#endif
      if (seqcol != GS_WHITE) {
        epix = st * PIXMAPGRID - 1;
        return 0;
      }
    } else {
#ifdef DEBUG
      fprintf(stderr, "MIXED: %x!\n", pl->lword);
#endif
      // ends here
      epix = st * PIXMAPGRID - 1;
      for (bit = 0; bit < PIXMAPGRID; bit++) {
        if (((pl->lword) & middle[bit]) == 0) {
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
#ifdef DEBUG
  fprintf(stderr, "end case: %d\n", epix);
#endif
  for (bit = 0; bit <= plc->pixwrem; bit++) {
    if (((pl->lword) & middle[bit]) == 0) {
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

//#define DEBUG
int gs::get_seqcol(int x, int plane, int stpix, int& epix, int& seqcol)
{
  if (!(_init & ALLOCATED)) {
    return -1;
  }

  if (x >= plc->width)
    return -1;

  if (stpix >= plc->height)
    return -1;

#ifdef DEBUG
  fprintf(stderr, "GSC: %d/%d, %d, %d\n", x, plane, stpix, epix);
#endif

  long offset;

  // get proper "word" offset
  int sto = x / PIXMAPGRID;
  int stc = x - (sto * PIXMAPGRID);

#ifdef DEBUG
  fprintf(stderr, "GSC2: %d/%d\n", sto, stc);
#endif
  int row         = stpix;
  offset          = sto + row * plc->pixstride;
  pixint  bitmask = middle[stc];
  pixmap* pl      = pldata[plane]->plane + offset;

  seqcol = GS_NONE;

  if ((pl->lword) & bitmask) {
    seqcol = GS_BLACK;
  } else {
    seqcol = GS_WHITE;
  }

  for (row = stpix + 1; row < plc->height; row++) {
    pl += plc->pixstride;
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

  epix = plc->height;
  return 0;
}

int gs::get_bw_count(int y, int plane, int& black, int& white)
{
  if (!(_init & ALLOCATED)) {
    return -1;
  }

  black = 0;
  white = 0;

  pixint* bmask;
  int     st;
  int     bit;

  long offset;

  offset     = y * plc->pixstride;
  pixmap* pl = pldata[plane]->plane + offset;

  for (st = 0; st < plc->pixfullblox; st++) {
    if (pl->lword == PIXFILL) {
      // fprintf(stderr,"b1\n");
      black += PIXMAPGRID;
    } else if (pl->lword == 0) {
      // fprintf(stderr,"w1\n");
      white += PIXMAPGRID;
    } else {
      bmask = middle;
      for (bit = 0; bit < PIXMAPGRID; bit++) {
        if ((pl->lword) & (*bmask)) {
          // fprintf(stderr,"b2\n");
          black++;
        } else {
          // fprintf(stderr,"w2\n");
          white++;
        }
        bmask++;
      }
    }
    pl++;
  }
  // handle non-even case
  bmask = middle;
  for (bit = 0; bit <= plc->pixwrem; bit++) {
    if ((pl->lword) & (*bmask)) {
      black++;
    } else {
      white++;
    }
    bmask++;
  }

  fprintf(
      stderr, "Row %d, plane %d, %d white, %d black\n", y, plane, white, black);
  return 0;
}

int gs::write_ppm_file(FILE* fp,
                       int   encoding,
                       int   width,
                       int   height,
                       int*  ll,
                       int*  ur)
{
  if (!(_init & ALLOCATED)) {
    return -1;
  }

  // fprintf(fp,"P3\n");
  struct rgb* pixlookup;

  char* buffptr;
  int   bufflen;

  if (encoding == 255) {
    pixlookup = pixcol;
    buffptr   = pixbuff;
    bufflen   = 3;
    fprintf(fp, "%s", ppmheader);
  } else {
    fprintf(stderr, "Unsupported encoding style!\n");
    return -1;
  }

  // just for testing
  // memset(plalloc[3],129,plc->height*(pixwidth+1)*sizeof(pixmap));

  int py;
  int px;
  int pln;

  char* outptr;

  int vll[2], vur[2];

  int xvres = (ur[0] - ll[0] + 1) / width;
  int yvres = (ur[1] - ll[1] + 1) / height;

  int xvirt;
  int yvirt;

  // pixint *bend=&middle[PIXMAPGRID];

  yvirt = ur[1];
  for (py = height - 1; py >= 0; py--) {
    xvirt = ll[0];
    for (px = 0; px < width; px++) {
      // pixel (px,py)
      // get this pixel in each plane

      vll[0] = xvirt;
      vll[1] = yvirt - (yvres - 1);
      vur[0] = xvirt + (xvres - 1);
      vur[1] = yvirt;
      // fprintf(stderr,"getPix: ll: %d, %d, ur: %d,
      // %d\n",vll[0],vll[1],vur[0],vur[1]); fprintf(stderr,"pixel: %d,
      // %d\n",px,py);
      outptr = buffptr;
      for (pln = maxplane; pln >= 0; pln--) {
        if (pldata[pln]->plalloc != NULL) {
          if (get_seq(vll, vur, GS_COLUMN, pln, NULL)) {
            // pixel is set
            // fprintf(stderr,"set in plane: %d!!\n",pln);
            outptr = pixlookup[pln].out;
            break;
          }
        }
      }
      fwrite(outptr, 1, bufflen, fp);
      xvirt += xvres;
    }
    yvirt -= yvres;
  }

  fflush(fp);
  return 0;
}

int gs::write_ppm_string(char** s,
                         int    encoding,
                         int /* unused: width */,
                         int /* unused: height */,
                         int* /* unused: ll */,
                         int* /* unused: ur */)
{
  if (!(_init & ALLOCATED)) {
    return -1;
  }

  // fprintf(fp,"P3\n");
  struct rgb* pixlookup;

  char* buffptr;
  int   bufflen;

  char* stout;

  if (encoding == 255) {
    pixlookup = pixcol;
    buffptr   = pixbuff;
    bufflen   = 3;
    // add a little extra for base-64 padding and header
    *s    = (char*) malloc(bufflen * plc->width * plc->height + 128);
    stout = *s;
    sprintf(stout, "%s", ppmheader);
  }
  {
    fprintf(stderr, "Unsupported encoding style!\n");
    return -1;
  }
  // fprintf(stderr,"Starting string write!\n");
  // dump_bytes(stout);
  stout = (stout + strlen(stout));

  // just for testing
  // memset(plalloc[3],129,plc->height*(pixwidth+1)*sizeof(pixmap));

  pixint* bmask;
  int     py;
  int     st;
  int     pln;
  int     bit;

  char* outptr;
  long  offset;

  // pixint *bend=&middle[PIXMAPGRID];
  int buffcopy;

  for (py = plc->height - 1; py >= 0; py--) {
    offset = py * plc->pixstride;
    for (pln = maxplane; pln >= 0; pln--) {
      plptr[pln] = pldata[pln]->plane + offset;
    }
    for (st = 0; st < plc->pixfullblox; st++) {
      bmask = middle;
      for (bit = 0; bit < PIXMAPGRID; bit++) {
        outptr = buffptr;
        for (pln = maxplane; pln >= 0; pln--) {
          if ((plptr[pln]->lword) & (*bmask)) {
            outptr = pixlookup[pln].out;
            break;
          }
        }
        for (buffcopy = 0; buffcopy < bufflen; buffcopy++) {
          *stout++ = *outptr++;
        }
        bmask++;
      }
      for (pln = maxplane; pln >= 0; pln--) {
        plptr[pln]++;
      }
    }
    // handle non-even case
    bmask = middle;
    for (bit = 0; bit <= plc->pixwrem; bit++) {
      outptr = buffptr;
      for (pln = maxplane; pln >= 0; pln--) {
        if ((plptr[pln]->lword) & (*bmask)) {
          outptr = pixlookup[pln].out;
          break;
        }
      }
      for (buffcopy = 0; buffcopy < bufflen; buffcopy++) {
        *stout++ = *outptr++;
      }
      bmask++;
    }
  }
  *stout = '\0';
  // fprintf(stderr,"Done string write!\n");
  // dump_bytes(*s);

  return 0;
}

int gs::get_max_slice()
{
  return maxslice;
}

int gs::get_rowcount(int sl)
{
  if ((sl < 0) || (sl > maxslice))
    return -1;

  return pldata[sl]->pixstride;
}

int gs::get_colcount(int sl)
{
  if ((sl < 0) || (sl > maxslice))
    return -1;

  return pldata[sl]->height;
}

int gs::get_cutoff(int sl)
{
  if ((sl < 0) || (sl > maxslice))
    return -1;

  return pldata[sl]->pixwrem;
}

pixmap* gs::get_array(int sl, int x, int y)
{
  if ((sl < 0) || (sl > maxslice))
    return NULL;

  return pldata[sl]->plane + pldata[sl]->pixstride * x + y;
}

char* gs::get_color_value(int slice)
{
  if ((slice < 0) || (slice > nslices)) {
    return NULL;
  }
  return pixcol[slice].out;
}

int gs::colorize(int slice, int rgb)
{
  if (!(_init & ALLOCATED)) {
    return -1;
  }

  /*
  if ( slice >= nslices )
  {
      fprintf(stderr,"Slice: %d, nslices: %d, rgb: %d\n",slice,nslices, rgb);
  }
  */
  assert(slice < nslices);
  assert(slice >= 0);
  char buff[4];
  sprintf(buff, "%c%c%c", R_COLOR(rgb), G_COLOR(rgb), B_COLOR(rgb));
  // fprintf(stderr,"COL: %x %x %x %x\n",&buff, &pixcol[slice].out[0],
  // &pixcol[slice].out[1], &pixcol[slice].out[2]);
  pixcol[slice].out[0] = buff[0];
  pixcol[slice].out[1] = buff[1];
  pixcol[slice].out[2] = buff[2];
  pixcol[slice].out[3] = '\0';

  return 0;
}

void gs::dump_bytes(char* s)
{
  fprintf(stderr, "First values:\n");
  if (s == NULL) {
    fprintf(stderr, "s is null!\n");
    return;
  }
  for (int i = 0; i < plc->width; i++) {
    fprintf(stderr, "%d: %x %x %x ", i, *(s + i), *(s + i + 1), *(s + i + 2));
    if ((i % 2) == 0) {
      fprintf(stderr, "\n");
    }
  }
}

void gs::dump_row(int row, int pl)
{
  fprintf(stderr, "Row %d dump for plane %d\n", row, pl);
  pixmap* pm = pldata[pl]->plane + (row * pldata[pl]->pixstride);
  for (int i = 0; i < pldata[pl]->pixstride; i++) {
    fprintf(stderr,
            "address: %ld (%ld, %d, %d, %d)\n",
            (long int) &(pm->lword),
            (long int) pldata[pl]->plane,
            i,
            row,
            pldata[pl]->pixstride);
    fprintf(stderr, "element %d: %lld\n", i, pm->lword);
    pm++;
  }
}

}  // namespace odb
