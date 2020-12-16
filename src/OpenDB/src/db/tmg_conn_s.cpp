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
#include <stdlib.h>

#include "db.h"
#include "dbShape.h"
#include "dbWireCodec.h"
#include "tmg_conn.h"

namespace odb {

// very simple slow implementation for testing

struct tcs_shape;
struct tcs_lev;

struct tcs_shape
{
  tcs_shape* next;
  int        lev;
  int        xlo, ylo, xhi, yhi;
  int        isVia;
  int        id;
};

struct tcs_lev
{
  tcs_shape* shape_list;
  tcs_shape* last_shape;
  int        xlo, ylo, xhi, yhi;
  int        n;
  tcs_lev*   left;
  tcs_lev*   right;
  tcs_lev*   parent;
};

class tmg_conn_search_internal
{
 public:
  tmg_conn_search_internal();
  ~tmg_conn_search_internal();
  void clear();
  void printShape(dbNet* net, int lev, char* filenm);
  void addShape(int lev, int xlo, int ylo, int xhi, int yhi, int isVia, int id);
  void searchStart(int lev, int xlo, int ylo, int xhi, int yhi, int isVia);
  bool searchNext(int* id);
  void setXmin(int xmin);
  void setXmax(int xmax);
  void setYmin(int ymin);
  void setYmax(int ymax);
  void resetSorted() { _sorted = false; }
  void sort();

 private:
  void sort1(tcs_lev*);
  void merge(tcs_lev* bin, tcs_lev* left, tcs_lev* right);

  tcs_shape** _shV;
  int         _shJ;
  int         _shJmax;
  int         _shN;
  tcs_lev     _levAllV[32768];
  int         _levAllN;
  tcs_lev*    _levV[32];
  int         _sxlo, _sylo, _sxhi, _syhi;
  int         _srcVia;
  tcs_lev*    _bin;
  tcs_shape*  _cur;
  tcs_shape*  _pcur;
  bool        _sorted;
};

tmg_conn_search_internal::tmg_conn_search_internal()
{
  _shN    = 0;
  _shJ    = 0;
  _shJmax = 256;
  _shV    = (tcs_shape**) malloc(_shJmax * sizeof(tcs_shape*));
  int j;
  for (j = 0; j < _shJmax; j++)
    _shV[j] = NULL;
  _shV[0] = (tcs_shape*) malloc(32768 * sizeof(tcs_shape));
  clear();
}

tmg_conn_search_internal::~tmg_conn_search_internal()
{
  int j;
  for (j = 0; j < _shJmax; j++)
    if (_shV[j])
      free(_shV[j]);
  free(_shV);
}

void tmg_conn_search_internal::clear()
{
  _shN     = 0;
  _shJ     = 0;
  _levAllN = 0;
  int j;
  for (j = 0; j < 32; j++) {
    _levV[j]             = _levAllV + _levAllN++;
    _levV[j]->shape_list = NULL;
    _levV[j]->last_shape = NULL;
    _levV[j]->left       = NULL;
    _levV[j]->right      = NULL;
    _levV[j]->parent     = NULL;
    _levV[j]->n          = 0;
    _levV[j]->xlo        = 0;
    _levV[j]->ylo        = 0;
    _levV[j]->xhi        = 0;
    _levV[j]->yhi        = 0;
  }
  _sorted = false;
}

void tmg_conn_search_internal::printShape(dbNet* net, int lev, char* filenm)
{
  FILE* fp;
  if (filenm && filenm[0] != '\0')
    fp = fopen(filenm, "w");
  else
    fp = stdout;
  fprintf(fp,
          "Shapes of net %d %s level %d in tmg_conn_search:\n",
          net->getId(),
          net->getConstName(),
          lev);
  tcs_shape* shape = _levV[lev]->shape_list;
  while (shape) {
    fprintf(fp,
            "   %d %d    %d %d\n",
            shape->xlo,
            shape->ylo,
            shape->xhi,
            shape->yhi);
    shape = shape->next;
  }
  if (fp != stdout)
    fclose(fp);
}

void tmg_conn_search_internal::addShape(int lev,
                                        int xlo,
                                        int ylo,
                                        int xhi,
                                        int yhi,
                                        int isVia,
                                        int id)
{
  if (_shN == 32768) {
    if (_shJ == _shJmax) {
      int j = _shJmax;
      _shJmax *= 2;
      _shV = (tcs_shape**) realloc(_shV, _shJmax * sizeof(tcs_shape*));
      for (; j < _shJmax; j++)
        _shV[j] = NULL;
    }
    _shJ++;
    if (!_shV[_shJ])
      _shV[_shJ] = (tcs_shape*) malloc(32768 * sizeof(tcs_shape));
    _shN = 0;
  }
  tcs_shape* s  = _shV[_shJ] + _shN++;
  s->lev        = lev;
  s->xlo        = xlo;
  s->ylo        = ylo;
  s->xhi        = xhi;
  s->yhi        = yhi;
  s->isVia      = isVia;
  s->id         = id;
  s->next       = NULL;
  tcs_lev* slev = _levV[lev];
  if (slev->shape_list == NULL) {
    slev->shape_list = s;
    slev->xlo        = xlo;
    slev->ylo        = ylo;
    slev->xhi        = xhi;
    slev->yhi        = yhi;
  } else {
    slev->last_shape->next = s;
    if (xlo < slev->xlo)
      slev->xlo = xlo;
    if (ylo < slev->ylo)
      slev->ylo = ylo;
    if (xhi > slev->xhi)
      slev->xhi = xhi;
    if (yhi > slev->yhi)
      slev->yhi = yhi;
  }
  slev->last_shape = s;
  slev->n++;
}

void tmg_conn_search_internal::setXmin(int xmin)
{
  _pcur->xlo = xmin;
}

void tmg_conn_search_internal::setXmax(int xmax)
{
  _pcur->xhi = xmax;
}

void tmg_conn_search_internal::setYmin(int ymin)
{
  _pcur->ylo = ymin;
}

void tmg_conn_search_internal::setYmax(int ymax)
{
  _pcur->yhi = ymax;
}
void tmg_conn_search_internal::searchStart(int lev,
                                           int xlo,
                                           int ylo,
                                           int xhi,
                                           int yhi,
                                           int isVia)
{
  if (!_sorted) {
    sort();
  }
  _bin    = _levV[lev];
  _cur    = _bin->shape_list;
  _pcur   = NULL;
  _sxlo   = xlo;
  _sylo   = ylo;
  _sxhi   = xhi;
  _syhi   = yhi;
  _srcVia = isVia;
}

// _srcVia = 0 ==> wire
//         = 1 ==> via
//         = 2 ==> pin
bool tmg_conn_search_internal::searchNext(int* id)
{
  *id = -1;
#if 1
  if (!_bin)
    return false;
  // this is for speed for ordinary small nets
  if (_srcVia == 1 && !_bin->parent && !_bin->left && !_bin->right) {
    while (_cur) {
      if (_cur->xlo >= _sxhi || _sxlo >= _cur->xhi || _cur->ylo >= _syhi
          || _sylo >= _cur->yhi) {
        _cur = _cur->next;
        continue;
      }
      *id   = _cur->id;
      _pcur = _cur;
      _cur  = _cur->next;
      return true;
    }
    return false;
  }
#endif

  while (_bin) {
    bool not_here = false;
    if (_bin->xhi < _sxlo || _bin->xlo > _sxhi || _bin->yhi < _sylo
        || _bin->ylo > _syhi)
      not_here = true;
    if (!not_here)
      while (_cur) {
        if (_srcVia == 1 || _cur->isVia == 1) {
          if (_cur->xlo >= _sxhi || _sxlo >= _cur->xhi || _cur->ylo >= _syhi
              || _sylo >= _cur->yhi) {
            _cur = _cur->next;
            continue;
          }
        } else {
          if (_cur->xlo > _sxhi || _sxlo > _cur->xhi || _cur->ylo > _syhi
              || _sylo > _cur->yhi) {
            _cur = _cur->next;
            continue;
          }
          if (_srcVia == 0 && (_cur->xlo == _sxhi || _sxlo == _cur->xhi)) {
            if (_cur->yhi < _sylo || _cur->ylo > _syhi
                || (_cur->yhi < _syhi && _cur->ylo < _sylo)
                || (_cur->yhi > _syhi && _cur->ylo > _sylo)) {
              _cur = _cur->next;
              continue;
            }
          } else if (_srcVia == 0
                     && (_cur->ylo == _syhi || _sylo == _cur->yhi)) {
            if (_cur->xhi < _sxlo || _cur->xlo > _sxhi
                || (_cur->xhi < _sxhi && _cur->xlo < _sxlo)
                || (_cur->xhi > _sxhi && _cur->xlo > _sxlo)) {
              _cur = _cur->next;
              continue;
            }
          }
        }
        *id   = _cur->id;
        _pcur = _cur;
        _cur  = _cur->next;
        return true;
      }
    if (_bin->left) {
      _bin  = _bin->left;
      _cur  = _bin->shape_list;
      _pcur = NULL;
    } else {
      while (_bin->parent && _bin == _bin->parent->right)
        _bin = _bin->parent;
      _bin = _bin->parent;
      if (_bin) {
        _bin  = _bin->right;
        _cur  = _bin->shape_list;
        _pcur = NULL;
      }
    }
  }
  return false;
}

void tmg_conn_search_internal::sort()
{
  _sorted = true;
  int j;
  for (j = 0; j < 32; j++)
    if (_levV[j]->n > 1024)
      sort1(_levV[j]);
}

static void tcs_lev_init(tcs_lev* bin)
{
  bin->shape_list = NULL;
  bin->last_shape = NULL;
  bin->left       = NULL;
  bin->right      = NULL;
  bin->parent     = NULL;
  bin->n          = 0;
}

static void tcs_lev_add(tcs_lev* bin, tcs_shape* s)
{
  if (bin->shape_list == NULL) {
    bin->shape_list = s;
    bin->xlo        = s->xlo;
    bin->ylo        = s->ylo;
    bin->xhi        = s->xhi;
    bin->yhi        = s->yhi;
  } else {
    bin->last_shape->next = s;
    if (s->xlo < bin->xlo)
      bin->xlo = s->xlo;
    if (s->ylo < bin->ylo)
      bin->ylo = s->ylo;
    if (s->xhi > bin->xhi)
      bin->xhi = s->xhi;
    if (s->yhi > bin->yhi)
      bin->yhi = s->yhi;
  }
  bin->last_shape = s;
  bin->n++;
}

static void tcs_lev_add_no_bb(tcs_lev* bin, tcs_shape* s)
{
  if (bin->shape_list == NULL) {
    bin->shape_list = s;
  } else {
    bin->last_shape->next = s;
  }
  bin->last_shape = s;
  bin->n++;
}

static void tcs_lev_wrap(tcs_lev* bin)
{
  if (bin->last_shape)
    bin->last_shape->next = NULL;
}

void tmg_conn_search_internal::sort1(tcs_lev* bin)
{
  if (_levAllN >= 32767)
    return;
  if (bin->n < 1024)
    return;
  tcs_lev* left = _levAllV + _levAllN++;
  tcs_lev_init(left);
  left->parent   = bin;
  tcs_lev* right = _levAllV + _levAllN++;
  tcs_lev_init(right);
  right->parent  = bin;
  tcs_shape* s   = bin->shape_list;
  tcs_lev*   par = bin->parent;
  tcs_lev_init(bin);
  bin->parent = par;
  bin->left   = left;
  bin->right  = right;
  if (bin->xhi - bin->xlo >= bin->yhi - bin->ylo) {
    int xmid = (bin->xlo + bin->xhi) / 2;
    for (; s; s = s->next) {
      if (s->xhi < xmid)
        tcs_lev_add(left, s);
      else if (s->xlo > xmid)
        tcs_lev_add(right, s);
      else
        tcs_lev_add_no_bb(bin, s);
    }
  } else {
    int ymid = (bin->ylo + bin->yhi) / 2;
    for (; s; s = s->next) {
      if (s->yhi < ymid)
        tcs_lev_add(left, s);
      else if (s->ylo > ymid)
        tcs_lev_add(right, s);
      else
        tcs_lev_add_no_bb(bin, s);
    }
  }
  tcs_lev_wrap(bin);
  tcs_lev_wrap(left);
  tcs_lev_wrap(right);
  sort1(left);
  sort1(right);
  // merge(bin, left, right);
}

void tmg_conn_search_internal::merge(tcs_lev* bin,
                                     tcs_lev* left,
                                     tcs_lev* right)
{
  tcs_shape* shapeList = NULL;
  tcs_shape* lastShape = NULL;
  if (left->shape_list) {
    shapeList = left->shape_list;
    lastShape = left->last_shape;
  }
  if (bin->shape_list) {
    if (shapeList)
      lastShape->next = bin->shape_list;
    else
      shapeList = bin->shape_list;
    lastShape = bin->last_shape;
  }
  if (right->shape_list) {
    if (shapeList)
      lastShape->next = right->shape_list;
    else
      shapeList = right->shape_list;
    lastShape = right->last_shape;
  }
  bin->shape_list = shapeList;
  bin->last_shape = lastShape;
}

/////////////////////////////////////////////

tmg_conn_search::tmg_conn_search()
{
  _d = new tmg_conn_search_internal();
}

tmg_conn_search::~tmg_conn_search()
{
  delete _d;
  _d = NULL;
}

void tmg_conn_search::clear()
{
  _d->clear();
}

void tmg_conn_search::resetSorted()
{
  _d->resetSorted();
}

void tmg_conn_search::sort()
{
  _d->sort();
}

void tmg_conn_search::printShape(dbNet* net, int lev, char* filenm)
{
  _d->printShape(net, lev, filenm);
}

void tmg_conn_search::addShape(int lev,
                               int xlo,
                               int ylo,
                               int xhi,
                               int yhi,
                               int isVia,
                               int id)
{
  _d->addShape(lev, xlo, ylo, xhi, yhi, isVia, id);
}

void tmg_conn_search::setXmin(int xmin)
{
  _d->setXmin(xmin);
}

void tmg_conn_search::setXmax(int xmax)
{
  _d->setXmax(xmax);
}

void tmg_conn_search::setYmin(int ymin)
{
  _d->setYmin(ymin);
}

void tmg_conn_search::setYmax(int ymax)
{
  _d->setYmax(ymax);
}

void tmg_conn_search::searchStart(int lev,
                                  int xlo,
                                  int ylo,
                                  int xhi,
                                  int yhi,
                                  int isVia)
{
  _d->searchStart(lev, xlo, ylo, xhi, yhi, isVia);
}

bool tmg_conn_search::searchNext(int* id)
{
  bool ret = _d->searchNext(id);
  return ret;
}

}  // namespace odb
