// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

namespace dpo {
class Optdp;
}

namespace ord {

class OpenRoad;

dpo::Optdp* makeOptdp();
void initOptdp(OpenRoad* openroad);
void deleteOptdp(dpo::Optdp* opt);

}  // namespace ord
