// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace tap {
class Tapcell;
}

namespace ord {

class OpenRoad;

tap::Tapcell* makeTapcell();

void deleteTapcell(tap::Tapcell* tapcell);

void initTapcell(OpenRoad* openroad);

}  // namespace ord
