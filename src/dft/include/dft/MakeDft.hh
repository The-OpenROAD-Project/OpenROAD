// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

namespace ord {
class OpenRoad;
}

namespace dft {
class Dft;

Dft* makeDft();
void initDft(ord::OpenRoad* openroad);
void deleteDft(Dft* dft);

}  // namespace dft
