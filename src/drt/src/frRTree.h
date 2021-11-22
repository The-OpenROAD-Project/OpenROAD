/* Authors: Matt Liberty */
/*
 * Copyright (c) 2020, The Regents of the University of California
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

#ifndef _FR_RTREE_H_
#define _FR_RTREE_H_

#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>

#include "db/infra/frBox.h"
#include "db/infra/frPoint.h"
#include "serialization.h"

namespace bgi = boost::geometry::index;

// Enable frPoint & frBox to be used with boost geometry
BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(fr::frPoint,
                                         fr::frCoord,
                                         cs::cartesian,
                                         x,
                                         y,
                                         setX,
                                         setY)

BOOST_GEOMETRY_REGISTER_BOX(fr::frBox, fr::frPoint, lowerLeft(), upperRight())

namespace fr {

template <typename T, typename Key = frBox>
using RTree = bgi::rtree<rq_box_value_t<T, Key>, bgi::quadratic<16>>;

// I tried to get the 'experimental' boost rtree serializer code to work
// without success.  That really is the nicer solution but such is life.
//
// So instead save the objects in the rtree as a vector and rebuild
// the rtree on load.  However this produces a tree which may have a
// different internal structure and therefore return its results in a
// different order.  To solve this you have to sort the query results
// which is unfortunate as it costs runtime.  If you can be certain
// that the ordering doesn't matter then you can skip the sort.

template <class Archive, typename T, typename Key>
void save(Archive& ar, const RTree<T, Key>& tree, const unsigned int version)
{
  std::vector<rq_box_value_t<T, Key>> objects(tree.begin(), tree.end());
  (ar) & objects;
}

template <class Archive, typename T, typename Key>
void load(Archive& ar, RTree<T, Key>& tree, const unsigned int version)
{
  std::vector<rq_box_value_t<T, Key>> objects;
  (ar) & objects;
  tree = boost::move(RTree<T, Key>(objects));
}

template <class Archive, typename T, typename Key>
void serialize(Archive& ar, RTree<T, Key>& tree, const unsigned int version)
{
  // calls save or load depending on the archive
  boost::serialization::split_free(ar, tree, version);
}

}  // namespace fr

#endif
