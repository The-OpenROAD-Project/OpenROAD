// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// FftBackend — the Strategy interface for the FFT / Poisson density solve.
// CpuFftBackend (the Ooura DCT) is always available; GpuFftBackend (a Kokkos
// Poisson solver) is added on an ENABLE_GPU build. makeFftBackend() picks one
// per process at run time (gpl::gpuEnabled()).
//
// This header is plain C++ — no Kokkos, no preprocessor branches — so fft.h
// can hold a std::unique_ptr<FftBackend> member without learning anything
// about the GPU build.

#pragma once

#include <memory>
#include <type_traits>

namespace gpl {

// POD view over a 2D bin grid laid out as a single row-major float buffer
// (size = bin_cnt_x * bin_cnt_y, fast axis = y). Backends and the FFT
// context share storage through this struct so the solve() signature carries
// the grid dimensions and addressing convention is unambiguous.
//
// Trivially copyable; copying just duplicates the pointer (non-owning).
struct BinGridSpan
{
  float* data = nullptr;
  int bin_cnt_x = 0;
  int bin_cnt_y = 0;

  float& operator()(int x, int y) { return data[x * bin_cnt_y + y]; }
  float operator()(int x, int y) const { return data[x * bin_cnt_y + y]; }
};

// Strategy: solves the Poisson equation on a density grid. The grids are owned
// by the FFT context and passed in by span — the backends share gpl's data
// and duplicate no storage. solve() reads `density` and writes `phi`,
// `field_x`, `field_y`. All four spans share the same bin_cnt_x / bin_cnt_y.
class FftBackend
{
 public:
  virtual ~FftBackend() = default;
  FftBackend(const FftBackend&) = delete;
  FftBackend& operator=(const FftBackend&) = delete;
  FftBackend(FftBackend&&) = delete;
  FftBackend& operator=(FftBackend&&) = delete;

  virtual void solve(BinGridSpan density,
                     BinGridSpan phi,
                     BinGridSpan field_x,
                     BinGridSpan field_y)
      = 0;

  // Short label for diagnostic logging; constructed-once factory choice.
  virtual const char* name() const = 0;

 protected:
  FftBackend() = default;
};

struct BackendContext;

// Factory: returns GpuFftBackend on an ENABLE_GPU build with the GPU path
// selected at run time, otherwise CpuFftBackend. Consumes ctx.bin_cnt_x /
// bin_cnt_y / bin_size_x / bin_size_y (grid geometry) and ctx.region_field
// (GPU path; may be null — GpuFftBackend borrows the region's bin Views when
// available, falling back to self-owned Views).
std::unique_ptr<FftBackend> makeFftBackend(const BackendContext& ctx);

static_assert(!std::is_copy_constructible_v<FftBackend>);
static_assert(!std::is_move_constructible_v<FftBackend>);

}  // namespace gpl
