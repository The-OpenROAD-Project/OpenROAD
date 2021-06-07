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

#ifndef _FR_REGIONQUERY_H_
#define _FR_REGIONQUERY_H_

#include "frBaseTypes.h"

namespace fr {
class frBlockObject;
struct frBlockObjectComp;
class frDesign;
class frGuide;
class frMarker;
class frNet;
class frPoint;
class frRect;
class frRPin;
class frShape;
class frVia;
class grBlockObject;
class grShape;
class grVia;

class frRegionQuery
{
 public:
  template <typename T>
  using Objects = std::vector<rq_box_value_t<T*>>;

  frRegionQuery(frDesign* design, Logger* logger);
  ~frRegionQuery();
  // getters
  frDesign* getDesign() const;

  // setters
  void addDRObj(frShape* in);
  void addDRObj(frVia* in);
  void addMarker(frMarker* in);
  void addGRObj(grShape* in);
  void addGRObj(grVia* in);

  // Queries
  void query(const frBox& box,
             const frLayerNum layerNum,
             Objects<frBlockObject>& result) const;
  void queryGuide(const frBox& box,
                  const frLayerNum layerNum,
                  Objects<frGuide>& result) const;
  void queryGuide(const frBox& box,
                  const frLayerNum layerNum,
                  std::vector<frGuide*>& result) const;
  void queryGuide(const frBox& box, std::vector<frGuide*>& result) const;
  void queryOrigGuide(const frBox& box,
                      const frLayerNum layerNum,
                      Objects<frNet>& result) const;
  void queryRPin(const frBox& box,
                 const frLayerNum layerNum,
                 Objects<frRPin>& result) const;
  void queryGRPin(const frBox& box, std::vector<frBlockObject*>& result) const;
  void queryDRObj(const frBox& box,
                  const frLayerNum layerNum,
                  Objects<frBlockObject>& result) const;
  void queryDRObj(const frBox& box,
                  const frLayerNum layerNum,
                  std::vector<frBlockObject*>& result) const;
  void queryDRObj(const frBox& box, std::vector<frBlockObject*>& result) const;
  void queryGRObj(const frBox& box,
                  const frLayerNum layerNum,
                  Objects<grBlockObject>& result) const;
  void queryGRObj(const frBox& box, std::vector<grBlockObject*>& result) const;
  void queryMarker(const frBox& box,
                   const frLayerNum layerNum,
                   std::vector<frMarker*>& result) const;
  void queryMarker(const frBox& box, std::vector<frMarker*>& result) const;

  void clearGuides();
  void removeDRObj(frShape* in);
  void removeDRObj(frVia* in);
  void removeGRObj(grShape* in);
  void removeGRObj(grVia* in);
  void removeMarker(frMarker* in);

  // init
  void init();
  void initGuide();
  void initOrigGuide(
      std::map<frNet*, std::vector<frRect>, frBlockObjectComp>& tmpGuides);
  void initGRPin(std::vector<std::pair<frBlockObject*, frPoint>>& in);
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

  // We will have to use explicit instantiation because the impl is private
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  frRegionQuery();  // for serialization

  friend class boost::serialization::access;
};
}  // namespace fr

#endif
