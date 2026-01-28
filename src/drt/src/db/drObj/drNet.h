// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "db/drObj/drBlockObject.h"
#include "db/drObj/drPin.h"
#include "db/drObj/drShape.h"
#include "db/drObj/drVia.h"
#include "db/infra/frPoint.h"
#include "db/infra/frSegStyle.h"
#include "db/obj/frAccess.h"
#include "frBaseTypes.h"
#include "global.h"

namespace drt {
class frNet;
class drNet : public drBlockObject
{
 public:
  // constructors
  drNet(frNet* net, RouterConfiguration* router_cfg) : fNet_(net)
  {
    if (hasNDR()) {
      maxRipupAvoids_ = router_cfg->NDR_NETS_RIPUP_HARDINESS;
    }
    if (isClockNetTrunk()) {
      maxRipupAvoids_ = std::max((int) maxRipupAvoids_,
                                 router_cfg->CLOCK_NETS_TRUNK_RIPUP_HARDINESS);
    } else if (isClockNetLeaf()) {
      maxRipupAvoids_ = std::max((int) maxRipupAvoids_,
                                 router_cfg->CLOCK_NETS_LEAF_RIPUP_HARDINESS);
    }
  }
  // getters
  const std::vector<std::unique_ptr<drPin>>& getPins() const { return pins_; }
  const std::vector<std::unique_ptr<drConnFig>>& getExtConnFigs() const
  {
    return extConnFigs_;
  }
  const std::vector<std::unique_ptr<drConnFig>>& getRouteConnFigs() const
  {
    return routeConnFigs_;
  }
  const std::vector<std::unique_ptr<drConnFig>>& getBestRouteConnFigs() const
  {
    return bestRouteConnFigs_;
  }
  void clearRouteConnFigs() { routeConnFigs_.clear(); }
  frNet* getFrNet() const { return fNet_; }
  void setFrNet(frNet* net) { fNet_ = net; }
  const std::set<frBlockObject*>& getFrNetTerms() const { return fNetTerms_; }
  bool isModified() const { return modified_; }
  int getNumMarkers() const { return numMarkers_; }
  int getNumPinsIn() const { return numPinsIn_; }
  bool hasMarkerDist() const { return (markerDist_ == -1); }
  frCoord getMarkerDist() const { return markerDist_; }
  odb::Rect getPinBox() { return pinBox_; }
  bool isRipup() const { return allowRipup_ ? ripup_ : false; }
  int getNumReroutes() const { return numReroutes_; }
  bool isInQueue() const { return inQueue_; }
  bool isRouted() const { return routed_; }
  const std::vector<frRect>& getOrigGuides() const { return origGuides_; }
  uint16_t getPriority() const { return priority_; }
  bool isFixed() const;
  // setters
  void incPriority()
  {
    if (priority_ < std::numeric_limits<uint16_t>::max()) {
      priority_++;
    }
  }
  void setPriority(uint16_t in) { priority_ = in; }
  void addPin(std::unique_ptr<drPin> pinIn)
  {
    pinIn->setNet(this);
    pinIn->setId(pins_.size());
    pins_.push_back(std::move(pinIn));
  }
  void addRoute(std::unique_ptr<drConnFig> in, bool isExt = false)
  {
    in->addToNet(this);
    if (isExt) {
      extConnFigs_.push_back(std::move(in));
    } else {
      routeConnFigs_.push_back(std::move(in));
    }
  }
  void setBestRouteConnFigs();
  void removeShape(drConnFig* shape, bool isExt = false);
  void clear()
  {
    routeConnFigs_.clear();
    modified_ = true;
    numMarkers_ = 0;
    routed_ = false;
    ext_figs_updates_.clear();
  }
  bool isClockNet() const;
  bool isClockNetTrunk() const
  {
    // TODO;
    return isClockNet();
  }
  bool isClockNetLeaf() const
  {
    // TODO;
    return false;
  }
  void setFrNetTerms(const std::set<frBlockObject*>& in) { fNetTerms_ = in; }
  void addFrNetTerm(frBlockObject* in) { fNetTerms_.insert(in); }
  void setModified(bool in) { modified_ = in; }

  void setNumMarkers(int in) { numMarkers_ = in; }
  void addMarker() { numMarkers_++; }
  void setNumPinsIn(int in) { numPinsIn_ = in; }
  void updateMarkerDist(frCoord in) { markerDist_ = std::min(markerDist_, in); }
  void resetMarkerDist() { markerDist_ = std::numeric_limits<frCoord>::max(); }
  void setPinBox(const odb::Rect& in) { pinBox_ = in; }
  void setRipup() { ripup_ = true; }
  void resetRipup() { ripup_ = false; }
  void setAllowRipup(bool in) { allowRipup_ = in; }
  void addNumReroutes() { numReroutes_++; }
  void resetNumReroutes() { numReroutes_ = 0; }
  void setInQueue() { inQueue_ = true; }
  void resetInQueue() { inQueue_ = false; }
  void setRouted() { routed_ = true; }
  void resetRouted() { routed_ = false; }
  void setOrigGuides(const std::vector<frRect>& in)
  {
    origGuides_.assign(in.begin(), in.end());
  }
  void cleanup();
  int getNRipupAvoids() const { return nRipupAvoids_; }
  void setNRipupAvoids(int n) { nRipupAvoids_ = n; }
  void incNRipupAvoids();
  bool hasNDR() const;
  // others
  frBlockObjectEnum typeId() const override { return drcNet; }

  bool operator<(const drNet& b) const
  {
    return (numMarkers_ == b.numMarkers_) ? (getId() < b.getId())
                                          : (numMarkers_ > b.numMarkers_);
  }
  bool canAvoidRipup() const { return nRipupAvoids_ < maxRipupAvoids_; }
  uint16_t getMaxRipupAvoids() const { return maxRipupAvoids_; }
  void setMaxRipupAvoids(uint16_t n) { maxRipupAvoids_ = n; }

  frAccessPoint* getFrAccessPoint(frCoord x,
                                  frCoord y,
                                  frLayerNum lNum,
                                  frBlockObject** owner = nullptr);
  void updateExtFigStyle(const Point3D& pt, const frSegStyle& style)
  {
    ext_figs_updates_[pt].is_via = false;
    ext_figs_updates_[pt].updated_style = style;
  }
  void updateExtFigConnected(const Point3D& pt,
                             const bool is_bottom_connected,
                             const bool is_top_connected)
  {
    ext_figs_updates_[pt].is_via = true;
    ext_figs_updates_[pt].is_bottom_connected = is_bottom_connected;
    ext_figs_updates_[pt].is_top_connected = is_top_connected;
  }
  bool hasExtFigUpdates() const { return !ext_figs_updates_.empty(); }
  std::vector<Point3D> getExtFigsUpdatesLocs() const
  {
    std::vector<Point3D> locs;
    locs.reserve(ext_figs_updates_.size());
    std::ranges::transform(ext_figs_updates_,
                           std::back_inserter(locs),
                           [](const auto& pair) { return pair.first; });
    return locs;
  }
  bool isExtFigUpdateVia(const Point3D& loc) const
  {
    return ext_figs_updates_.at(loc).is_via;
  }
  void getExtFigUpdate(const Point3D& loc, frSegStyle& style) const
  {
    style = ext_figs_updates_.at(loc).updated_style;
  }
  void getExtFigUpdate(const Point3D& loc,
                       bool& is_bottom_connected,
                       bool& is_top_connected) const
  {
    is_bottom_connected = ext_figs_updates_.at(loc).is_bottom_connected;
    is_top_connected = ext_figs_updates_.at(loc).is_top_connected;
  }

 private:
  drNet() = default;  // for serialization

  std::vector<std::unique_ptr<drPin>> pins_;
  std::vector<std::unique_ptr<drConnFig>> extConnFigs_;
  std::vector<std::unique_ptr<drConnFig>> routeConnFigs_;
  std::vector<std::unique_ptr<drConnFig>> bestRouteConnFigs_;
  std::set<frBlockObject*> fNetTerms_;
  frNet* fNet_{nullptr};
  // old
  bool modified_{false};
  int numMarkers_{0};
  int numPinsIn_{0};
  frCoord markerDist_{std::numeric_limits<frCoord>::max()};
  bool allowRipup_{true};
  odb::Rect pinBox_;
  bool ripup_{false};
  // new
  int numReroutes_{0};
  // the number of times this net avoided to be ripped up
  uint16_t nRipupAvoids_{0};
  uint16_t maxRipupAvoids_{0};
  bool inQueue_{false};
  bool routed_{false};

  std::vector<frRect> origGuides_;
  uint16_t priority_{0};
  struct ExtFigUpdate
  {
    frSegStyle updated_style;
    bool is_bottom_connected{false};
    bool is_top_connected{false};
    bool is_via{false};

   private:
    template <class Archive>
    void serialize(Archive& ar, unsigned int version);

    friend class boost::serialization::access;
  };
  std::map<Point3D, ExtFigUpdate> ext_figs_updates_;

  template <class Archive>
  void serialize(Archive& ar, unsigned int version);

  friend class boost::serialization::access;
};
}  // namespace drt
