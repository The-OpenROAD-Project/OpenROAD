// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <memory>

#include "odb/db.h"
#include "tmg_conn.h"

namespace odb {

struct tcs_shape
{
  tcs_shape* next = nullptr;
  Rect bounds;
  int level = 0;
  int is_via = 0;
  int id = 0;

  int xMin() const { return bounds.xMin(); }
  int yMin() const { return bounds.yMin(); }
  int xMax() const { return bounds.xMax(); }
  int yMax() const { return bounds.yMax(); }
};

struct tcs_level
{
  tcs_shape* shape_list = nullptr;
  tcs_shape* last_shape = nullptr;
  tcs_level* left = nullptr;
  tcs_level* right = nullptr;
  tcs_level* parent = nullptr;
  Rect bounds;
  int num_shapes = 0;

  int xMin() const { return bounds.xMin(); }
  int yMin() const { return bounds.yMin(); }
  int xMax() const { return bounds.xMax(); }
  int yMax() const { return bounds.yMax(); }
  void reset();
  void add_shape(tcs_shape* shape, bool update_bounds = true);
};

void tcs_level::reset()
{
  shape_list = nullptr;
  last_shape = nullptr;
  left = nullptr;
  right = nullptr;
  parent = nullptr;
  bounds.reset(0, 0, 0, 0);
  num_shapes = 0;
}

void tcs_level::add_shape(tcs_shape* shape, bool update_bounds)
{
  if (shape_list == nullptr) {
    shape_list = shape;
    if (update_bounds) {
      bounds = shape->bounds;
    }
  } else {
    last_shape->next = shape;
    if (update_bounds) {
      bounds.merge(shape->bounds);
    }
  }
  last_shape = shape;
  num_shapes++;
}

class tmg_conn_search::Impl
{
 public:
  Impl();
  void clear();
  void addShape(int level, const Rect& bounds, int is_via, int id);
  void searchStart(int level, const Rect& bounds, int is_via);
  bool searchNext(int* id);

 private:
  void sort();
  void sort_level(tcs_level* bin);

  std::deque<tcs_shape> _shapes;
  tcs_level _levAllV[32768];
  int _levAllN;
  std::array<tcs_level*, 32> _levV;
  Rect _search_box;
  int _src_via;
  tcs_level* _bin;
  tcs_shape* _cur;
  bool _sorted;

  static constexpr int sort_threshold = 1024;
};

tmg_conn_search::Impl::Impl()
{
  _src_via = 0;
  _bin = nullptr;
  _cur = nullptr;
  clear();
}

void tmg_conn_search::Impl::clear()
{
  _levAllN = 0;
  for (int j = 0; j < _levV.size(); j++) {
    _levV[j] = _levAllV + _levAllN++;
    _levV[j]->reset();
  }
  _sorted = false;
}

void tmg_conn_search::Impl::addShape(const int level,
                                     const Rect& bounds,
                                     const int is_via,
                                     const int id)
{
  tcs_shape* shape = &_shapes.emplace_back();
  shape->level = level;
  shape->bounds = bounds;
  shape->is_via = is_via;
  shape->id = id;
  shape->next = nullptr;
  tcs_level* slev = _levV.at(level);
  if (slev->shape_list == nullptr) {
    slev->shape_list = shape;
    slev->bounds = shape->bounds;
  } else {
    slev->last_shape->next = shape;
    slev->bounds.merge(shape->bounds);
  }
  slev->last_shape = shape;
  slev->num_shapes++;
}

void tmg_conn_search::Impl::searchStart(const int level,
                                        const Rect& bounds,
                                        const int is_via)
{
  if (!_sorted) {
    sort();
  }
  _bin = _levV.at(level);
  _cur = _bin->shape_list;
  _search_box = bounds;
  _src_via = is_via;
}

// _src_via = 0 ==> wire
//          = 1 ==> via
//          = 2 ==> pin
bool tmg_conn_search::Impl::searchNext(int* id)
{
  *id = -1;
  if (!_bin) {
    return false;
  }
  // this is for speed for ordinary small nets
  if (_src_via == 1 && !_bin->parent && !_bin->left && !_bin->right) {
    while (_cur) {
      if (_cur->bounds.overlaps(_search_box)) {
        *id = _cur->id;
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
        if (_src_via == 1 || _cur->is_via == 1) {
          if (!_cur->bounds.overlaps(_search_box)) {
            _cur = _cur->next;
            continue;
          }
        } else {
          if (!_cur->bounds.intersects(_search_box)) {
            _cur = _cur->next;
            continue;
          }
          if (_src_via == 0
              && (_cur->xMin() == _search_box.xMax()
                  || _search_box.xMin() == _cur->xMax())) {
            if (!_cur->bounds.intersects(_search_box)) {
              _cur = _cur->next;
              continue;
            }
          } else if (_src_via == 0
                     && (_cur->yMin() == _search_box.yMax()
                         || _search_box.yMin() == _cur->yMax())) {
            if (!_cur->bounds.intersects(_search_box)) {
              _cur = _cur->next;
              continue;
            }
          }
        }
        *id = _cur->id;
        _cur = _cur->next;
        return true;
      }
    }
    if (_bin->left) {
      _bin = _bin->left;
      _cur = _bin->shape_list;
    } else {
      while (_bin->parent && _bin == _bin->parent->right) {
        _bin = _bin->parent;
      }
      _bin = _bin->parent;
      if (_bin) {
        _bin = _bin->right;
        _cur = _bin->shape_list;
      }
    }
  }
  return false;
}

static void tcs_level_init(tcs_level* bin,
                           tcs_level* parent,
                           tcs_level* left = nullptr,
                           tcs_level* right = nullptr)
{
  bin->shape_list = nullptr;
  bin->last_shape = nullptr;
  bin->left = left;
  bin->right = right;
  bin->parent = parent;
  bin->num_shapes = 0;
}

static void tcs_level_wrap(tcs_level* bin)
{
  if (bin->last_shape) {
    bin->last_shape->next = nullptr;
  }
}

void tmg_conn_search::Impl::sort_level(tcs_level* bin)
{
  if (_levAllN >= 32767) {
    return;
  }
  if (bin->num_shapes < sort_threshold) {
    return;
  }
  tcs_level* left = _levAllV + _levAllN++;
  tcs_level_init(left, bin);

  tcs_level* right = _levAllV + _levAllN++;
  tcs_level_init(right, bin);

  tcs_shape* shape = bin->shape_list;
  tcs_level_init(bin, bin->parent, left, right);

  if (bin->bounds.dx() >= bin->bounds.dy()) {
    const int xmid = bin->bounds.xCenter();
    for (; shape; shape = shape->next) {
      if (shape->xMax() < xmid) {
        left->add_shape(shape);
      } else if (shape->xMin() > xmid) {
        right->add_shape(shape);
      } else {
        bin->add_shape(shape, /* update_bounds */ false);
      }
    }
  } else {
    const int ymid = bin->bounds.yCenter();
    for (; shape; shape = shape->next) {
      if (shape->yMax() < ymid) {
        left->add_shape(shape);
      } else if (shape->yMin() > ymid) {
        right->add_shape(shape);
      } else {
        bin->add_shape(shape, /* update_bounds */ false);
      }
    }
  }
  tcs_level_wrap(bin);
  tcs_level_wrap(left);
  tcs_level_wrap(right);
  sort_level(left);
  sort_level(right);
}

void tmg_conn_search::Impl::sort()
{
  _sorted = true;
  for (int j = 0; j < _levV.size(); j++) {
    if (_levV[j]->num_shapes > sort_threshold) {
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

void tmg_conn_search::addShape(int level,
                               const Rect& bounds,
                               int is_via,
                               int id)
{
  impl_->addShape(level, bounds, is_via, id);
}

void tmg_conn_search::searchStart(int level, const Rect& bounds, int is_via)
{
  impl_->searchStart(level, bounds, is_via);
}

bool tmg_conn_search::searchNext(int* id)
{
  return impl_->searchNext(id);
}

}  // namespace odb
