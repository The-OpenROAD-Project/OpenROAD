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

#include "odb/db.h"
#include "tmg_conn.h"

namespace odb {

struct tcs_shape
{
  tcs_shape* next = nullptr;
  Rect bounds;
  int lev = 0;
  int isVia = 0;
  int id = 0;

  int xMin() const { return bounds.xMin(); }
  int yMin() const { return bounds.yMin(); }
  int xMax() const { return bounds.xMax(); }
  int yMax() const { return bounds.yMax(); }
};

struct tcs_lev
{
  tcs_shape* shape_list = nullptr;
  tcs_shape* last_shape = nullptr;
  tcs_lev* left = nullptr;
  tcs_lev* right = nullptr;
  tcs_lev* parent = nullptr;
  Rect bounds;
  int n = 0;

  int xMin() const { return bounds.xMin(); }
  int yMin() const { return bounds.yMin(); }
  int xMax() const { return bounds.xMax(); }
  int yMax() const { return bounds.yMax(); }
};

class tmg_conn_search::Impl
{
 public:
  Impl();
  ~Impl();
  void clear();
  void addShape(int lev, const Rect& bounds, int isVia, int id);
  void searchStart(int lev, const Rect& bounds, int isVia);
  bool searchNext(int* id);

 private:
  void sort();
  void sort_level(tcs_lev* bin);

  tcs_shape** _shV;
  int _shJ;
  int _shJmax;
  int _shN;
  tcs_lev _levAllV[32768];
  int _levAllN;
  tcs_lev* _levV[32];
  Rect _search_box;
  int _srcVia;
  tcs_lev* _bin;
  tcs_shape* _cur;
  tcs_shape* _pcur;
  bool _sorted;

  static constexpr int sort_threshold = 1024;
};

tmg_conn_search::Impl::Impl()
{
  _shN = 0;
  _shJ = 0;
  _shJmax = 256;
  _shV = (tcs_shape**) malloc(_shJmax * sizeof(tcs_shape*));
  for (int j = 0; j < _shJmax; j++) {
    _shV[j] = nullptr;
  }
  _shV[0] = (tcs_shape*) malloc(32768 * sizeof(tcs_shape));
  _srcVia = 0;
  _bin = nullptr;
  _cur = nullptr;
  _pcur = nullptr;
  clear();
}

tmg_conn_search::Impl::~Impl()
{
  for (int j = 0; j < _shJmax; j++) {
    if (_shV[j]) {
      free(_shV[j]);
    }
  }
  free(_shV);
}

void tmg_conn_search::Impl::clear()
{
  _shN = 0;
  _shJ = 0;
  _levAllN = 0;
  for (int j = 0; j < 32; j++) {
    _levV[j] = _levAllV + _levAllN++;
    _levV[j]->shape_list = nullptr;
    _levV[j]->last_shape = nullptr;
    _levV[j]->left = nullptr;
    _levV[j]->right = nullptr;
    _levV[j]->parent = nullptr;
    _levV[j]->n = 0;
    _levV[j]->bounds = {0, 0, 0, 0};
  }
  _sorted = false;
}

void tmg_conn_search::Impl::addShape(int lev,
                                     const Rect& bounds,
                                     int isVia,
                                     int id)
{
  if (_shN == 32768) {
    if (_shJ == _shJmax) {
      int j = _shJmax;
      _shJmax *= 2;
      _shV = (tcs_shape**) realloc(_shV, _shJmax * sizeof(tcs_shape*));
      for (; j < _shJmax; j++) {
        _shV[j] = nullptr;
      }
    }
    _shJ++;
    if (!_shV[_shJ]) {
      _shV[_shJ] = (tcs_shape*) malloc(32768 * sizeof(tcs_shape));
    }
    _shN = 0;
  }
  tcs_shape* shape = _shV[_shJ] + _shN++;
  shape->lev = lev;
  shape->bounds = bounds;
  shape->isVia = isVia;
  shape->id = id;
  shape->next = nullptr;
  tcs_lev* slev = _levV[lev];
  if (slev->shape_list == nullptr) {
    slev->shape_list = shape;
    slev->bounds = shape->bounds;
  } else {
    slev->last_shape->next = shape;
    slev->bounds.merge(shape->bounds);
  }
  slev->last_shape = shape;
  slev->n++;
}

void tmg_conn_search::Impl::searchStart(int lev, const Rect& bounds, int isVia)
{
  if (!_sorted) {
    sort();
  }
  _bin = _levV[lev];
  _cur = _bin->shape_list;
  _pcur = nullptr;
  _search_box = bounds;
  _srcVia = isVia;
}

// _srcVia = 0 ==> wire
//         = 1 ==> via
//         = 2 ==> pin
bool tmg_conn_search::Impl::searchNext(int* id)
{
  *id = -1;
  if (!_bin) {
    return false;
  }
  // this is for speed for ordinary small nets
  if (_srcVia == 1 && !_bin->parent && !_bin->left && !_bin->right) {
    while (_cur) {
      if (_cur->bounds.overlaps(_search_box)) {
        *id = _cur->id;
        _pcur = _cur;
        _cur = _cur->next;
        return true;
      }
      _cur = _cur->next;
    }
    return false;
  }

  while (_bin) {
    bool not_here = false;
    if (!_bin->bounds.intersects(_search_box)) {
      not_here = true;
    }
    if (!not_here) {
      while (_cur) {
        if (_srcVia == 1 || _cur->isVia == 1) {
          if (!_cur->bounds.overlaps(_search_box)) {
            _cur = _cur->next;
            continue;
          }
        } else {
          if (!_cur->bounds.intersects(_search_box)) {
            _cur = _cur->next;
            continue;
          }
          if (_srcVia == 0
              && (_cur->xMin() == _search_box.xMax()
                  || _search_box.xMin() == _cur->xMax())) {
            if (!_cur->bounds.intersects(_search_box)) {
              _cur = _cur->next;
              continue;
            }
          } else if (_srcVia == 0
                     && (_cur->yMin() == _search_box.yMax()
                         || _search_box.yMin() == _cur->yMax())) {
            if (!_cur->bounds.intersects(_search_box)) {
              _cur = _cur->next;
              continue;
            }
          }
        }
        *id = _cur->id;
        _pcur = _cur;
        _cur = _cur->next;
        return true;
      }
    }
    if (_bin->left) {
      _bin = _bin->left;
      _cur = _bin->shape_list;
      _pcur = nullptr;
    } else {
      while (_bin->parent && _bin == _bin->parent->right) {
        _bin = _bin->parent;
      }
      _bin = _bin->parent;
      if (_bin) {
        _bin = _bin->right;
        _cur = _bin->shape_list;
        _pcur = nullptr;
      }
    }
  }
  return false;
}

static void tcs_lev_init(tcs_lev* bin)
{
  bin->shape_list = nullptr;
  bin->last_shape = nullptr;
  bin->left = nullptr;
  bin->right = nullptr;
  bin->parent = nullptr;
  bin->n = 0;
}

static void tcs_lev_add(tcs_lev* bin, tcs_shape* shape)
{
  if (bin->shape_list == nullptr) {
    bin->shape_list = shape;
    bin->bounds = shape->bounds;
  } else {
    bin->last_shape->next = shape;
    bin->bounds.merge(shape->bounds);
  }
  bin->last_shape = shape;
  bin->n++;
}

static void tcs_lev_add_no_bb(tcs_lev* bin, tcs_shape* shape)
{
  if (bin->shape_list == nullptr) {
    bin->shape_list = shape;
  } else {
    bin->last_shape->next = shape;
  }
  bin->last_shape = shape;
  bin->n++;
}

static void tcs_lev_wrap(tcs_lev* bin)
{
  if (bin->last_shape) {
    bin->last_shape->next = nullptr;
  }
}

void tmg_conn_search::Impl::sort_level(tcs_lev* bin)
{
  if (_levAllN >= 32767) {
    return;
  }
  if (bin->n < sort_threshold) {
    return;
  }
  tcs_lev* left = _levAllV + _levAllN++;
  tcs_lev_init(left);
  left->parent = bin;

  tcs_lev* right = _levAllV + _levAllN++;
  tcs_lev_init(right);
  right->parent = bin;

  tcs_shape* shape = bin->shape_list;
  tcs_lev* par = bin->parent;
  tcs_lev_init(bin);
  bin->parent = par;
  bin->left = left;
  bin->right = right;

  if (bin->bounds.dx() >= bin->bounds.dy()) {
    const int xmid = bin->bounds.xCenter();
    for (; shape; shape = shape->next) {
      if (shape->xMax() < xmid) {
        tcs_lev_add(left, shape);
      } else if (shape->xMin() > xmid) {
        tcs_lev_add(right, shape);
      } else {
        tcs_lev_add_no_bb(bin, shape);
      }
    }
  } else {
    const int ymid = bin->bounds.yCenter();
    for (; shape; shape = shape->next) {
      if (shape->yMax() < ymid) {
        tcs_lev_add(left, shape);
      } else if (shape->yMin() > ymid) {
        tcs_lev_add(right, shape);
      } else {
        tcs_lev_add_no_bb(bin, shape);
      }
    }
  }
  tcs_lev_wrap(bin);
  tcs_lev_wrap(left);
  tcs_lev_wrap(right);
  sort_level(left);
  sort_level(right);
}

void tmg_conn_search::Impl::sort()
{
  _sorted = true;
  for (int j = 0; j < 32; j++) {
    if (_levV[j]->n > sort_threshold) {
      sort_level(_levV[j]);
    }
  }
}

/////////////////////////////////////////////

tmg_conn_search::tmg_conn_search()
{
  impl_ = std::make_unique<Impl>();
}

tmg_conn_search::~tmg_conn_search() = default;

void tmg_conn_search::clear()
{
  impl_->clear();
}

void tmg_conn_search::addShape(int lev, const Rect& bounds, int isVia, int id)
{
  impl_->addShape(lev, bounds, isVia, id);
}

void tmg_conn_search::searchStart(int lev, const Rect& bounds, int isVia)
{
  impl_->searchStart(lev, bounds, isVia);
}

bool tmg_conn_search::searchNext(int* id)
{
  return impl_->searchNext(id);
}

}  // namespace odb
