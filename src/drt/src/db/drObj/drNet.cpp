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

#include "db/drObj/drNet.h"

#include "distributed/frArchive.h"
#include "dr/FlexDR.h"
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
}

// Explicit instantiations
template void drNet::serialize<frIArchive>(frIArchive& ar,
                                           const unsigned int file_version);

template void drNet::serialize<frOArchive>(frOArchive& ar,
                                           const unsigned int file_version);

}  // namespace drt
