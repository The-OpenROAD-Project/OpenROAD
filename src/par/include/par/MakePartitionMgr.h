// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace par {
class PartitionMgr;
}

namespace ord {

class OpenRoad;

par::PartitionMgr* makePartitionMgr();

void initPartitionMgr(OpenRoad* openroad);

void deletePartitionMgr(par::PartitionMgr* partitionmgr);

}  // namespace ord
