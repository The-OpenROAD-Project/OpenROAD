// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2026, The OpenROAD Authors

#pragma once

#include <cuda_runtime.h>
#include <cstdint>
#include <vector>
#include <string>

// Forward declarations of STA/OpenDB classes we will read from
namespace sta {
class dbSta;
class Network;
class LibertyCell;
class Instance;
class LibertyPort;
class Sta;
}  // namespace sta

namespace rsz {
namespace gpu {

// Maximum number of input pins per cell we support in the GPU fast path
constexpr int MAX_PINS_PER_CELL = 8;
// Number of points in a standard timing Liberty LUT (e.g., 7x7)
constexpr int LUT_SIZE = 49;
// Maximum alternative cell sizes we evaluate per instance (same function group)
constexpr int MAX_ALTS = 16;

// A compact struct representing a Liberty standard cell on the GPU
struct GpuCellInfo {
  float area;
  float leakage_power;

  // Basic pin capacitance for up to MAX_PINS_PER_CELL inputs
  int num_input_pins;
  float pin_caps[MAX_PINS_PER_CELL];

  // Phase 2: Timing-driven sizing - drive strength as a proxy for delay
  // Higher drive_strength = faster cell (lower delay)
  float drive_strength;

  // Mean input pin capacitance — proxy for upstream loading / routability
  float mean_input_cap;

  // Index offset into global alternative cell array for the same function
  int alt_start;  // index into h_alt_cells_ where alternatives begin
  int alt_count;  // number of alternative sizes for this cell's function
};

// Represents an instantiated gate in the design
struct GpuInstanceInfo {
  int cell_index;      // Index into the global GpuCellInfo array
  float slack;         // Current timing slack (negative = violating)
  float load_cap;      // Current capacitive load driven by this instance
  int fanout;          // Output connected-pin count (routability proxy)

  // Phase 2: Set by sensitivity kernel, read back on CPU for actual swap
  int best_cell_index;  // Best alternative cell chosen by GPU
  float sensitivity;    // Computed improvement score (higher = more benefit)
};

// Phase 2: Lightweight struct to pass cell swap results back to CPU
struct GpuSwapResult {
  int instance_index;  // which instance to swap
  int new_cell_index;  // which new cell to use
};

// Main GPU Database Manager for Resizer
class GpuResizerDb {
 public:
  GpuResizerDb();
  ~GpuResizerDb();

  // Phase 1: Initialize the GPU buffers from OpenSTA network
  void init(sta::Network* network);

  // Re-read STA slacks / loads and upload host instances (including
  // cell_index after noteSwap) for the next GPU sizing iteration.
  void updateTimingSlacks(sta::Sta* sta, sta::Network* network);

  // Host-side target mask: keep STA slack only for selected instance
  // indices so the existing kernel still skips slack >= 0. Re-uploads.
  void keepOnlyTargetSlacks(const std::vector<uint8_t>& keep);

  // Phase 2: Read back GPU swap decisions to CPU
  std::vector<GpuSwapResult> readSwapResults();

  // Record a CPU replaceCell so later updateTimingSlacks uploads the new size.
  void noteSwap(int instance_index, int new_cell_index);

  const GpuCellInfo& hostCell(int cell_index) const { return h_cells_[cell_index]; }
  const GpuInstanceInfo& hostInstance(int inst_index) const {
    return h_instances_[inst_index];
  }

  void clear();

  // Getters for GPU device pointers
  GpuCellInfo* getDeviceCells() const { return d_cells_; }
  GpuInstanceInfo* getDeviceInstances() const { return d_instances_; }
  int getNumCells() const { return num_cells_; }
  int getNumInstances() const { return num_instances_; }

  // Phase 2: Map GPU cell index back to liberty cell name (for CPU swap)
  const std::string& getCellName(int cell_index) const {
    return cell_names_[cell_index];
  }

  double liberty_ms = 0.0;
  double equiv_ms = 0.0;
  double inst_extract_ms = 0.0;
  double malloc_ms = 0.0;
  double h2d_ms = 0.0;
  double slack_host_ms = 0.0;
  double slack_h2d_ms = 0.0;
  double d2h_memcpy_ms = 0.0;

 private:
  int num_cells_;
  int num_instances_;

  // Host arrays (pinned memory or simple vectors before transfer)
  std::vector<GpuCellInfo> h_cells_;
  std::vector<GpuInstanceInfo> h_instances_;

  // Phase 2: Cell name lookup (needed to call replaceCell on CPU)
  std::vector<std::string> cell_names_;

  // Device pointers
  GpuCellInfo* d_cells_ = nullptr;
  GpuInstanceInfo* d_instances_ = nullptr;
};

}  // namespace gpu
}  // namespace rsz
