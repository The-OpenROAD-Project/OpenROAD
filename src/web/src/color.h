// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>

namespace web {

struct Color
{
  unsigned char r = 0;  // Red (0-255)
  unsigned char g = 0;  // Green (0-255)
  unsigned char b = 0;  // Blue (0-255)
  unsigned char a = 0;  // Alpha (0-255)

  Color lighter(double factor = 1.5) const;
  Color darken(double factor = 0.5) const;
};

// Per-layer fill pattern, mirroring gui::DisplayControls' kBrushPatterns so the
// Qt GUI and the web viewer share one pattern index over the wire and in
// persisted state.  Order MUST match the GUI's brush list:
// {NoBrush, Solid} {Hor, Ver} {Cross, DiagCross} {FDiag, BDiag}.
enum class FillPattern : uint8_t
{
  kNone = 0,    // outline only (Qt::NoBrush)
  kSolid,       // Qt::SolidPattern
  kHorizontal,  // Qt::HorPattern
  kVertical,    // Qt::VerPattern
  kCross,       // Qt::CrossPattern
  kDiagCross,   // Qt::DiagCrossPattern
  kFDiag,       // Qt::FDiagPattern
  kBDiag,       // Qt::BDiagPattern
};

}  // namespace web
