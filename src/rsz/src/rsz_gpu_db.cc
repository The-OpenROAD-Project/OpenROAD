#include "sta/Scene.hh"
#include "sta/MinMax.hh"
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2026, The OpenROAD Authors

#include "rsz_gpu_db.cuh"

#include <chrono>
#include <map>
#include <string>

#include "sta/ConcreteLibrary.hh"
#include "sta/EquivCells.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PortDirection.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"
namespace rsz {
namespace gpu {

namespace {
using Clock = std::chrono::steady_clock;
double msSince(Clock::time_point t)
{
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}
}  // namespace

GpuResizerDb::GpuResizerDb()
    : num_cells_(0), num_instances_(0), d_cells_(nullptr), d_instances_(nullptr)
{
}

GpuResizerDb::~GpuResizerDb()
{
  clear();
}

void GpuResizerDb::clear()
{
  if (d_cells_) {
    cudaFree(d_cells_);
    d_cells_ = nullptr;
  }
  if (d_instances_) {
    cudaFree(d_instances_);
    d_instances_ = nullptr;
  }
  h_cells_.clear();
  h_instances_.clear();
  num_cells_ = 0;
  num_instances_ = 0;
}

void GpuResizerDb::init(sta::Network* network)
{
  clear();
  std::map<sta::LibertyCell*, int> cell_to_index;

  auto t0 = Clock::now();
  sta::LibertyLibrarySeq libs;
  sta::LibertyLibraryIterator* lib_iter = network->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    libs.push_back(lib_iter->next());
  }
  delete lib_iter;

  std::vector<sta::LibertyCell*> all_cells;
  for (sta::LibertyLibrary* lib : libs) {
    sta::ConcreteLibraryCellIterator* cell_iter = lib->cellIterator();
    while (cell_iter->hasNext()) {
      sta::ConcreteCell* conc_cell = cell_iter->next();
      sta::LibertyCell* cell = conc_cell->libertyCell();
      if (cell) {
        all_cells.push_back(cell);
      }
    }
    delete cell_iter;
  }
  liberty_ms = msSince(t0);

  t0 = Clock::now();
  std::vector<std::vector<sta::LibertyCell*>> equiv_classes;
  for (sta::LibertyCell* cell : all_cells) {
    bool found = false;
    for (auto& eq_class : equiv_classes) {
      if (sta::equivCells(cell, eq_class[0])) {
        eq_class.push_back(cell);
        found = true;
        break;
      }
    }
    if (!found) {
      equiv_classes.push_back({cell});
    }
  }

  int current_cell_index = 0;
  for (auto& eq_class : equiv_classes) {
    int alt_start = current_cell_index;
    int alt_count = eq_class.size();

    for (sta::LibertyCell* cell : eq_class) {
      GpuCellInfo gpu_cell;
      gpu_cell.area = cell->area();
      float leakage = 0.0f;
      bool exists = false;
      cell->leakagePower(leakage, exists);
      gpu_cell.leakage_power = exists ? leakage : 0.0f;
      gpu_cell.num_input_pins = 0;
      gpu_cell.alt_start = alt_start;
      gpu_cell.alt_count = alt_count;
      gpu_cell.drive_strength = 0.0f;
      gpu_cell.mean_input_cap = 0.0f;

      float input_cap_sum = 0.0f;
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        sta::LibertyPort* port = port_iter.next();
        if (port->direction()->isInput()
            && gpu_cell.num_input_pins < MAX_PINS_PER_CELL) {
          gpu_cell.pin_caps[gpu_cell.num_input_pins] = port->capacitance();
          input_cap_sum += port->capacitance();
          gpu_cell.num_input_pins++;
        } else if (port->direction()->isOutput()) {
          float res = port->driveResistance();
          if (res > 0.0f) {
            gpu_cell.drive_strength = 1.0f / res;
          } else {
            gpu_cell.drive_strength = 1e6f;
          }
        }
      }
      if (gpu_cell.num_input_pins > 0) {
        gpu_cell.mean_input_cap = input_cap_sum / gpu_cell.num_input_pins;
      }

      h_cells_.push_back(gpu_cell);
      cell_names_.push_back(cell->name());
      cell_to_index[cell] = current_cell_index++;
    }
  }
  num_cells_ = h_cells_.size();
  equiv_ms = msSince(t0);

  t0 = Clock::now();
  sta::Instance* top_inst = network->topInstance();
  if (top_inst) {
    sta::InstanceChildIterator* child_iter = network->childIterator(top_inst);
    while (child_iter->hasNext()) {
      sta::Instance* inst = child_iter->next();
      sta::LibertyCell* cell = network->libertyCell(inst);
      if (cell) {
        GpuInstanceInfo gpu_inst;
        auto it = cell_to_index.find(cell);
        gpu_inst.cell_index = (it != cell_to_index.end()) ? it->second : -1;

        gpu_inst.slack = 0.0f;
        gpu_inst.load_cap = 0.0f;
        gpu_inst.fanout = 0;
        gpu_inst.best_cell_index = gpu_inst.cell_index;
        gpu_inst.sensitivity = 0.0f;
        h_instances_.push_back(gpu_inst);
      }
    }
    delete child_iter;
  }
  num_instances_ = h_instances_.size();
  inst_extract_ms = msSince(t0);

  t0 = Clock::now();
  if (num_cells_ > 0) {
    cudaMalloc(&d_cells_, num_cells_ * sizeof(GpuCellInfo));
  }
  if (num_instances_ > 0) {
    cudaMalloc(&d_instances_, num_instances_ * sizeof(GpuInstanceInfo));
  }
  malloc_ms = msSince(t0);

  t0 = Clock::now();
  if (num_cells_ > 0) {
    cudaMemcpy(d_cells_,
               h_cells_.data(),
               num_cells_ * sizeof(GpuCellInfo),
               cudaMemcpyHostToDevice);
  }
  if (num_instances_ > 0) {
    cudaMemcpy(d_instances_,
               h_instances_.data(),
               num_instances_ * sizeof(GpuInstanceInfo),
               cudaMemcpyHostToDevice);
  }
  h2d_ms = msSince(t0);
}

void GpuResizerDb::updateTimingSlacks(sta::Sta* sta, sta::Network* network)
{
  if (num_instances_ == 0) {
    return;
  }

  const auto t_host = Clock::now();
  for (int i = 0; i < num_instances_; i++) {
    h_instances_[i].slack = 1e9f;
  }

  sta->findRequireds();

  int inst_idx = 0;
  sta::Instance* top_inst = network->topInstance();
  if (top_inst) {
    sta::InstanceChildIterator* child_iter = network->childIterator(top_inst);
    while (child_iter->hasNext() && inst_idx < num_instances_) {
      sta::Instance* inst = child_iter->next();
      sta::LibertyCell* cell = network->libertyCell(inst);
      if (!cell) {
        continue;
      }

      float worst_slack = 1e9f;
      float worst_load = 0.0f;
      int fanout = 0;

      sta::InstancePinIterator* pin_iter = network->pinIterator(inst);
      while (pin_iter->hasNext()) {
        sta::Pin* pin = pin_iter->next();
        if (!network->direction(pin)->isOutput()) {
          continue;
        }
        sta::Vertex* vertex = sta->graph()->pinDrvrVertex(pin);
        if (vertex) {
          sta::Slack s = sta->slack(vertex, sta::MinMax::max());
          if (s != sta::INF && (float) s < worst_slack) {
            worst_slack = (float) s;
          }
        }
        sta::GraphDelayCalc* gdc = sta->graphDelayCalc();
        const sta::Scene* scene = sta->cmdScene();
        if (gdc && scene) {
          float cap = (float) gdc->loadCap(pin, scene, sta::MinMax::max());
          if (cap > worst_load) {
            worst_load = cap;
          }
        }
        int fo = 0;
        sta::PinConnectedPinIterator* cpi = network->connectedPinIterator(pin);
        while (cpi->hasNext()) {
          if (cpi->next() != pin) {
            fo++;
          }
        }
        delete cpi;
        if (fo > fanout) {
          fanout = fo;
        }
      }
      delete pin_iter;

      if (worst_slack < 1e8f) {
        h_instances_[inst_idx].slack = worst_slack;
      } else {
        h_instances_[inst_idx].slack = 0.0f;
      }
      h_instances_[inst_idx].load_cap = worst_load;
      h_instances_[inst_idx].fanout = fanout;
      h_instances_[inst_idx].best_cell_index = h_instances_[inst_idx].cell_index;
      h_instances_[inst_idx].sensitivity = 0.0f;

      inst_idx++;
    }
    delete child_iter;
  }
  slack_host_ms += msSince(t_host);

  const auto t_h2d = Clock::now();
  cudaMemcpy(d_instances_,
             h_instances_.data(),
             num_instances_ * sizeof(GpuInstanceInfo),
             cudaMemcpyHostToDevice);
  slack_h2d_ms += msSince(t_h2d);
}

std::vector<GpuSwapResult> GpuResizerDb::readSwapResults()
{
  std::vector<GpuSwapResult> results;

  if (num_instances_ > 0) {
    const auto t_d2h = Clock::now();
    cudaMemcpy(h_instances_.data(),
               d_instances_,
               num_instances_ * sizeof(GpuInstanceInfo),
               cudaMemcpyDeviceToHost);
    d2h_memcpy_ms += msSince(t_d2h);

    for (int i = 0; i < num_instances_; i++) {
      if (h_instances_[i].best_cell_index >= 0
          && h_instances_[i].best_cell_index != h_instances_[i].cell_index) {
        GpuSwapResult res;
        res.instance_index = i;
        res.new_cell_index = h_instances_[i].best_cell_index;
        results.push_back(res);
      }
    }
  }

  return results;
}

void GpuResizerDb::keepOnlyTargetSlacks(const std::vector<uint8_t>& keep)
{
  if (num_instances_ == 0
      || keep.size() != static_cast<size_t>(num_instances_)) {
    return;
  }
  bool any = false;
  for (int i = 0; i < num_instances_; i++) {
    if (keep[i] == 0) {
      h_instances_[i].slack = 0.0f;
    } else {
      any = true;
    }
  }
  if (!any) {
    return;
  }
  cudaMemcpy(d_instances_,
             h_instances_.data(),
             num_instances_ * sizeof(GpuInstanceInfo),
             cudaMemcpyHostToDevice);
}

void GpuResizerDb::noteSwap(int instance_index, int new_cell_index)
{
  if (instance_index < 0 || instance_index >= num_instances_) {
    return;
  }
  h_instances_[instance_index].cell_index = new_cell_index;
  h_instances_[instance_index].best_cell_index = new_cell_index;
  h_instances_[instance_index].sensitivity = 0.0f;
}

}  // namespace gpu
}  // namespace rsz
