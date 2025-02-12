#pragma once

#include "Kokkos_Core.hpp"

KOKKOS_INLINE_FUNCTION bool isPowerOf2(int val)
{
  return val && (val & (val - 1)) == 0;
}

KOKKOS_INLINE_FUNCTION int INDEX(const int hid, const int wid, const int N)
{
  return (hid * N + wid);
}

KOKKOS_INLINE_FUNCTION Kokkos::complex<float> complexMul(const Kokkos::complex<float>& x,
                                          const Kokkos::complex<float>& y)
{
  Kokkos::complex<float> res;
  res.real() = x.real() * y.real() - x.imag() * y.imag();
  res.imag() = x.real() * y.imag() + x.imag() * y.real();
  return res;
}

KOKKOS_INLINE_FUNCTION float RealPartOfMul(const Kokkos::complex<float>& x,
                                          const Kokkos::complex<float>& y)
{
  return x.real() * y.real() - x.imag() * y.imag();
}

KOKKOS_INLINE_FUNCTION float ImaginaryPartOfMul(const Kokkos::complex<float>& x,
                                               const Kokkos::complex<float>& y)
{
  return x.real() * y.imag() + x.imag() * y.real();
}

KOKKOS_INLINE_FUNCTION Kokkos::complex<float> complexAdd(const Kokkos::complex<float>& x,
                                          const Kokkos::complex<float>& y)
{
  Kokkos::complex<float> res;
  res.real() = x.real() + y.real();
  res.imag() = x.imag() + y.imag();
  return res;
}

KOKKOS_INLINE_FUNCTION Kokkos::complex<float> complexSubtract(const Kokkos::complex<float>& x,
                                               const Kokkos::complex<float>& y)
{
  Kokkos::complex<float> res;
  res.real() = x.real() - y.real();
  res.imag() = x.imag() - y.imag();
  return res;
}

KOKKOS_INLINE_FUNCTION Kokkos::complex<float> complexConj(const Kokkos::complex<float>& x)
{
  Kokkos::complex<float> res;
  res.real() = x.real();
  res.imag() = -x.imag();
  return res;
}

KOKKOS_INLINE_FUNCTION Kokkos::complex<float> complexMulConj(const Kokkos::complex<float>& x,
                                              const Kokkos::complex<float>& y)
{
  Kokkos::complex<float> res;
  res.real() = x.real() * y.real() - x.imag() * y.imag();
  res.imag() = -(x.real() * y.imag() + x.imag() * y.real());
  return res;
}

// Device and host may use different implementations of math functions giving different results which is not desirable in OpenROAD
// The consistent* functions are meant to fix that.
KOKKOS_INLINE_FUNCTION float consistentSinf(float x) {
  return sin((double) x);
}

KOKKOS_INLINE_FUNCTION float consistentCosf(float x) {
  return cos((double) x);
}

KOKKOS_INLINE_FUNCTION float consistentExpf(float x) {
  return exp((double) x);
}
