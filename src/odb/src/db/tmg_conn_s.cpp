// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <memory>

#include "odb/db.h"
#include "tmg_conn.h"

namespace odb {

struct tcs_shape
{
  int xMin() const { return bounds.xMin(); }
  int yMin() const { return bounds.yMin(); }
  int xMax() const { return bounds.xMax(); }
  int yMax() const { return bounds.yMax(); }

  tcs_shape* next = nullptr;
  Rect bounds;
  int level = 0;
  int is_via = 0;
  int id = 0;
};

struct tcs_level
{
  int xMin() const { return bounds.xMin(); }
  int yMin() const { return bounds.yMin(); }
  int xMax() const { return bounds.xMax(); }
  int yMax() const { return bounds.yMax(); }
  void reset();
  void add_shape(tcs_shape* shape, bool update_bounds = true);

  tcs_shape* shape_list = nullptr;
  tcs_shape* last_shape = nullptr;
  tcs_level* left = nullptr;
  tcs_level* right = nullptr;
  tcs_level* parent = nullptr;
  Rect bounds;
  int num_shapes = 0;
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

  // Use deque so that emplace_back doesn't move prior elements so pointer
  // into these structures are safe.
  std::deque<tcs_shape> _shapes;
  std::deque<tcs_level> _levels;
  std::array<tcs_level*, 32> _root_for_level;

  // Used during searching
  Rect _search_box;
  int _search_via{0};
  tcs_level* _search_bin{nullptr};
  tcs_shape* _search_shape{nullptr};

  // Sorting happens after all the shapes have been added and the
  // first searchStart happens
  bool _sorted{false};

  static constexpr int sort_threshold = 1024;
};

tmg_conn_search::Impl::Impl()
{
  clear();
}

void tmg_conn_search::Impl::clear()
{
  _shapes.clear();
  _levels.clear();
  for (tcs_level*& level : _root_for_level) {
    level = &_levels.emplace_back();
    level->reset();
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
  tcs_level* slev = _root_for_level.at(level);
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
  _search_bin = _root_for_level.at(level);
  _search_shape = _search_bin->shape_list;
  _search_box = bounds;
  _search_via = is_via;
}

bool tmg_conn_search::Impl::searchNext(int* id)
{
  *id = -1;
  if (!_search_bin) {
    return false;
  }
  // this is for speed for ordinary small nets
  if (_search_via == 1 && !_search_bin->parent && !_search_bin->left
      && !_search_bin->right) {
    while (_search_shape) {
      if (_search_shape->bounds.overlaps(_search_box)) {
        *id = _search_shape->id;
        _search_shape = _search_shape->next;
        return true;
      }
      _search_shape = _search_shape->next;
    }
    return false;
  }

  while (_search_bin) {
    if (_search_bin->bounds.intersects(_search_box)) {
      while (_search_shape) {
        if (_search_via == 1 || _search_shape->is_via == 1) {
          if (!_search_shape->bounds.overlaps(_search_box)) {
            _search_shape = _search_shape->next;
            continue;
          }
        } else {
          if (!_search_shape->bounds.intersects(_search_box)) {
            _search_shape = _search_shape->next;
            continue;
          }
          // Skip wire segments that are abutting but staggered, eg
          //        |-------
          //   -----|
          //        |-------
          //   ------
          if (_search_via == 0
              && (_search_shape->xMin() == _search_box.xMax()
                  || _search_box.xMin() == _search_shape->xMax())) {
            if ((_search_shape->yMax() < _search_box.yMax()
                 && _search_shape->yMin() < _search_box.yMin())
                || (_search_shape->yMax() > _search_box.yMax()
                    && _search_shape->yMin() > _search_box.yMin())) {
              _search_shape = _search_shape->next;
              continue;
            }
          } else if (_search_via == 0
                     && (_search_shape->yMin() == _search_box.yMax()
                         || _search_box.yMin() == _search_shape->yMax())) {
            if ((_search_shape->xMax() < _search_box.xMax()
                 && _search_shape->xMin() < _search_box.xMin())
                || (_search_shape->xMax() > _search_box.xMax()
                    && _search_shape->xMin() > _search_box.xMin())) {
              _search_shape = _search_shape->next;
              continue;
            }
          }
        }
        *id = _search_shape->id;
        _search_shape = _search_shape->next;
        return true;
      }
    }
    if (_search_bin->left) {
      _search_bin = _search_bin->left;
      _search_shape = _search_bin->shape_list;
    } else {
      while (_search_bin->parent && _search_bin == _search_bin->parent->right) {
        _search_bin = _search_bin->parent;
      }
      _search_bin = _search_bin->parent;
      if (_search_bin) {
        _search_bin = _search_bin->right;
        _search_shape = _search_bin->shape_list;
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
  if (bin->num_shapes < sort_threshold) {
    return;
  }
  tcs_level* left = &_levels.emplace_back();
  tcs_level_init(left, bin);  // NOLINT(readability-suspicious-call-argument)

  tcs_level* right = &_levels.emplace_back();
  tcs_level_init(right, bin);  // NOLINT(readability-suspicious-call-argument)

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
  for (tcs_level* level : _root_for_level) {
    sort_level(level);
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
