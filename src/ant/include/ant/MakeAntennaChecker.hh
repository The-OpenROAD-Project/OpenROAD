// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

namespace ant {
class AntennaChecker;
}

namespace ord {

class OpenRoad;

ant::AntennaChecker* makeAntennaChecker();

void deleteAntennaChecker(ant::AntennaChecker* antennachecker);

void initAntennaChecker(OpenRoad* openroad);

}  // namespace ord
