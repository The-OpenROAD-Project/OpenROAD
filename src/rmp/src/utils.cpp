// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

#include "src/rmp/src/utils.h"

#include <fcntl.h>

#include <vector>

#include "aig/aig/aig.h"
#include "aig/gia/gia.h"
#include "base/abc/abc.h"
#include "src/cut/include/cut/abc_library_factory.h"
#include "src/cut/include/cut/logic_cut.h"
#include "src/dbSta/include/db_sta/dbNetwork.hh"
#include "src/dbSta/include/db_sta/dbSta.hh"
#include "src/rsz/include/rsz/Resizer.hh"
#include "src/sta/include/sta/Delay.hh"
#include "src/sta/include/sta/Graph.hh"
#include "src/sta/include/sta/MinMax.hh"
#include "src/sta/include/sta/PortDirection.hh"
#include "src/utl/include/utl/deleter.h"

namespace rmp {

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> WrapUnique(abc::Abc_Ntk_t* ntk)
{
  return {ntk, &abc::Abc_NtkDelete};
}
utl::UniquePtrWithDeleter<abc::Aig_Man_t> WrapUnique(abc::Aig_Man_t* aig)
{
  return {aig, &abc::Aig_ManStop};
}
utl::UniquePtrWithDeleter<abc::Gia_Man_t> WrapUnique(abc::Gia_Man_t* gia)
{
  return {gia, &abc::Gia_ManStop};
}

std::vector<sta::Vertex*> GetEndpoints(sta::dbSta* sta,
                                       rsz::Resizer* resizer,
                                       sta::Slack slack_threshold)
{
  std::vector<sta::Vertex*> result;

  sta::dbNetwork* network = sta->getDbNetwork();
  for (sta::Vertex* vertex : sta->endpoints()) {
    sta::Pin* pin = vertex->pin();
    sta::PortDirection* direction = network->direction(vertex->pin());
    if (!direction->isInput()) {
      continue;
    }

    if (resizer != nullptr) {
      if (resizer->dontTouch(pin) || resizer->dontTouch(network->net(pin))
          || resizer->dontTouch(network->instance(pin))) {
        continue;
      }
    }

    const sta::Slack slack = sta->slack(vertex, sta::MinMax::max());

    if (slack < slack_threshold) {
      result.push_back(vertex);
    }
  }

  return result;
}

}  // namespace rmp
