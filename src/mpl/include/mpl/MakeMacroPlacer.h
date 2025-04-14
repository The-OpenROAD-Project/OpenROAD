// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

namespace mpl {
class MacroPlacer;
}

namespace ord {

class OpenRoad;

mpl::MacroPlacer* makeMacroPlacer();

void initMacroPlacer(OpenRoad* openroad);

void deleteMacroPlacer(mpl::MacroPlacer* macro_placer);

}  // namespace ord
