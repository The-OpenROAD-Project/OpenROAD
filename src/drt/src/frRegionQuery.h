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

#pragma once

#include "frBaseTypes.h"

namespace odb {
class Point;
class Rect;
}  // namespace odb

namespace drt {
using odb::Rect;
class frBlockObject;
struct frBlockObjectComp;
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
  void initOrigGuide(
      std::map<frNet*, std::vector<frRect>, frBlockObjectComp>& tmpGuides);
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
  bool verbose_;

  frRegionQuery();
  std::vector<std::pair<frBlockObject*, Rect>> getVias(frLayerNum layer_num);

  friend class FlexDR;
};
}  // namespace drt
