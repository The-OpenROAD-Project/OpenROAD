// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

namespace fin {
class Finale;
}

namespace ord {

class OpenRoad;

fin::Finale* makeFinale();
void initFinale(OpenRoad* openroad);
void deleteFinale(fin::Finale* finale);

}  // namespace ord
