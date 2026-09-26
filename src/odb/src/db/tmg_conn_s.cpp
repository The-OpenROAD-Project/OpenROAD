// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <array>
#include <deque>

#include "shapeSearch.h"

namespace odb {

void ShapeSearch::Bin::init(ShapeSearch::Bin* parent,
                            ShapeSearch::Bin* left,
                            ShapeSearch::Bin* right)
{
  first_shape = nullptr;
  last_shape = nullptr;

  this->left = left;
  this->right = right;
  this->parent = parent;

  num_shapes = 0;
}

void ShapeSearch::Bin::wrap()
{
  if (last_shape) {
    last_shape->next = nullptr;
  }
}

void ShapeSearch::Bin::addShape(ShapeSearch::Shape* shape, bool update_bounds)
{
  if (first_shape == nullptr) {
    first_shape = shape;
    if (update_bounds) {
      bounds = *shape;
    }
  } else {
    last_shape->next = shape;
    if (update_bounds) {
      bounds.merge(*shape);
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

  bins_are_split_ = false;
}

void ShapeSearch::addShape(const int level,
                           const Rect& bounds,
                           const Type type,
                           const int id)
{
  ShapeSearch::Shape* shape = &shapes_.emplace_back(bounds, type, id);
  ShapeSearch::Bin* level_root_bin = root_for_level_.at(level);
  level_root_bin->addShape(shape);
}

void ShapeSearch::searchStart(const int level,
                              const Rect& bounds,
                              const Type type)
{
  if (!bins_are_split_) {
    splitBins();
  }
  search_bin_ = root_for_level_.at(level);
  search_shape_ = search_bin_->first_shape;
  search_box_ = bounds;
  search_type_ = type;
}

bool ShapeSearch::searchNext(int* id)
{
  *id = -1;
  if (!search_bin_) {
    return false;
  }
  // this is for speed for ordinary small nets
  if (search_type_ == Type::kVia && !search_bin_->parent && !search_bin_->left
      && !search_bin_->right) {
    while (search_shape_) {
      if (search_shape_->overlaps(search_box_)) {
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
        if (search_type_ == Type::kVia || search_shape_->type == Type::kVia) {
          if (!search_shape_->overlaps(search_box_)) {
            search_shape_ = search_shape_->next;
            continue;
          }
        } else {
          if (!search_shape_->intersects(search_box_)) {
            search_shape_ = search_shape_->next;
            continue;
          }
          // Skip wire segments that are abutting but staggered, eg
          //        |-------
          //   -----|
          //        |-------
          //   ------
          if (search_type_ == Type::kWire
              && (search_shape_->xMin() == search_box_.xMax()
                  || search_box_.xMin() == search_shape_->xMax())) {
            if ((search_shape_->yMax() < search_box_.yMax()
                 && search_shape_->yMin() < search_box_.yMin())
                || (search_shape_->yMax() > search_box_.yMax()
                    && search_shape_->yMin() > search_box_.yMin())) {
              search_shape_ = search_shape_->next;
              continue;
            }
          } else if (search_type_ == Type::kWire
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
      search_shape_ = search_bin_->first_shape;
    } else {
      while (search_bin_->parent && search_bin_ == search_bin_->parent->right) {
        search_bin_ = search_bin_->parent;
      }
      search_bin_ = search_bin_->parent;
      if (search_bin_) {
        search_bin_ = search_bin_->right;
        search_shape_ = search_bin_->first_shape;
      }
    }
  }
  return false;
}

void ShapeSearch::splitBin(ShapeSearch::Bin* bin)
{
  if (bin->num_shapes < kSortThreshold) {
    return;
  }
  ShapeSearch::Bin* left = &bins_.emplace_back();
  left->init(bin);

  ShapeSearch::Bin* right = &bins_.emplace_back();
  right->init(bin);

  ShapeSearch::Shape* shape = bin->first_shape;
  bin->init(bin->parent, left, right);

  if (bin->bounds.dx() >= bin->bounds.dy()) {
    const int xmid = bin->bounds.xCenter();
    for (; shape; shape = shape->next) {
      if (shape->xMax() < xmid) {
        left->addShape(shape);
      } else if (shape->xMin() > xmid) {
        right->addShape(shape);
      } else {
        bin->addShape(shape, /* update_bounds */ false);
      }
    }
  } else {
    const int ymid = bin->bounds.yCenter();
    for (; shape; shape = shape->next) {
      if (shape->yMax() < ymid) {
        left->addShape(shape);
      } else if (shape->yMin() > ymid) {
        right->addShape(shape);
      } else {
        bin->addShape(shape, /* update_bounds */ false);
      }
    }
  }
  bin->wrap();
  left->wrap();
  right->wrap();

  splitBin(left);
  splitBin(right);
}

void ShapeSearch::splitBins()
{
  bins_are_split_ = true;
  for (ShapeSearch::Bin* root_bin : root_for_level_) {
    splitBin(root_bin);
  }
}

}  // namespace odb
