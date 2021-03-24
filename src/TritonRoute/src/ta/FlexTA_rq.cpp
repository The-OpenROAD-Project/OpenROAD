/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "frRTree.h"
#include "ta/FlexTA.h"

using namespace std;
using namespace fr;

struct FlexTAWorkerRegionQuery::Impl
{
  FlexTAWorker* taWorker;
  std::vector<bgi::rtree<rq_box_value_t<taPinFig*>,
                         bgi::quadratic<16>>>
      shapes;  // resource map
  // fixed objs, owner:: nullptr or net, con = short
  std::vector<
      bgi::rtree<rq_box_value_t<std::pair<frBlockObject*, frConstraint*>>,
                 bgi::quadratic<16>>>
      costs;
};

FlexTAWorkerRegionQuery::FlexTAWorkerRegionQuery(FlexTAWorker* in)
    : impl_(make_unique<Impl>())
{
  impl_->taWorker = in;
}

FlexTAWorkerRegionQuery::~FlexTAWorkerRegionQuery() = default;

FlexTAWorker* FlexTAWorkerRegionQuery::getTAWorker() const
{
  return impl_->taWorker;
}

frDesign* FlexTAWorkerRegionQuery::getDesign() const
{
  return impl_->taWorker->getDesign();
}

void FlexTAWorkerRegionQuery::add(taPinFig* fig)
{
  frBox box;
  frPoint bp, ep;
  if (fig->typeId() == tacPathSeg) {
    auto obj = static_cast<taPathSeg*>(fig);
    obj->getPoints(bp, ep);
    box = frBox(bp, ep);
    impl_->shapes.at(obj->getLayerNum()).insert(make_pair(box, obj));
  } else if (fig->typeId() == tacVia) {
    auto obj = static_cast<taVia*>(fig);
    obj->getOrigin(bp);
    box = frBox(bp, bp);
    impl_->shapes.at(obj->getViaDef()->getCutLayerNum())
        .insert(make_pair(box, obj));
  } else {
    cout << "Error: unsupported region query add" << endl;
  }
}

void FlexTAWorkerRegionQuery::remove(taPinFig* fig)
{
  frBox box;
  frPoint bp, ep;
  if (fig->typeId() == tacPathSeg) {
    auto obj = static_cast<taPathSeg*>(fig);
    obj->getPoints(bp, ep);
    box = frBox(bp, ep);
    impl_->shapes.at(obj->getLayerNum()).remove(make_pair(box, obj));
  } else if (fig->typeId() == tacVia) {
    auto obj = static_cast<taVia*>(fig);
    obj->getOrigin(bp);
    box = frBox(bp, bp);
    impl_->shapes.at(obj->getViaDef()->getCutLayerNum())
        .remove(make_pair(box, obj));
  } else {
    cout << "Error: unsupported region query add" << endl;
  }
}

void FlexTAWorkerRegionQuery::query(const frBox& box,
                                    frLayerNum layerNum,
                                    set<taPin*, frBlockObjectComp>& result)
{
  vector<rq_box_value_t<taPinFig*>> temp;
  auto& tree = impl_->shapes.at(layerNum);
  transform(tree.qbegin(bgi::intersects(box)),
            tree.qend(),
            inserter(result, result.end()),
            [](const auto& box_fig) { return box_fig.second->getPin(); });
}

void FlexTAWorkerRegionQuery::init()
{
  int numLayers = getDesign()->getTech()->getLayers().size();
  impl_->shapes.clear();
  impl_->shapes.resize(numLayers);
  impl_->costs.clear();
  impl_->costs.resize(numLayers);
}

void FlexTAWorkerRegionQuery::addCost(const frBox& box,
                                      frLayerNum layerNum,
                                      frBlockObject* obj,
                                      frConstraint* con)
{
  impl_->costs.at(layerNum).insert(make_pair(box, make_pair(obj, con)));
}

void FlexTAWorkerRegionQuery::removeCost(const frBox& box,
                                         frLayerNum layerNum,
                                         frBlockObject* obj,
                                         frConstraint* con)
{
  impl_->costs.at(layerNum).remove(make_pair(box, make_pair(obj, con)));
}

void FlexTAWorkerRegionQuery::queryCost(
    const frBox& box,
    frLayerNum layerNum,
    vector<rq_box_value_t<pair<frBlockObject*, frConstraint*>>>& result)
{
  impl_->costs.at(layerNum).query(bgi::intersects(box), back_inserter(result));
}
