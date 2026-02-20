// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/drObj/drNet.h"

#include <memory>
#include <utility>
#include <vector>

#include "db/drObj/drBlockObject.h"
#include "db/drObj/drShape.h"
#include "db/drObj/drVia.h"
#include "db/obj/frAccess.h"
#include "db/obj/frBTerm.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frInstTerm.h"
#include "distributed/frArchive.h"
#include "dr/FlexDR.h"
#include "frBaseTypes.h"
#include "serialization.h"

namespace drt {

void drNet::setBestRouteConnFigs()
{
  bestRouteConnFigs_.clear();
  for (auto& uConnFig : routeConnFigs_) {
    if (uConnFig->typeId() == drcPathSeg) {
      std::unique_ptr<drConnFig> uPtr = std::make_unique<drPathSeg>(
          *static_cast<drPathSeg*>(uConnFig.get()));
      bestRouteConnFigs_.push_back(std::move(uPtr));
    } else if (uConnFig->typeId() == drcVia) {
      std::unique_ptr<drConnFig> uPtr
          = std::make_unique<drVia>(*static_cast<drVia*>(uConnFig.get()));
      bestRouteConnFigs_.push_back(std::move(uPtr));
    } else if (uConnFig->typeId() == drcPatchWire) {
      std::unique_ptr<drConnFig> uPtr = std::make_unique<drPatchWire>(
          *static_cast<drPatchWire*>(uConnFig.get()));
      bestRouteConnFigs_.push_back(std::move(uPtr));
    }
  }
}

void drNet::removeShape(drConnFig* shape, bool isExt)
{
  std::vector<std::unique_ptr<drConnFig>>* v
      = isExt ? &extConnFigs_ : &routeConnFigs_;
  for (int i = 0; i < v->size(); i++) {
    auto& s = (*v)[i];
    if (s.get() == shape) {
      v->erase(v->begin() + i);
      return;
    }
  }
}

void drNet::cleanup()
{
  pins_.clear();
  pins_.shrink_to_fit();
  extConnFigs_.clear();
  extConnFigs_.shrink_to_fit();
  routeConnFigs_.clear();
  routeConnFigs_.shrink_to_fit();
  fNetTerms_.clear();
  origGuides_.clear();
  origGuides_.shrink_to_fit();
}

frAccessPoint* drNet::getFrAccessPoint(frCoord x,
                                       frCoord y,
                                       frLayerNum lNum,
                                       frBlockObject** owner)
{
  for (auto& term : fNetTerms_) {
    if (term->typeId() == frBlockObjectEnum::frcInstTerm) {
      frInstTerm* it = static_cast<frInstTerm*>(term);
      frAccessPoint* ap = it->getAccessPoint(x, y, lNum);
      if (ap) {
        if (owner) {
          (*owner) = term;
        }
        return ap;
      }
    } else if (term->typeId() == frBlockObjectEnum::frcBTerm) {
      frBTerm* t = static_cast<frBTerm*>(term);
      frAccessPoint* ap = t->getAccessPoint(x, y, lNum, 0);
      if (ap) {
        if (owner) {
          (*owner) = term;
        }
        return ap;
      }
    }
  }
  return nullptr;
}

void drNet::incNRipupAvoids()
{
  nRipupAvoids_++;
}
bool drNet::hasNDR() const
{
  return getFrNet()->getNondefaultRule() != nullptr;
}

bool drNet::isClockNet() const
{
  return fNet_->isClock();
}
bool drNet::isFixed() const
{
  return fNet_->isFixed();
}

template <class Archive>
void drNet::ExtFigUpdate::serialize(Archive& ar, const unsigned int version)
{
  (ar) & updated_style;
  (ar) & is_bottom_connected;
  (ar) & is_top_connected;
  (ar) & is_via;
}

template <class Archive>
void drNet::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<drBlockObject>(*this);
  (ar) & bestRouteConnFigs_;
  (ar) & modified_;
  (ar) & numMarkers_;
  (ar) & numPinsIn_;
  (ar) & markerDist_;
  (ar) & allowRipup_;
  (ar) & pinBox_;
  (ar) & ripup_;
  (ar) & numReroutes_;
  (ar) & nRipupAvoids_;
  (ar) & maxRipupAvoids_;
  (ar) & inQueue_;
  (ar) & routed_;
  if (is_loading(ar)) {
    frBlockObject* obj;
    serializeBlockObject(ar, obj);
    fNet_ = (frNet*) obj;
  } else {
    frBlockObject* obj = (frBlockObject*) fNet_;
    serializeBlockObject(ar, obj);
  }
  (ar) & ext_figs_updates_;
}

// Explicit instantiations
template void drNet::serialize<frIArchive>(frIArchive& ar,
                                           const unsigned int file_version);

template void drNet::serialize<frOArchive>(frOArchive& ar,
                                           const unsigned int file_version);

}  // namespace drt
