// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

// The density force is calculated by solving the Poisson equation.
// Derived from UCSD's DG-RePlAce (github.com/ABKGroup/DG-RePlAce,
// BSD-3-Clause, Copyright (c) The Regents of the University of California),
// via the Kokkos conversion in PR #5352. The Poisson solver itself was
// originally written by Jaekyung Im (jkim97@postech.ac.kr, POSTECH); we
// thank him for his contribution.

#pragma once

#include <Kokkos_Core.hpp>

namespace gpl {

void dct_2d_fft(int M,
                int N,
                const Kokkos::View<const Kokkos::complex<float>*>& expkM,
                const Kokkos::View<const Kokkos::complex<float>*>& expkN,
                const Kokkos::View<const float*>& input,
                const Kokkos::View<float*>& pre,
                const Kokkos::View<Kokkos::complex<float>*>& fft,
                const Kokkos::View<float*>& post);

void idct_2d_fft(int M,
                 int N,
                 const Kokkos::View<const Kokkos::complex<float>*>& expkM,
                 const Kokkos::View<const Kokkos::complex<float>*>& expkN,
                 const Kokkos::View<const Kokkos::complex<float>*>& expkMN1,
                 const Kokkos::View<const Kokkos::complex<float>*>& expkMN2,
                 const Kokkos::View<const float*>& input,
                 const Kokkos::View<Kokkos::complex<float>*>& pre,
                 const Kokkos::View<float*>& ifft,
                 const Kokkos::View<float*>& post);

void idxst_idct(int M,
                int N,
                const Kokkos::View<const Kokkos::complex<float>*>& expkM,
                const Kokkos::View<const Kokkos::complex<float>*>& expkN,
                const Kokkos::View<const Kokkos::complex<float>*>& expkMN1,
                const Kokkos::View<const Kokkos::complex<float>*>& expkMN2,
                const Kokkos::View<const float*>& input,
                const Kokkos::View<float*>& workSpaceReal1,
                const Kokkos::View<Kokkos::complex<float>*>& workSpaceComplex,
                const Kokkos::View<float*>& workSpaceReal2,
                const Kokkos::View<float*>& workSpaceReal3,
                const Kokkos::View<float*>& output);

void idct_idxst(int M,
                int N,
                const Kokkos::View<const Kokkos::complex<float>*>& expkM,
                const Kokkos::View<const Kokkos::complex<float>*>& expkN,
                const Kokkos::View<const Kokkos::complex<float>*>& expkMN1,
                const Kokkos::View<const Kokkos::complex<float>*>& expkMN2,
                const Kokkos::View<const float*>& input,
                const Kokkos::View<float*>& workSpaceReal1,
                const Kokkos::View<Kokkos::complex<float>*>& workSpaceComplex,
                const Kokkos::View<float*>& workSpaceReal2,
                const Kokkos::View<float*>& workSpaceReal3,
                const Kokkos::View<float*>& output);

}  // namespace gpl
