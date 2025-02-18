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

#ifdef KOKKOS_ENABLE_CUDA
  #define HOST_FUNCTION __host__
#else
  #define HOST_FUNCTION KOKKOS_FUNCTION
#endif

#ifdef KOKKOS_ENABLE_CUDA
  #define HOST_INLINE_FUNCTION inline __host__
#else
  #define HOST_INLINE_FUNCTION KOKKOS_INLINE_FUNCTION
#endif

// We can't use parallel_reduce as we would lose consisiency between platforms
// In order to ensure consistency with as low performance penalty as possible, we do it with host-only functions
// that are autovectorizable by compiler.
HOST_INLINE_FUNCTION float sumFloats(const Kokkos::View<const float*> arr, size_t size) {
  float partialSums[4] = {0.0, 0.0, 0.0, 0.0};
  auto hArr = Kokkos::create_mirror_view_and_copy(Kokkos::DefaultHostExecutionSpace(), arr);
  for(int i = 0; i<size/4*4; i+=4) {
    partialSums[0] += hArr[i+0];
    partialSums[1] += hArr[i+1];
    partialSums[2] += hArr[i+2];
    partialSums[3] += hArr[i+3];
  }
  float leftover = 0.0;
  for(int i = size/4*4; i<size; ++i) {
    leftover += hArr[i];
  }
  return partialSums[0] + partialSums[1] + partialSums[2] + partialSums[3] + leftover;
}

// More accurate version of sumFloats() that use double as accumulator. TODO: Consider using Kahan summation algorithm
HOST_INLINE_FUNCTION float sumFloatsAccurate(const Kokkos::View<const float*> arr, size_t size) {
  auto hArr = Kokkos::create_mirror_view_and_copy(Kokkos::DefaultHostExecutionSpace(), arr);
  double partialSums[4] = {0.0, 0.0, 0.0, 0.0};
  for(int i = 0; i<size/4*4; i+=4) {
    partialSums[0] += hArr[i+0];
    partialSums[1] += hArr[i+1];
    partialSums[2] += hArr[i+2];
    partialSums[3] += hArr[i+3];
  }
  double leftover = 0.0;
  for(int i = size/4*4; i<size; ++i) {
    leftover += hArr[i];
  }
  return partialSums[0] + partialSums[1] + partialSums[2] + partialSums[3] + leftover;
}
