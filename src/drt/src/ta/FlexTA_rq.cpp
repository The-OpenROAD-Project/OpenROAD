// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "db/obj/frBlockObject.h"
#include "db/taObj/taFig.h"
#include "db/taObj/taPin.h"
#include "db/taObj/taShape.h"
#include "db/taObj/taVia.h"
#include "db/tech/frConstraint.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frRTree.h"
#include "odb/geom.h"
#include "ta/FlexTA.h"

namespace drt {

namespace {

bool getTaPinFigBoxAndLayer(taPinFig* fig,
                            odb::Rect& box,
                            frLayerNum& layer_num)
{
  if (fig->typeId() == tacPathSeg) {
    auto* obj = static_cast<taPathSeg*>(fig);
    auto [begin, end] = obj->getPoints();
    box = odb::Rect(begin, end);
    layer_num = obj->getLayerNum();
    return true;
  }
  if (fig->typeId() == tacVia) {
    auto* obj = static_cast<taVia*>(fig);
    const auto origin = obj->getOrigin();
    box = odb::Rect(origin, origin);
    layer_num = obj->getViaDef()->getCutLayerNum();
    return true;
  }
  return false;
}

}  // namespace

struct FlexTAWorkerRegionQuery::Impl
{
  FlexTAWorker* ta_worker;
  std::vector<RTree<taPinFig*>> shapes;  // resource map
  // fixed objs, owner:: nullptr or net, con = short
  std::vector<RTree<std::pair<frBlockObject*, frConstraint*>>> route_costs;
  std::vector<RTree<std::pair<frBlockObject*, frConstraint*>>> via_costs;
};

FlexTAWorkerRegionQuery::FlexTAWorkerRegionQuery(FlexTAWorker* in)
    : impl_(std::make_unique<Impl>())
{
  impl_->ta_worker = in;
}

FlexTAWorkerRegionQuery::~FlexTAWorkerRegionQuery() = default;

FlexTAWorker* FlexTAWorkerRegionQuery::getTAWorker() const
{
  return impl_->ta_worker;
}

frDesign* FlexTAWorkerRegionQuery::getDesign() const
{
  return impl_->ta_worker->getDesign();
}

void FlexTAWorkerRegionQuery::add(taPinFig* fig)
{
  odb::Rect box;
  frLayerNum layer_num = 0;
  if (!getTaPinFigBoxAndLayer(fig, box, layer_num)) {
    std::cout << "Error: unsupported region query add\n";
    return;
  }
  impl_->shapes.at(layer_num).insert(std::make_pair(box, fig));
}

void FlexTAWorkerRegionQuery::remove(taPinFig* fig)
{
  odb::Rect box;
  frLayerNum layer_num = 0;
  if (!getTaPinFigBoxAndLayer(fig, box, layer_num)) {
    std::cout << "Error: unsupported region query add\n";
    return;
  }
  impl_->shapes.at(layer_num).remove(std::make_pair(box, fig));
}

void FlexTAWorkerRegionQuery::query(const odb::Rect& box,
                                    const frLayerNum layer_num,
                                    frOrderedIdSet<taPin*>& result) const
{
  auto& tree = impl_->shapes.at(layer_num);
  transform(tree.qbegin(bgi::intersects(box)),
            tree.qend(),
            inserter(result, result.end()),
            [](const auto& box_fig) { return box_fig.second->getPin(); });
}

void FlexTAWorkerRegionQuery::init()
{
  int num_layers = getDesign()->getTech()->getLayers().size();
  impl_->shapes.clear();
  impl_->shapes.resize(num_layers);
  impl_->route_costs.clear();
  impl_->route_costs.resize(num_layers);
  impl_->via_costs.clear();
  impl_->via_costs.resize(num_layers);
}

void FlexTAWorkerRegionQuery::addCost(const odb::Rect& box,
                                      const frLayerNum layer_num,
                                      frBlockObject* obj,
                                      frConstraint* con)
{
  impl_->route_costs.at(layer_num).insert(
      std::make_pair(box, std::make_pair(obj, con)));
}

void FlexTAWorkerRegionQuery::removeCost(const odb::Rect& box,
                                         const frLayerNum layer_num,
                                         frBlockObject* obj,
                                         frConstraint* con)
{
  impl_->route_costs.at(layer_num).remove(
      std::make_pair(box, std::make_pair(obj, con)));
}

void FlexTAWorkerRegionQuery::queryCost(
    const odb::Rect& box,
    const frLayerNum layer_num,
    std::vector<rq_box_value_t<std::pair<frBlockObject*, frConstraint*>>>&
        result) const
{
  impl_->route_costs.at(layer_num).query(bgi::intersects(box),
                                         back_inserter(result));
}

void FlexTAWorkerRegionQuery::addViaCost(const odb::Rect& box,
                                         const frLayerNum layer_num,
                                         frBlockObject* obj,
                                         frConstraint* con)
{
  impl_->via_costs.at(layer_num).insert(
      std::make_pair(box, std::make_pair(obj, con)));
}

void FlexTAWorkerRegionQuery::removeViaCost(const odb::Rect& box,
                                            const frLayerNum layer_num,
                                            frBlockObject* obj,
                                            frConstraint* con)
{
  impl_->via_costs.at(layer_num).remove(
      std::make_pair(box, std::make_pair(obj, con)));
}

void FlexTAWorkerRegionQuery::queryViaCost(
    const odb::Rect& box,
    const frLayerNum layer_num,
    std::vector<rq_box_value_t<std::pair<frBlockObject*, frConstraint*>>>&
        result) const
{
  impl_->via_costs.at(layer_num).query(bgi::intersects(box),
                                       back_inserter(result));
}

}  // namespace drt
