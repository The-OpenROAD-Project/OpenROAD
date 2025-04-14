// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace rmp {
class Restructure;
}

namespace ord {

class OpenRoad;

rmp::Restructure* makeRestructure();

void initRestructure(OpenRoad* openroad);

void deleteRestructure(rmp::Restructure* restructure);

}  // namespace ord
