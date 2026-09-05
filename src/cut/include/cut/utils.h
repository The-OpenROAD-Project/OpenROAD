// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

namespace sta {
class LibertyCell;
}  // namespace sta

namespace cut {

bool IsCombinational(sta::LibertyCell* cell);

}  // namespace cut
