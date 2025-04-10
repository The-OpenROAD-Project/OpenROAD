// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

namespace ord {

class OpenRoad;

// There is no make/delete GUI as it is created at startup and can't
// be deleted.

void initGui(OpenRoad* openroad);

}  // namespace ord
