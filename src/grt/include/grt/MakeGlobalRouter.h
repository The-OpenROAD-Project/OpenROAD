// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace grt {
class GlobalRouter;
}

namespace ord {

class OpenRoad;

grt::GlobalRouter* makeGlobalRouter();

void initGlobalRouter(OpenRoad* openroad);

void deleteGlobalRouter(grt::GlobalRouter* global_router);

}  // namespace ord
