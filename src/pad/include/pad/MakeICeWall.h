// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include "pad/ICeWall.h"

namespace ord {

class OpenRoad;

void initICeWall(OpenRoad* openroad);

pad::ICeWall* makeICeWall();

void deleteICeWall(pad::ICeWall* icewall);

}  // namespace ord
