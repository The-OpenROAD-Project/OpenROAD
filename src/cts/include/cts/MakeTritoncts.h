// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace cts {
class TritonCTS;
}

namespace ord {

class OpenRoad;

cts::TritonCTS* makeTritonCts();

void initTritonCts(OpenRoad* openroad);

void deleteTritonCts(cts::TritonCTS* tritoncts);

}  // namespace ord
