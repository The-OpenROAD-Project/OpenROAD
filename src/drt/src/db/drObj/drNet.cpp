/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "db/drObj/drNet.h"

#include "distributed/frArchive.h"
#include "dr/FlexDR.h"
#include "serialization.h"
using namespace fr;
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
  (ar) & pins_;
  (ar) & extConnFigs_;
  (ar) & routeConnFigs_;
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
  (ar) & origGuides_;
  if (is_loading(ar)) {
    frBlockObject* obj;
    serializeBlockObject(ar, obj);
    fNet_ = (frNet*) obj;
    int terms_sz;
    (ar) & terms_sz;
    while (terms_sz--) {
      serializeBlockObject(ar, obj);
      fNetTerms_.insert(obj);
    }
  } else {
    frBlockObject* obj = (frBlockObject*) fNet_;
    serializeBlockObject(ar, obj);
    int terms_sz = fNetTerms_.size();
    (ar) & terms_sz;
    for (auto fNetTerm : fNetTerms_) {
      obj = (frBlockObject*) fNetTerm;
      serializeBlockObject(ar, obj);
    }
  }
}

// Explicit instantiations
template void drNet::serialize<frIArchive>(frIArchive& ar,
                                           const unsigned int file_version);

template void drNet::serialize<frOArchive>(frOArchive& ar,
                                           const unsigned int file_version);
