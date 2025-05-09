// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "frBaseTypes.h"

namespace odb {
class Point;
class Rect;
}  // namespace odb

namespace drt {
using odb::Rect;
class frBlockObject;
class frDesign;
class frGuide;
class frMarker;
class frNet;
class frRect;
class frRPin;
class frShape;
class frVia;
class grBlockObject;
class grShape;
class grVia;
class FlexDR;
struct RouterConfiguration;

class frRegionQuery
{
 public:
  template <typename T>
  using Objects = std::vector<rq_box_value_t<T*>>;

  frRegionQuery(frDesign* design,
                Logger* logger,
                RouterConfiguration* router_cfg);
  ~frRegionQuery();
  // getters
  frDesign* getDesign() const;

  // setters
  void addDRObj(frShape* shape);
  void addDRObj(frVia* via);
  void addMarker(frMarker* in);
  void addGRObj(grShape* shape);
  void addGRObj(grVia* via);
  void addBlockObj(frBlockObject* obj);

  // Queries
  void query(const box_t& boostb,
             frLayerNum layerNum,
             Objects<frBlockObject>& result) const;
  void query(const Rect& box,
             frLayerNum layerNum,
             Objects<frBlockObject>& result) const;
  void queryGuide(const Rect& box,
                  frLayerNum layerNum,
                  Objects<frGuide>& result) const;
  void queryGuide(const Rect& box,
                  frLayerNum layerNum,
                  std::vector<frGuide*>& result) const;
  void queryGuide(const Rect& box, std::vector<frGuide*>& result) const;
  void queryOrigGuide(const Rect& box,
                      frLayerNum layerNum,
                      Objects<frNet>& result) const;
  void queryRPin(const Rect& box,
                 frLayerNum layerNum,
                 Objects<frRPin>& result) const;
  void queryGRPin(const Rect& box, std::vector<frBlockObject*>& result) const;
  void queryDRObj(const box_t& boostb,
                  frLayerNum layerNum,
                  Objects<frBlockObject>& result) const;
  void queryDRObj(const Rect& box,
                  frLayerNum layerNum,
                  Objects<frBlockObject>& result) const;
  void queryDRObj(const Rect& box,
                  frLayerNum layerNum,
                  std::vector<frBlockObject*>& result) const;
  void queryDRObj(const Rect& box, std::vector<frBlockObject*>& result) const;
  void queryGRObj(const Rect& box,
                  frLayerNum layerNum,
                  Objects<grBlockObject>& result) const;
  void queryGRObj(const Rect& box, std::vector<grBlockObject*>& result) const;
  void queryMarker(const Rect& box,
                   frLayerNum layerNum,
                   std::vector<frMarker*>& result) const;
  void queryMarker(const Rect& box, std::vector<frMarker*>& result) const;

  void clearGuides();
  void removeDRObj(frShape* shape);
  void removeDRObj(frVia* via);
  void removeGRObj(grShape* shape);
  void removeGRObj(grVia* via);
  void removeMarker(frMarker* in);
  void removeBlockObj(frBlockObject* obj);

  // init
  void init();
  void initGuide();
  void initOrigGuide(frOrderedIdMap<frNet*, std::vector<frRect>>& tmpGuides);
  void initGRPin(std::vector<std::pair<frBlockObject*, odb::Point>>& in);
  void initRPin();
  void initDRObj();
  void initGRObj();

  // utility
  void print();
  void printGuide();
  void printDRObj();
  void printGRObj();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  frRegionQuery();
  std::vector<std::pair<frBlockObject*, Rect>> getVias(frLayerNum layer_num);

  friend class FlexDR;
};
}  // namespace drt
