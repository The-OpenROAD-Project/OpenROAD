// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

namespace psm {
class PDNSim;
}

namespace ord {
class OpenRoad;

psm::PDNSim* makePDNSim();

void initPDNSim(OpenRoad* openroad);

void deletePDNSim(psm::PDNSim* pdnsim);

}  // namespace ord
