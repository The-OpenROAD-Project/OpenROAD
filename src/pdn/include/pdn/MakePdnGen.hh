// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace ord {
class OpenRoad;
}

namespace pdn {
class PdnGen;
}

namespace ord {

void initPdnGen(OpenRoad* openroad);

pdn::PdnGen* makePdnGen();

void deletePdnGen(pdn::PdnGen* pdngen);

}  // namespace ord
