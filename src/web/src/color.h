// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

namespace web {

struct Color
{
  unsigned char r;  // Red (0-255)
  unsigned char g;  // Green (0-255)
  unsigned char b;  // Blue (0-255)
  unsigned char a;  // Alpha (0-255)

  Color lighter(double factor = 1.5) const;
  Color darken(double factor = 0.5) const;
};

}  // namespace web
