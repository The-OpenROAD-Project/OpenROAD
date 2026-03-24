// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <array>
#include <deque>
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
  std::deque<tcs_shape> shapes_;
  std::deque<tcs_level> levels_;
  std::array<tcs_level*, 32> root_for_level_;

  // Used during searching
  Rect search_box_;
  int search_via_{0};
  tcs_level* search_bin_{nullptr};
  tcs_shape* search_shape_{nullptr};

  // Sorting happens after all the shapes have been added and the
  // first searchStart happens
  bool sorted_{false};

  static constexpr int kSortThreshold = 1024;
};

tmg_conn_search::Impl::Impl()
{
  clear();
}

void tmg_conn_search::Impl::clear()
{
  shapes_.clear();
  levels_.clear();
  for (tcs_level*& level : root_for_level_) {
    level = &levels_.emplace_back();
    level->reset();
  }
  sorted_ = false;
}

void tmg_conn_search::Impl::addShape(const int level,
                                     const Rect& bounds,
                                     const int is_via,
                                     const int id)
{
  tcs_shape* shape = &shapes_.emplace_back();
  shape->level = level;
  shape->bounds = bounds;
  shape->is_via = is_via;
  shape->id = id;
  shape->next = nullptr;
  tcs_level* slev = root_for_level_.at(level);
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
  if (!sorted_) {
    sort();
  }
  search_bin_ = root_for_level_.at(level);
  search_shape_ = search_bin_->shape_list;
  search_box_ = bounds;
  search_via_ = is_via;
}

bool tmg_conn_search::Impl::searchNext(int* id)
{
  *id = -1;
  if (!search_bin_) {
    return false;
  }
  // this is for speed for ordinary small nets
  if (search_via_ == 1 && !search_bin_->parent && !search_bin_->left
      && !search_bin_->right) {
    while (search_shape_) {
      if (search_shape_->bounds.overlaps(search_box_)) {
        *id = search_shape_->id;
        search_shape_ = search_shape_->next;
        return true;
      }
      search_shape_ = search_shape_->next;
    }
    return false;
  }

  while (search_bin_) {
    if (search_bin_->bounds.intersects(search_box_)) {
      while (search_shape_) {
        if (search_via_ == 1 || search_shape_->is_via == 1) {
          if (!search_shape_->bounds.overlaps(search_box_)) {
            search_shape_ = search_shape_->next;
            continue;
          }
        } else {
          if (!search_shape_->bounds.intersects(search_box_)) {
            search_shape_ = search_shape_->next;
            continue;
          }
          // Skip wire segments that are abutting but staggered, eg
          //        |-------
          //   -----|
          //        |-------
          //   ------
          if (search_via_ == 0
              && (search_shape_->xMin() == search_box_.xMax()
                  || search_box_.xMin() == search_shape_->xMax())) {
            if ((search_shape_->yMax() < search_box_.yMax()
                 && search_shape_->yMin() < search_box_.yMin())
                || (search_shape_->yMax() > search_box_.yMax()
                    && search_shape_->yMin() > search_box_.yMin())) {
              search_shape_ = search_shape_->next;
              continue;
            }
          } else if (search_via_ == 0
                     && (search_shape_->yMin() == search_box_.yMax()
                         || search_box_.yMin() == search_shape_->yMax())) {
            if ((search_shape_->xMax() < search_box_.xMax()
                 && search_shape_->xMin() < search_box_.xMin())
                || (search_shape_->xMax() > search_box_.xMax()
                    && search_shape_->xMin() > search_box_.xMin())) {
              search_shape_ = search_shape_->next;
              continue;
            }
          }
        }
        *id = search_shape_->id;
        search_shape_ = search_shape_->next;
        return true;
      }
    }
    if (search_bin_->left) {
      search_bin_ = search_bin_->left;
      search_shape_ = search_bin_->shape_list;
    } else {
      while (search_bin_->parent && search_bin_ == search_bin_->parent->right) {
        search_bin_ = search_bin_->parent;
      }
      search_bin_ = search_bin_->parent;
      if (search_bin_) {
        search_bin_ = search_bin_->right;
        search_shape_ = search_bin_->shape_list;
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
  if (bin->num_shapes < kSortThreshold) {
    return;
  }
  tcs_level* left = &levels_.emplace_back();
  tcs_level_init(left, bin);  // NOLINT(readability-suspicious-call-argument)

  tcs_level* right = &levels_.emplace_back();
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
  sorted_ = true;
  for (tcs_level* level : root_for_level_) {
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
