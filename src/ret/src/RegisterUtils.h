// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "odb/db.h"

namespace sta {
class dbSta;
}

namespace ret {

// Register classification shared by the graph builder, the search and the
// apply step, so the three can never disagree about which registers move.

// True for a register retiming must never move: scan cells, latches, cells
// without a liberty view or with more than one sequential element, and
// asynchronous set/reset registers whose controls are not tied inactive.
bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst);

// True when this input pin of a sequential cell is in the support of its
// next_state function (clock, async controls and scan pins are not).
bool isRegisterDataPin(sta::dbSta* sta, odb::dbITerm* pin);

// The register's data input pin, or nullptr.
odb::dbITerm* registerDataPin(sta::dbSta* sta, odb::dbInst* inst);

}  // namespace ret
