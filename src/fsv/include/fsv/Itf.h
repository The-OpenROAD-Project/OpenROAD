// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <istream>
#include <string>
#include <vector>

#include "fsv/Stack.h"

namespace fsv {

// Something in the file the reader skipped: a keyword it does not know,
// with the line it was on.
struct ItfWarning
{
  int line;
  std::string message;
};

// Reads a process stack from an ITF (Interconnect Technology Format) file.
//
// Modeled: the header (TECHNOLOGY, GLOBAL_TEMPERATURE, BACKGROUND_ER,
// HALF_NODE_SCALE_FACTOR, REFERENCE_DIRECTION); planar DIELECTRIC layers
// (THICKNESS, ER, MEASURED_FROM) and conformal ones (IS_CONFORMAL with
// SW_T, TW_T and an optional ASSOCIATED_CONDUCTOR); CONDUCTOR layers with
// THICKNESS, WMIN, SMIN, LAYER_TYPE, their etch (ETCH, CAPACITIVE_ONLY_ETCH,
// RESISTIVE_ONLY_ETCH, ETCH_VS_WIDTH_AND_SPACING), sheet resistance (RPSQ,
// RHO, RPSQ_VS_SI_WIDTH, RPSQ_VS_WIDTH_AND_SPACING,
// RHO_VS_WIDTH_AND_SPACING) and temperature coefficients (T0, CRT1, CRT2,
// CRT_VS_SI_WIDTH); VIA layers with FROM and TO, in either order (the
// upper layer is decided by height; SUBSTRATE names the ground plane),
// their resistance (RPV with AREA, RHO, RPV_VS_AREA) and temperature
// coefficients (T0, CRT1, CRT2, CRT_VS_AREA). Commas separate values like
// spaces, and a '$' starts a comment that runs to the end of the line.
//
// Parsed and skipped: device-level modeling (DEVICE_TYPE, gate and
// diffusion keywords, PARALLEL_TO_GATE etch tables, contact and trench
// contact via tables, MULTIGATE), TSV blocks, VARIATION_PARAMETERS and the
// EM_INFORMATION section, and the header's USE_SI_DENSITY and
// DROP_FACTOR_LATERAL_SPACING.
//
// Refused with an error naming the keyword: effects on the geometry or the
// dielectrics the solver does not model (thickness variation with width,
// spacing or density, ILD, air gaps, dielectric fill, permittivity against
// spacing, side tangents, fill emulation, drop factors, multiple-mask etch,
// ETCH_FROM_TOP).
//
// Any other keyword is skipped with a warning: one per statement, so an
// unknown block ({ ... }) gives one warning for the block and none for
// what it holds. The warnings go to `warnings` when the caller gives one.
//
// The solver itself places wires as drawn: the etch, resistance and
// temperature values are carried for the extractor that builds its models
// from the stack, and the solver's own read_itf refuses a stack whose
// etch would change capacitance. The ground plane is placed at the top of
// the lowest dielectric, which is not part of the returned stack.
//
// Throws std::runtime_error on errors.
Stack readItf(std::istream& in, std::vector<ItfWarning>* warnings = nullptr);
Stack readItfFile(const std::string& path,
                  std::vector<ItfWarning>* warnings = nullptr);

}  // namespace fsv
