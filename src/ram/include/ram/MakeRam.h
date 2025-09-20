// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

namespace ram {
class RamGen;
}

namespace ord {

class OpenRoad;

ram::RamGen* makeRamGen();
void initRamGen(OpenRoad* openroad);
void deleteRamGen(ram::RamGen* ram);

}  // namespace ord
