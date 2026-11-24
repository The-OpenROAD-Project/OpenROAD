// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "CtsOptions.h"

#include <algorithm>

#include "odb/db.h"

namespace cts {

CtsOptions::RunConfiguration CtsOptions::saveRunConfiguration() const
{
  return {dbUnits_,
          wireSegmentUnit_,
          maxWl_,
          maxCharSlew_,
          maxCharCap_,
          sinkBufferInputCap_,
          rootBuffer_,
          sinkBuffer_,
          dlyBufferList_,
          maxFanout_,
          sinkClustersSize_,
          sinkClustersSizeSet_,
          macroMaxDiameter_,
          macroMaxDiameterSet_,
          rootBufferInferred_,
          sinkBufferInferred_,
          dummyLoad_};
}

void CtsOptions::restoreRunConfiguration(const RunConfiguration& config)
{
  dbUnits_ = config.db_units;
  wireSegmentUnit_ = config.wire_segment_unit;
  maxWl_ = config.max_wire_length;
  maxCharSlew_ = config.max_char_slew;
  maxCharCap_ = config.max_char_cap;
  sinkBufferInputCap_ = config.sink_buffer_input_cap;
  rootBuffer_ = config.root_buffer;
  sinkBuffer_ = config.sink_buffer;
  dlyBufferList_ = config.delay_buffers;
  maxFanout_ = config.max_fanout;
  sinkClustersSize_ = config.sink_cluster_size;
  sinkClustersSizeSet_ = config.sink_cluster_size_set;
  macroMaxDiameter_ = config.macro_max_diameter;
  macroMaxDiameterSet_ = config.macro_max_diameter_set;
  rootBufferInferred_ = config.root_buffer_inferred;
  sinkBufferInferred_ = config.sink_buffer_inferred;
  dummyLoad_ = config.dummy_load;
}

CtsOptions::MasterType CtsOptions::getType(odb::dbInst* inst) const
{
  if (inst->getName().substr(0, dummyload_prefix_.size())
      == dummyload_prefix_) {
    return MasterType::DUMMY;
  }

  return MasterType::TREE;
}

void CtsOptions::inDbInstCreate(odb::dbInst* inst)
{
  recordBuffer(inst->getMaster(), getType(inst));
}

void CtsOptions::limitSinkClusteringSizes(unsigned limit)
{
  unsigned new_size = limit;
  if (sinkClustersSizeSet_) {
    new_size = std::min(new_size, sinkClustersSize_);
  }
  setSinkClusteringSize(new_size);
}

void CtsOptions::recordBuffer(odb::dbMaster* master, MasterType type)
{
  switch (type) {
    case MasterType::DUMMY:
      dummy_count_[master]++;
      break;
    case MasterType::TREE:
      buffer_count_[master]++;
      break;
  }
}

}  // namespace cts
