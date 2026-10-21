// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <istream>
#include <string>

#include "fsv/Stack.h"

namespace fsv {

// Reads a process stack from an ITF (Interconnect Technology Format) file.
//
// Supported: planar DIELECTRIC layers (THICKNESS, ER, MEASURED_FROM),
// CONDUCTOR layers (THICKNESS, WMIN, SMIN), conformal dielectrics
// (IS_CONFORMAL with SW_T, TW_T and an optional ASSOCIATED_CONDUCTOR), VIA
// FROM/TO, TECHNOLOGY and BACKGROUND_ER. A '$' starts a comment that runs to
// the end of the line. FROM and TO may be given in either order; a via's
// upper layer is decided by height, and SUBSTRATE names the ground plane.
// Resistance and temperature keywords are accepted and ignored. Any other
// keyword, and any option that would change capacitance but is not modeled
// (such as a non-zero CAPACITIVE_ONLY_ETCH), is an error naming the keyword and
// line.
//
// The ground plane is placed at the top of the lowest dielectric, which is
// not part of the returned stack.
//
// Throws std::runtime_error on errors.
Stack readItf(std::istream& in);
Stack readItfFile(const std::string& path);

}  // namespace fsv
