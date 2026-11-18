// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

namespace fsv {

// The one-dimensional modes behind the two-dielectric tables of the cube
// and of the square: the cell [-1, 1] along the interface normal z is split
// at z = t into e_lo below and e_hi above, with the potential and e * dZ/dz
// continuous at t.

// Tabulated interface positions per half cell: t = k / kInterfacePositions,
// |k| < kInterfacePositions.
inline constexpr int kInterfacePositions = 8;

// Unit data on the bottom face z = -1 in one Fourier mode along the face
// with wavenumber k. With a = 1 + t and b = 1 - t, the z profile is
//   lower: (sinh(k (a - (z + 1))) + c sinh(k (z + 1))) / sinh(k a)
//   upper: c sinh(k (1 - z)) / sinh(k b)
// where c, its value at the interface, follows from the flux condition.
struct ZProfile
{
  double value;  // Z(0)
  double flux;   // e(0) * Z'(0), continuous across the interface
};

ZProfile bottomCenter(double k, double t, double e_lo, double e_hi);

// One eigenfunction of -(e Z')' = lambda e Z on [-1, 1] with Z(+-1) = 0 and
// the interface at t: Z = A sin(q (z + 1)) below, B sin(q (1 - z)) above.
struct ZMode
{
  double q;
  double lower;  // A
  double upper;  // B
};

// The modes for an interface at t = position / kInterfacePositions, in
// increasing q, up to the largest root whose weight is above double
// precision.
std::vector<ZMode> zModes(int position, double e_lo, double e_hi);

// What a side face needs of one mode: its e-weighted norm, its value and
// flux e * Z' at the center, and its e-weighted integral over each of n
// equal panels of [-1, 1] (the interface lies on a panel boundary).
struct ModeTerms
{
  double norm;
  double value;
  double flux;
  std::vector<double> panel;
};

ModeTerms modeTerms(const ZMode& mode,
                    double t,
                    int n,
                    double e_lo,
                    double e_hi);

}  // namespace fsv
