// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include "boost/polygon/polygon.hpp"

namespace fin {

using boost::polygon::operators::operator-=;
using boost::polygon::operators::operator+=;
using boost::polygon::operators::operator&;
using boost::polygon::operators::operator+;
using boost::polygon::operators::operator-;

using Rectangle = boost::polygon::rectangle_data<int>;
using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
using Polygon90Set = boost::polygon::polygon_90_set_data<int>;

};  // namespace fin
