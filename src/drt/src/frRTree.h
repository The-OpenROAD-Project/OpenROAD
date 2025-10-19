// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <utility>

#include "boost/geometry/algorithms/covered_by.hpp"
#include "boost/geometry/algorithms/equals.hpp"
#include "boost/geometry/geometries/register/box.hpp"
#include "boost/geometry/geometries/register/point.hpp"
#include "boost/geometry/geometry.hpp"
#include "db/infra/frBox.h"
#include "db/infra/frPoint.h"
#include "frBaseTypes.h"
#include "odb/geom.h"
#include "serialization.h"

namespace bgi = boost::geometry::index;

// Enable Point & odb::Rect to be used with boost geometry
BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(odb::Point,
                                         drt::frCoord,
                                         cs::cartesian,
                                         x,
                                         y,
                                         setX,
                                         setY)

BOOST_GEOMETRY_REGISTER_BOX(odb::Rect, odb::Point, ll(), ur())

namespace drt {

template <typename T, typename Key = odb::Rect>
using RTree = bgi::rtree<std::pair<Key, T>, bgi::quadratic<16>>;

}  // namespace drt
