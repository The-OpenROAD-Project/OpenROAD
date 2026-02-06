// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "gc/FlexGC.h"

#include <limits>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "db/drObj/drShape.h"
#include "db/gcObj/gcNet.h"
#include "db/gcObj/gcPin.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frMarker.h"
#include "db/tech/frTechObject.h"
#include "dr/FlexDR.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "gc/FlexGC_impl.h"
#include "global.h"
#include "utl/Logger.h"

namespace drt {

FlexGCWorker::FlexGCWorker(frTechObject* techIn,
                           utl::Logger* logger,
                           RouterConfiguration* router_cfg,
                           FlexDRWorker* drWorkerIn)
    : impl_(
          std::make_unique<Impl>(techIn, logger, router_cfg, drWorkerIn, this))
{
}

FlexGCWorker::~FlexGCWorker() = default;

FlexGCWorker::Impl::Impl() : Impl(nullptr, nullptr, nullptr, nullptr, nullptr)
{
}

FlexGCWorker::Impl::Impl(frTechObject* techIn,
                         utl::Logger* logger,
                         RouterConfiguration* router_cfg,
                         FlexDRWorker* drWorkerIn,
                         FlexGCWorker* gcWorkerIn)
    : tech_(techIn),
      logger_(logger),
      router_cfg_(router_cfg),
      drWorker_(drWorkerIn),
      rq_(gcWorkerIn),
      printMarker_(false),
      targetNet_(nullptr),
      targetDRNet_(nullptr),
      minLayerNum_(std::numeric_limits<frLayerNum>::min()),
      maxLayerNum_(std::numeric_limits<frLayerNum>::max()),
      ignoreDB_(false),
      ignoreMinArea_(false),
      ignoreLongSideEOL_(false),
      ignoreCornerSpacing_(false),
      surgicalFixEnabled_(false)
{
}

void FlexGCWorker::Impl::addMarker(std::unique_ptr<frMarker> in)
{
  odb::Rect bbox = in->getBBox();
  auto layerNum = in->getLayerNum();
  auto con = in->getConstraint();
  if (mapMarkers_.find({bbox, layerNum, con, in->getSrcs()})
      != mapMarkers_.end()) {
    return;
  }
  mapMarkers_[{bbox, layerNum, con, in->getSrcs()}] = in.get();
  markers_.push_back(std::move(in));
}

void FlexGCWorker::addPAObj(frConnFig* obj, frBlockObject* owner)
{
  impl_->addPAObj(obj, owner);
}

void FlexGCWorker::init(const frDesign* design)
{
  impl_->init(design);
}

int FlexGCWorker::main()
{
  return impl_->main();
}

void FlexGCWorker::checkMinStep(gcPin* pin)
{
  impl_->checkMetalShape_minStep(pin);
}

void FlexGCWorker::updateGCWorker()
{
  impl_->updateGCWorker();
}

void FlexGCWorker::initPA0(const frDesign* design)
{
  impl_->initPA0(design);
}

void FlexGCWorker::initPA1()
{
  impl_->initPA1();
}

void FlexGCWorker::setExtBox(const odb::Rect& in)
{
  impl_->extBox_ = in;
}

void FlexGCWorker::setDrcBox(const odb::Rect& in)
{
  impl_->drcBox_ = in;
}

const std::vector<std::unique_ptr<frMarker>>& FlexGCWorker::getMarkers() const
{
  return impl_->markers_;
}

const std::vector<std::unique_ptr<drPatchWire>>& FlexGCWorker::getPWires() const
{
  return impl_->pwires_;
}

void FlexGCWorker::clearPWires()
{
  impl_->pwires_.clear();
}

bool FlexGCWorker::setTargetNet(frBlockObject* in)
{
  auto& owner2nets = impl_->owner2nets_;
  if (owner2nets.find(in) != owner2nets.end()) {
    impl_->targetNet_ = owner2nets[in];
    return true;
  }
  return false;
}

bool FlexGCWorker::setTargetNet(drNet* in)
{
  bool found = setTargetNet(in->getFrNet());
  if (found) {
    impl_->targetDRNet_ = in;
  }
  return found;
}

gcNet* FlexGCWorker::getTargetNet()
{
  return impl_->targetNet_;
}
void FlexGCWorker::setEnableSurgicalFix(bool in)
{
  impl_->surgicalFixEnabled_ = in;
}

void FlexGCWorker::resetTargetNet()
{
  impl_->targetNet_ = nullptr;
  impl_->targetDRNet_ = nullptr;
}

void FlexGCWorker::addTargetObj(frBlockObject* in)
{
  impl_->targetObjs_.insert(in);
}

void FlexGCWorker::setTargetObjs(const std::set<frBlockObject*>& targetObjs)
{
  impl_->targetObjs_ = targetObjs;
}

void FlexGCWorker::setIgnoreDB()
{
  impl_->ignoreDB_ = true;
}

void FlexGCWorker::setIgnoreMinArea()
{
  impl_->ignoreMinArea_ = true;
}

void FlexGCWorker::setIgnoreCornerSpacing()
{
  impl_->ignoreCornerSpacing_ = true;
}

void FlexGCWorker::setIgnoreLongSideEOL()
{
  impl_->ignoreLongSideEOL_ = true;
}

std::vector<std::unique_ptr<gcNet>>& FlexGCWorker::getNets()
{
  return impl_->getNets();
}

gcNet* FlexGCWorker::getNet(frNet* net)
{
  return impl_->getNet(net);
}

}  // namespace drt
