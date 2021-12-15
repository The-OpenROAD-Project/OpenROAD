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

#ifndef _GC_PIN_H
#define _GC_PIN_H

#include "db/gcObj/gcShape.h"

namespace fr {
class gcNet;
class gcPin : public gcBlockObject
{
 public:
  // constructors
  gcPin()
      : gcBlockObject(),
        polygon_(nullptr),
        net_(nullptr) /*, dirty_(false)*/,
        polygon_edges_(),
        polygon_corners_(),
        max_rectangles_()
  {
  }
  gcPin(const gtl::polygon_90_with_holes_data<frCoord>& shapeIn,
        frLayerNum layerNumIn,
        gcNet* netIn)
      : gcBlockObject(),
        polygon_(std::make_unique<gcPolygon>(shapeIn, layerNumIn, this, netIn)),
        net_(netIn) /*, dirty_(true)*/,
        polygon_edges_(),
        polygon_corners_(),
        max_rectangles_()
  {
  }
  // setters
  void setNet(gcNet* in) { net_ = in; }
  void addPolygonEdges(std::vector<std::unique_ptr<gcSegment>>& in)
  {
    polygon_edges_.push_back(std::move(in));
  }
  void addPolygonCorners(std::vector<std::unique_ptr<gcCorner>>& in)
  {
    polygon_corners_.push_back(std::move(in));
  }
  void addMaxRectangle(std::unique_ptr<gcRect> in)
  {
    max_rectangles_.push_back(std::move(in));
  }

  // getters
  gcPolygon* getPolygon() const { return polygon_.get(); }
  const std::vector<std::vector<std::unique_ptr<gcSegment>>>& getPolygonEdges()
      const
  {
    return polygon_edges_;
  }
  const std::vector<std::vector<std::unique_ptr<gcCorner>>>& getPolygonCorners()
      const
  {
    return polygon_corners_;
  }
  const std::vector<std::unique_ptr<gcRect>>& getMaxRectangles() const
  {
    return max_rectangles_;
  }

  gcNet* getNet() { return net_; }
  // others
  frBlockObjectEnum typeId() const override { return gccPin; }

 private:
  std::unique_ptr<gcPolygon> polygon_;
  gcNet* net_;
  // assisting structures
  std::vector<std::vector<std::unique_ptr<gcSegment>>> polygon_edges_;
  std::vector<std::vector<std::unique_ptr<gcCorner>>> polygon_corners_;
  std::vector<std::unique_ptr<gcRect>> max_rectangles_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<gcBlockObject>(*this);
    (ar) & polygon_;
    (ar) & net_;
    (ar) & polygon_edges_;
    (ar) & polygon_corners_;
    (ar) & max_rectangles_;
  }

  friend class boost::serialization::access;
};
}  // namespace fr

#endif
