// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace ppl {
class IOPlacer;
}

namespace ord {

class OpenRoad;

ppl::IOPlacer* makeIoplacer();

void initIoplacer(OpenRoad* openroad);

void deleteIoplacer(ppl::IOPlacer* ioplacer);

}  // namespace ord
