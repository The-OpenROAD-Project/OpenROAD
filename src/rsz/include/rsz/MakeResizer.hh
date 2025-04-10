// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace sta {
class dbSta;
}

namespace rsz {
class Resizer;
}

namespace ord {

class OpenRoad;

rsz::Resizer* makeResizer();

void deleteResizer(rsz::Resizer* resizer);

void initResizer(OpenRoad* openroad);

}  // namespace ord
