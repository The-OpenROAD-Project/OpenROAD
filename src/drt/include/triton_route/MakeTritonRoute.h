// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

namespace drt {
class TritonRoute;
}

namespace ord {

class OpenRoad;

drt::TritonRoute* makeTritonRoute();

void deleteTritonRoute(drt::TritonRoute* router);

void initTritonRoute(OpenRoad* openroad);

}  // namespace ord
