// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <array>
#include <deque>

#include "shapeSearch.h"

namespace odb {

static void tcs_level_init(ShapeSearch::Bin* bin,
                           ShapeSearch::Bin* parent,
                           ShapeSearch::Bin* left = nullptr,
                           ShapeSearch::Bin* right = nullptr)
{
  bin->shape_list = nullptr;
  bin->last_shape = nullptr;
  bin->left = left;
  bin->right = right;
  bin->parent = parent;
  bin->num_shapes = 0;
}

static void tcs_level_wrap(ShapeSearch::Bin* bin)
{
  if (bin->last_shape) {
    bin->last_shape->next = nullptr;
  }
}

//////////////////////////////////////////////////

void ShapeSearch::Bin::add_shape(ShapeSearch::Shape* shape, bool update_bounds)
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

//////////////////////////////////////////////////

ShapeSearch::ShapeSearch()
{
  clear();
}

void ShapeSearch::clear()
{
  shapes_.clear();
  bins_.clear();

  for (ShapeSearch::Bin*& root_bin : root_for_level_) {
    root_bin = &bins_.emplace_back();
  }

  sorted_ = false;
}

void ShapeSearch::addShape(const int level,
                           const Rect& bounds,
                           const int is_via,
                           const int id)
{
  ShapeSearch::Shape* shape = &shapes_.emplace_back();
  shape->level = level;
  shape->bounds = bounds;
  shape->is_via = is_via;
  shape->id = id;
  shape->next = nullptr;
  ShapeSearch::Bin* slev = root_for_level_.at(level);
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

void ShapeSearch::searchStart(const int level,
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

bool ShapeSearch::searchNext(int* id)
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

void ShapeSearch::sort_level(ShapeSearch::Bin* bin)
{
  if (bin->num_shapes < kSortThreshold) {
    return;
  }
  ShapeSearch::Bin* left = &bins_.emplace_back();
  tcs_level_init(left, bin);  // NOLINT(readability-suspicious-call-argument)

  ShapeSearch::Bin* right = &bins_.emplace_back();
  tcs_level_init(right, bin);  // NOLINT(readability-suspicious-call-argument)

  ShapeSearch::Shape* shape = bin->shape_list;
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

void ShapeSearch::sort()
{
  sorted_ = true;
  for (ShapeSearch::Bin* level : root_for_level_) {
    sort_level(level);
  }
}

}  // namespace odb
