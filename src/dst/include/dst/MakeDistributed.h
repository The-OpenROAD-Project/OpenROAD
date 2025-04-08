// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

namespace dst {
class Distributed;
}

namespace ord {

class OpenRoad;

dst::Distributed* makeDistributed();

void deleteDistributed(dst::Distributed* dstr);

void initDistributed(OpenRoad* openroad);

}  // namespace ord
