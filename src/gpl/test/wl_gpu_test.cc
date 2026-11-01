// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// GPU wirelength-kernel correctness test.
//
// Exercises the wlop K1 (per-net bbox) and K3 (per-net WA B/C sums) kernels
// in src/gpl/src/gpu/wirelengthOp.cpp against host references, on a synthetic
// net→pin CSR that includes nets on both sides of kHighFanoutThreshold. The
// point is the high-fanout team-parallel pass: integration tests and the
// regression designs almost exclusively contain small nets, so without this
// test the team path would only ever be exercised by manual benchmark runs
// on designs with clock/reset-class fanout (e.g. large01's 67k-pin net).
//
// Unlike fft_gpu_test (which goes through the Kokkos-free gpl::FFT facade),
// the wlop launchers take a KokkosDeviceState directly, so this TU includes
// Kokkos-laden headers and is compiled with the device language (see
// set_source_files_properties in test/CMakeLists.txt). Building the
// KokkosDeviceState views by hand — rather than through DeviceState — keeps
// the test free of GNet/GPin/odb scaffolding; the view-filling code below
// mirrors DeviceState::buildTopology's CSR layout and its
// d_high_fanout_net_idx selection rule (degree > kHighFanoutThreshold).
//
// Expectations:
//  - K1 bbox: int min/max is associative, so serial and team passes must be
//    bit-identical to the host loop — exact EXPECT_EQ.
//  - K3 B/C: float sums; the team pass reassociates. Compared against a
//    double-precision host reference with a relative tolerance that covers
//    worst-case float error growth of the serial pass on threshold-sized
//    nets (n·eps ≈ 1024 · 6e-8 ≈ 6e-5) with margin.

#include <Kokkos_Core.hpp>
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "src/gpl/src/gpu/deviceState_kokkos.h"
#include "src/gpl/src/gpu/gpuRuntime.h"
#include "src/gpl/src/gpu/wirelengthOp.h"

namespace {

// Deterministic LCG (Numerical Recipes constants) — keeps the topology and
// coordinates identical across runs and platforms without <random>.
class Lcg
{
 public:
  explicit Lcg(uint32_t seed) : state_(seed) {}

  uint32_t next()
  {
    state_ = state_ * 1664525u + 1013904223u;
    return state_;
  }

  // Uniform int in [0, bound).
  int nextInt(int bound) { return static_cast<int>(next() % bound); }

  // Uniform float in (0, 1].
  float nextUnitFloat()
  {
    return (static_cast<float>(next() >> 8) + 1.0f) / 16777216.0f;
  }

 private:
  uint32_t state_;
};

// Die-sized coordinate range in DBU — matches the magnitude the kernels see
// on real designs (large01's die is ~1.9e6 DBU across).
constexpr int kCoordRange = 2'000'000;

struct SyntheticNetlist
{
  // Net degrees straddling the threshold:
  //  - 0: dangling net (GNet::getHpwl() == 0 case)
  //  - 1 / 2 / 7: typical signal nets (serial path)
  //  - kHighFanoutThreshold: largest net still on the serial path
  //  - kHighFanoutThreshold + 1: smallest net on the team path
  //  - 50'000: clock/reset-class net, deep team path
  std::vector<int> net_deg;
  std::vector<int> net_pin_off;  // CSR offsets, size n_nets + 1
  std::vector<int> net_pin_idx;  // CSR data: permuted global pin ids
  std::vector<int> pin_cx;
  std::vector<int> pin_cy;
  std::vector<int> high_fanout_net_idx;

  int numNets() const { return static_cast<int>(net_deg.size()); }
  int numPins() const { return static_cast<int>(pin_cx.size()); }
};

SyntheticNetlist makeNetlist()
{
  SyntheticNetlist nl;
  nl.net_deg = {0,
                1,
                2,
                7,
                gpl::kHighFanoutThreshold,
                gpl::kHighFanoutThreshold + 1,
                50'000};

  nl.net_pin_off.assign(1, 0);
  for (const int deg : nl.net_deg) {
    nl.net_pin_off.push_back(nl.net_pin_off.back() + deg);
  }
  const int n_pins = nl.net_pin_off.back();

  Lcg lcg(0xC0FFEEu);
  nl.pin_cx.resize(n_pins);
  nl.pin_cy.resize(n_pins);
  for (int p = 0; p < n_pins; ++p) {
    nl.pin_cx[p] = lcg.nextInt(kCoordRange);
    nl.pin_cy[p] = lcg.nextInt(kCoordRange);
  }

  // CSR data: a permutation of all pin ids, so net→pin indirection is
  // non-trivial (kernels must follow d_net_pin_idx, not assume identity).
  nl.net_pin_idx.resize(n_pins);
  for (int p = 0; p < n_pins; ++p) {
    nl.net_pin_idx[p] = p;
  }
  for (int p = n_pins - 1; p > 0; --p) {
    std::swap(nl.net_pin_idx[p], nl.net_pin_idx[lcg.nextInt(p + 1)]);
  }

  // Same selection rule as DeviceState::buildTopology.
  for (int n = 0; n < nl.numNets(); ++n) {
    if (nl.net_deg[n] > gpl::kHighFanoutThreshold) {
      nl.high_fanout_net_idx.push_back(n);
    }
  }
  return nl;
}

template <typename T>
Kokkos::View<T*> toDevice(const char* label, const std::vector<T>& host)
{
  Kokkos::View<T*> dev(label, host.size());
  Kokkos::View<const T*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> host_v(
      host.data(), host.size());
  Kokkos::deep_copy(dev, host_v);
  return dev;
}

template <typename T>
std::vector<T> toHost(const Kokkos::View<T*>& dev)
{
  std::vector<T> host(dev.extent(0));
  Kokkos::View<T*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> host_v(
      host.data(), host.size());
  Kokkos::deep_copy(host_v, dev);
  return host;
}

// Fills only the KokkosDeviceState views K1/K3 read or write; the rest stay
// default-constructed (never dereferenced by these kernels).
gpl::KokkosDeviceState makeDeviceState(const SyntheticNetlist& nl)
{
  gpl::KokkosDeviceState ds;
  ds.d_net_pin_off = toDevice("t_net_pin_off", nl.net_pin_off);
  ds.d_net_pin_idx = toDevice("t_net_pin_idx", nl.net_pin_idx);
  ds.d_pin_cx = toDevice("t_pin_cx", nl.pin_cx);
  ds.d_pin_cy = toDevice("t_pin_cy", nl.pin_cy);
  ds.d_high_fanout_net_idx
      = toDevice("t_high_fanout_net_idx", nl.high_fanout_net_idx);

  const int n_nets = nl.numNets();
  ds.d_net_lx = Kokkos::View<int*>("t_net_lx", n_nets);
  ds.d_net_ly = Kokkos::View<int*>("t_net_ly", n_nets);
  ds.d_net_ux = Kokkos::View<int*>("t_net_ux", n_nets);
  ds.d_net_uy = Kokkos::View<int*>("t_net_uy", n_nets);

  ds.d_net_b_pos_x = Kokkos::View<float*>("t_net_b_pos_x", n_nets);
  ds.d_net_b_neg_x = Kokkos::View<float*>("t_net_b_neg_x", n_nets);
  ds.d_net_b_pos_y = Kokkos::View<float*>("t_net_b_pos_y", n_nets);
  ds.d_net_b_neg_y = Kokkos::View<float*>("t_net_b_neg_y", n_nets);
  ds.d_net_c_pos_x = Kokkos::View<float*>("t_net_c_pos_x", n_nets);
  ds.d_net_c_neg_x = Kokkos::View<float*>("t_net_c_neg_x", n_nets);
  ds.d_net_c_pos_y = Kokkos::View<float*>("t_net_c_pos_y", n_nets);
  ds.d_net_c_neg_y = Kokkos::View<float*>("t_net_c_neg_y", n_nets);
  return ds;
}

TEST(WlGpuTest, NetBBoxMatchesHostReferenceExactly)
{
  gpl::ensureKokkosInitialized();
  const SyntheticNetlist nl = makeNetlist();
  gpl::KokkosDeviceState ds = makeDeviceState(nl);

  gpl::wlop::launchUpdateNetBBox(ds, nl.numNets());
  Kokkos::fence();

  const std::vector<int> lx = toHost(ds.d_net_lx);
  const std::vector<int> ly = toHost(ds.d_net_ly);
  const std::vector<int> ux = toHost(ds.d_net_ux);
  const std::vector<int> uy = toHost(ds.d_net_uy);

  for (int n = 0; n < nl.numNets(); ++n) {
    int ref_lx = INT_MAX;
    int ref_ly = INT_MAX;
    int ref_ux = INT_MIN;
    int ref_uy = INT_MIN;
    for (int j = nl.net_pin_off[n]; j < nl.net_pin_off[n + 1]; ++j) {
      const int p = nl.net_pin_idx[j];
      ref_lx = std::min(ref_lx, nl.pin_cx[p]);
      ref_ly = std::min(ref_ly, nl.pin_cy[p]);
      ref_ux = std::max(ref_ux, nl.pin_cx[p]);
      ref_uy = std::max(ref_uy, nl.pin_cy[p]);
    }
    // Exact: int min/max must be order-independent. The dangling net (deg 0)
    // checks the INT_MAX/INT_MIN sentinel convention survives both passes.
    EXPECT_EQ(lx[n], ref_lx) << "net " << n << " deg " << nl.net_deg[n];
    EXPECT_EQ(ly[n], ref_ly) << "net " << n << " deg " << nl.net_deg[n];
    EXPECT_EQ(ux[n], ref_ux) << "net " << n << " deg " << nl.net_deg[n];
    EXPECT_EQ(uy[n], ref_uy) << "net " << n << " deg " << nl.net_deg[n];
  }
}

TEST(WlGpuTest, ComputeBCMatchesHostReference)
{
  gpl::ensureKokkosInitialized();
  const SyntheticNetlist nl = makeNetlist();
  gpl::KokkosDeviceState ds = makeDeviceState(nl);

  // Per-pin WA exponentials in (0, 1] — the kernel consumes them as-is, so
  // any deterministic positive values exercise the sums fully.
  Lcg lcg(0xBADF00Du);
  const int n_pins = nl.numPins();
  std::vector<float> a_pos_x(n_pins), a_neg_x(n_pins);
  std::vector<float> a_pos_y(n_pins), a_neg_y(n_pins);
  for (int p = 0; p < n_pins; ++p) {
    a_pos_x[p] = lcg.nextUnitFloat();
    a_neg_x[p] = lcg.nextUnitFloat();
    a_pos_y[p] = lcg.nextUnitFloat();
    a_neg_y[p] = lcg.nextUnitFloat();
  }
  ds.d_pin_a_pos_x = toDevice("t_pin_a_pos_x", a_pos_x);
  ds.d_pin_a_neg_x = toDevice("t_pin_a_neg_x", a_neg_x);
  ds.d_pin_a_pos_y = toDevice("t_pin_a_pos_y", a_pos_y);
  ds.d_pin_a_neg_y = toDevice("t_pin_a_neg_y", a_neg_y);

  gpl::wlop::launchComputeBC(ds, nl.numNets());
  Kokkos::fence();

  const std::vector<float> b_pos_x = toHost(ds.d_net_b_pos_x);
  const std::vector<float> b_neg_x = toHost(ds.d_net_b_neg_x);
  const std::vector<float> b_pos_y = toHost(ds.d_net_b_pos_y);
  const std::vector<float> b_neg_y = toHost(ds.d_net_b_neg_y);
  const std::vector<float> c_pos_x = toHost(ds.d_net_c_pos_x);
  const std::vector<float> c_neg_x = toHost(ds.d_net_c_neg_x);
  const std::vector<float> c_pos_y = toHost(ds.d_net_c_pos_y);
  const std::vector<float> c_neg_y = toHost(ds.d_net_c_neg_y);

  // Double-precision reference over the same float inputs. Covers serial
  // float error growth on threshold-sized nets (~6e-5) and the team pass's
  // (smaller) tree-reduction error, with an order of magnitude of margin.
  constexpr double kRelTol = 1e-3;
  auto expectNear = [&](float got, double ref, int n, const char* what) {
    const double denom = std::max(1.0, std::abs(ref));
    EXPECT_NEAR(static_cast<double>(got) / denom, ref / denom, kRelTol)
        << what << " of net " << n << " deg " << nl.net_deg[n];
  };

  for (int n = 0; n < nl.numNets(); ++n) {
    double ref_bpx = 0, ref_bnx = 0, ref_bpy = 0, ref_bny = 0;
    double ref_cpx = 0, ref_cnx = 0, ref_cpy = 0, ref_cny = 0;
    for (int j = nl.net_pin_off[n]; j < nl.net_pin_off[n + 1]; ++j) {
      const int p = nl.net_pin_idx[j];
      const double px = static_cast<float>(nl.pin_cx[p]);
      const double py = static_cast<float>(nl.pin_cy[p]);
      ref_bpx += a_pos_x[p];
      ref_bnx += a_neg_x[p];
      ref_bpy += a_pos_y[p];
      ref_bny += a_neg_y[p];
      ref_cpx += px * a_pos_x[p];
      ref_cnx += px * a_neg_x[p];
      ref_cpy += py * a_pos_y[p];
      ref_cny += py * a_neg_y[p];
    }
    expectNear(b_pos_x[n], ref_bpx, n, "b_pos_x");
    expectNear(b_neg_x[n], ref_bnx, n, "b_neg_x");
    expectNear(b_pos_y[n], ref_bpy, n, "b_pos_y");
    expectNear(b_neg_y[n], ref_bny, n, "b_neg_y");
    expectNear(c_pos_x[n], ref_cpx, n, "c_pos_x");
    expectNear(c_neg_x[n], ref_cnx, n, "c_neg_x");
    expectNear(c_pos_y[n], ref_cpy, n, "c_pos_y");
    expectNear(c_neg_y[n], ref_cny, n, "c_neg_y");
  }
}

}  // namespace
