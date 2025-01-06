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

#pragma once

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/polygon/polygon.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include "db/drObj/drMarker.h"
#include "db/drObj/drNet.h"
#include "db/drObj/drPin.h"
#include "db/gcObj/gcNet.h"
#include "db/gcObj/gcPin.h"
#include "db/gcObj/gcShape.h"
#include "db/infra/frBox.h"
#include "db/obj/frMarker.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "distributed/drUpdate.h"
#include "distributed/paUpdate.h"
#include "frDesign.h"
#include "global.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace gtl = boost::polygon;
namespace bg = boost::geometry;

namespace boost::serialization {

// Enable serialization of a std::tuple using recursive templates.
// For some reason boost serialize seems to leave out this std class.
template <unsigned int N>
struct TupleSerializer
{
  template <class Archive, typename... Types>
  static void serialize(Archive& ar,
                        std::tuple<Types...>& t,
                        const unsigned int version)
  {
    (ar) & std::get<N - 1>(t);
    TupleSerializer<N - 1>::serialize(ar, t, version);
  }
};

template <>
struct TupleSerializer<0>
{
  template <class Archive, typename... Types>
  static void serialize(Archive& /* ar */,
                        std::tuple<Types...>& /* t */,
                        const unsigned int /* version */)
  {
  }
};

template <class Archive, typename... Types>
void serialize(Archive& ar, std::tuple<Types...>& t, const unsigned int version)
{
  TupleSerializer<sizeof...(Types)>::serialize(ar, t, version);
}

// Sadly boost polygon lacks serializers so here are some home grown ones.
template <class Archive>
void serialize(Archive& ar,
               gtl::point_data<drt::frCoord>& point,
               const unsigned int version)
{
  if (drt::is_loading(ar)) {
    drt::frCoord x = 0;
    drt::frCoord y = 0;
    (ar) & x;
    (ar) & y;
    point.x(x);
    point.y(y);
  } else {
    drt::frCoord x = point.x();
    drt::frCoord y = point.y();
    (ar) & x;
    (ar) & y;
  }
}

template <class Archive>
void serialize(Archive& ar,
               gtl::interval_data<drt::frCoord>& interval,
               const unsigned int version)
{
  if (drt::is_loading(ar)) {
    drt::frCoord low = 0;
    drt::frCoord high = 0;
    (ar) & low;
    (ar) & high;
    interval.low(low);
    interval.high(high);
  } else {
    auto low = interval.low();
    auto high = interval.high();
    (ar) & low;
    (ar) & high;
  }
}

template <class Archive>
void serialize(Archive& ar,
               gtl::segment_data<drt::frCoord>& segment,
               const unsigned int version)
{
  if (drt::is_loading(ar)) {
    gtl::point_data<drt::frCoord> low;
    gtl::point_data<drt::frCoord> high;
    (ar) & low;
    (ar) & high;
    segment.low(low);
    segment.high(high);
  } else {
    gtl::point_data<drt::frCoord> low = segment.low();
    gtl::point_data<drt::frCoord> high = segment.high();
    (ar) & low;
    (ar) & high;
  }
}

template <class Archive>
void serialize(Archive& ar,
               gtl::rectangle_data<drt::frCoord>& rect,
               const unsigned int version)
{
  if (drt::is_loading(ar)) {
    gtl::interval_data<drt::frCoord> h;
    gtl::interval_data<drt::frCoord> v;
    (ar) & h;
    (ar) & v;
    rect.set(gtl::HORIZONTAL, h);
    rect.set(gtl::VERTICAL, v);
  } else {
    auto h = rect.get(gtl::HORIZONTAL);
    auto v = rect.get(gtl::VERTICAL);
    (ar) & h;
    (ar) & v;
  }
}

template <class Archive>
void serialize(Archive& ar,
               gtl::polygon_90_data<drt::frCoord>& polygon,
               const unsigned int version)
{
  if (drt::is_loading(ar)) {
    std::vector<drt::frCoord> coordinates;
    (ar) & coordinates;
    polygon.set_compact(coordinates.begin(), coordinates.end());
  } else {
    std::vector<drt::frCoord> coordinates(polygon.begin_compact(),
                                          polygon.end_compact());
    (ar) & coordinates;
  }
}

template <class Archive>
void serialize(Archive& ar,
               gtl::polygon_90_with_holes_data<drt::frCoord>& polygon,
               const unsigned int version)
{
  if (drt::is_loading(ar)) {
    gtl::polygon_90_data<drt::frCoord> outside;
    (ar) & outside;
    polygon.set(outside.begin(), outside.end());

    std::list<gtl::polygon_90_data<drt::frCoord>> holes;
    (ar) & holes;
    polygon.set_holes(holes.begin(), holes.end());
  } else {
    gtl::polygon_90_data<drt::frCoord> outside;
    outside.set(polygon.begin(), polygon.end());
    (ar) & outside;
    std::list<gtl::polygon_90_data<drt::frCoord>> holes(polygon.begin_holes(),
                                                        polygon.end_holes());
    (ar) & holes;
  }
}

template <class Archive>
void serialize(Archive& ar,
               gtl::polygon_90_set_data<drt::frCoord>& polygon_set,
               const unsigned int version)
{
  std::vector<gtl::polygon_90_with_holes_data<drt::frCoord>> polygons;
  if (drt::is_loading(ar)) {
    (ar) & polygons;
    polygon_set.insert(polygons.begin(), polygons.end());
  } else {
    polygon_set.get_polygons(polygons);
    (ar) & polygons;
  }
}

// Sadly boost geometry lacks serializers so here are some home grown ones.
template <class Archive>
void serialize(Archive& ar, drt::point_t& point, const unsigned int version)
{
  if (drt::is_loading(ar)) {
    drt::frCoord x = 0;
    drt::frCoord y = 0;
    (ar) & x;
    (ar) & y;
    point.x(x);
    point.y(y);
  } else {
    drt::frCoord x = point.x();
    drt::frCoord y = point.y();
    (ar) & x;
    (ar) & y;
  }
}

template <class Archive>
void serialize(Archive& ar, drt::segment_t& segment, const unsigned int version)
{
  if (drt::is_loading(ar)) {
    drt::frCoord xl = 0;
    drt::frCoord xh = 0;
    drt::frCoord yl = 0;
    drt::frCoord yh = 0;
    (ar) & xl;
    (ar) & xh;
    (ar) & yl;
    (ar) & yh;
    bg::set<0, 0>(segment, xl);
    bg::set<0, 1>(segment, xh);
    bg::set<1, 0>(segment, yl);
    bg::set<1, 1>(segment, yh);
  } else {
    drt::frCoord xl = bg::get<0, 0>(segment);
    drt::frCoord xh = bg::get<0, 1>(segment);
    drt::frCoord yl = bg::get<1, 0>(segment);
    drt::frCoord yh = bg::get<1, 1>(segment);
    (ar) & xl;
    (ar) & xh;
    (ar) & yl;
    (ar) & yh;
  }
}

// odb classes
template <class Archive>
void serialize(Archive& ar, odb::Rect& r, const unsigned int version)
{
  if (drt::is_loading(ar)) {
    drt::frCoord xlo = 0, ylo = 0, xhi = 0, yhi = 0;
    (ar) & xlo;
    (ar) & ylo;
    (ar) & xhi;
    (ar) & yhi;
    r.reset(xlo, ylo, xhi, yhi);
  } else {
    drt::frCoord xlo, ylo, xhi, yhi;
    xlo = r.xMin();
    ylo = r.yMin();
    xhi = r.xMax();
    yhi = r.yMax();
    (ar) & xlo;
    (ar) & ylo;
    (ar) & xhi;
    (ar) & yhi;
  }
}

template <class Archive>
void serialize(Archive& ar, odb::Point& p, const unsigned int version)
{
  if (drt::is_loading(ar)) {
    drt::frCoord x = 0, y = 0;
    (ar) & x;
    (ar) & y;
    p = {x, y};
  } else {
    drt::frCoord x, y;
    x = p.x();
    y = p.y();
    (ar) & x;
    (ar) & y;
  }
}

template <class Archive>
void serialize(Archive& ar, odb::dbSigType& type, const unsigned int version)
{
  odb::dbSigType::Value v = odb::dbSigType::SIGNAL;
  if (drt::is_loading(ar)) {
    (ar) & v;
    type = odb::dbSigType(v);
  } else {
    v = type.getValue();
    (ar) & v;
  }
}

template <class Archive>
void serialize(Archive& ar, odb::dbIoType& type, const unsigned int version)
{
  odb::dbIoType::Value v = odb::dbIoType::INOUT;
  if (drt::is_loading(ar)) {
    (ar) & v;
    type = odb::dbIoType(v);
  } else {
    v = type.getValue();
    (ar) & v;
  }
}

template <class Archive>
void serialize(Archive& ar,
               odb::dbTechLayerType& type,
               const unsigned int version)
{
  odb::dbTechLayerType::Value v = odb::dbTechLayerType::NONE;
  if (drt::is_loading(ar)) {
    (ar) & v;
    type = odb::dbTechLayerType(v);
  } else {
    v = type.getValue();
    (ar) & v;
  }
}

template <class Archive>
void serialize(Archive& ar, odb::dbMasterType& type, const unsigned int version)
{
  odb::dbMasterType::Value v = odb::dbMasterType::CORE;
  if (drt::is_loading(ar)) {
    (ar) & v;
    type = odb::dbMasterType(v);
  } else {
    v = type.getValue();
    (ar) & v;
  }
}

template <class Archive>
void serialize(Archive& ar,
               odb::dbTechLayerDir& type,
               const unsigned int version)
{
  odb::dbTechLayerDir::Value v = odb::dbTechLayerDir::NONE;
  if (drt::is_loading(ar)) {
    (ar) & v;
    type = odb::dbTechLayerDir(v);
  } else {
    v = type.getValue();
    (ar) & v;
  }
}

template <class Archive>
void serialize(Archive& ar, odb::dbOrientType& type, const unsigned int version)
{
  odb::dbOrientType::Value v = odb::dbOrientType::R0;
  if (drt::is_loading(ar)) {
    (ar) & v;
    type = odb::dbOrientType(v);
  } else {
    v = type.getValue();
    (ar) & v;
  }
}

template <class Archive>
void serialize(Archive& ar,
               odb::dbTransform& transform,
               const unsigned int version)
{
  odb::dbOrientType type = odb::dbOrientType::R0;
  odb::Point offset;
  if (drt::is_loading(ar)) {
    (ar) & type;
    (ar) & offset;
    transform.setOrient(type);
    transform.setOffset(offset);
  } else {
    type = transform.getOrient();
    offset = transform.getOffset();
    (ar) & type;
    (ar) & offset;
  }
}

}  // namespace boost::serialization

namespace drt {

template <class Archive>
void registerTypes(Archive& ar)
{
  // The serialization library needs to be told about these classes
  // as we often only encounter them through their base classes.
  // More details here
  // https://www.boost.org/doc/libs/1_76_0/libs/serialization/doc/serialization.html#derivedpointers

  ar.template register_type<drUpdate>();
  ar.template register_type<paUpdate>();
  ar.template register_type<frRect>();
  ar.template register_type<frPathSeg>();
  ar.template register_type<frPatchWire>();
  ar.template register_type<frPolygon>();

  ar.template register_type<drPathSeg>();
  ar.template register_type<drVia>();
  ar.template register_type<drPatchWire>();

  ar.template register_type<drMazeMarker>();
  ar.template register_type<drNet>();
  ar.template register_type<drPin>();

  ar.template register_type<frMarker>();
  ar.template register_type<frVia>();
  ar.template register_type<frBox3D>();

  ar.template register_type<frPinAccess>();
  ar.template register_type<frAccessPoint>();
}

inline bool inBounds(int id, int sz)
{
  return id >= 0 && id < sz;
}
template <class Archive>
void serializeBlockObject(Archive& ar, frBlockObject*& obj)
{
  frDesign* design = ar.getDesign();
  if (is_loading(ar)) {
    obj = nullptr;
    frBlockObjectEnum type = frcBlock;
    (ar) & type;
    switch (type) {
      case frcNet: {
        bool fake = false;
        bool special = false;
        int id = -1;
        bool modified = false;
        (ar) & fake;
        (ar) & special;
        (ar) & id;
        (ar) & modified;
        if (fake) {
          if (id == 0) {
            obj = design->getTopBlock()->getFakeVSSNet();
          } else {
            obj = design->getTopBlock()->getFakeVDDNet();
          }
        } else {
          if (special) {
            obj = design->getTopBlock()->getSNet(id);
          } else {
            obj = design->getTopBlock()->getNet(id);
          }
        }
        if (obj != nullptr && modified) {
          ((frNet*) obj)->setModified(true);
        }
        break;
      }
      case frcBTerm: {
        int id = -1;
        (ar) & id;
        if (!inBounds(id, design->getTopBlock()->getTerms().size())) {
          exit(1);  // should throw error
        }
        obj = design->getTopBlock()->getTerms().at(id).get();
        break;
      }
      case frcBlockage: {
        int id = -1;
        (ar) & id;
        if (!inBounds(id, design->getTopBlock()->getBlockages().size())) {
          exit(1);
        }
        obj = design->getTopBlock()->getBlockages().at(id).get();
        break;
      }
      case frcInst: {
        int inst_id = -1;
        (ar) & inst_id;
        if (!inBounds(inst_id, design->getTopBlock()->getInsts().size())) {
          exit(1);
        }
        obj = design->getTopBlock()->getInsts().at(inst_id).get();
        break;
      }
      case frcInstTerm: {
        int inst_id = 0;
        int id = 0;
        (ar) & inst_id;
        (ar) & id;
        if (!inBounds(inst_id, design->getTopBlock()->getInsts().size())) {
          exit(1);
        }
        auto inst = design->getTopBlock()->getInsts().at(inst_id).get();
        if (!inBounds(id, inst->getInstTerms().size())) {
          exit(1);
        }
        obj = inst->getInstTerms().at(id).get();
        break;
      }
      case frcInstBlockage: {
        int inst_id = 0;
        int id = 0;
        (ar) & inst_id;
        (ar) & id;
        if (!inBounds(inst_id, design->getTopBlock()->getInsts().size())) {
          exit(1);
        }
        auto inst = design->getTopBlock()->getInsts().at(inst_id).get();
        if (!inBounds(id, inst->getInstBlockages().size())) {
          exit(1);
        }
        obj = inst->getInstBlockages().at(id).get();
        break;
      }
      case frcMaster: {
        int id = 0;
        (ar) & id;
        id--;
        if (!inBounds(id, design->getMasters().size())) {
          exit(1);
        }
        obj = design->getMasters().at(id).get();
        break;
      }
      case frcMTerm: {
        frBlockObject* blockObj;
        serializeBlockObject(ar, blockObj);
        frMaster* master = (frMaster*) blockObj;
        int id = 0;
        (ar) & id;
        if (!inBounds(id, master->getTerms().size())) {
          std::cout << "frcMTerm" << std::endl;
          exit(1);
        }
        obj = master->getTerms().at(id).get();
        break;
      }
      case frcMPin: {
        frBlockObject* blockObj;
        serializeBlockObject(ar, blockObj);
        frMTerm* term = (frMTerm*) blockObj;
        int id = 0;
        (ar) & id;
        if (!inBounds(id, term->getPins().size())) {
          std::cout << "frcMPin" << std::endl;
          exit(1);
        }
        obj = term->getPins().at(id).get();
        break;
      }
      case frcBPin: {
        frBlockObject* blockObj;
        serializeBlockObject(ar, blockObj);
        frBTerm* term = (frBTerm*) blockObj;
        int id = 0;
        (ar) & id;
        if (!inBounds(id, term->getPins().size())) {
          std::cout << "frcBPin" << std::endl;
          exit(1);
        }
        obj = term->getPins().at(id).get();
        break;
      }
      case frcPinAccess: {
        frBlockObject* blockObj;
        serializeBlockObject(ar, blockObj);
        frPin* pin = (frPin*) blockObj;
        int id = 0;
        (ar) & id;
        if (!inBounds(id, pin->getNumPinAccess())) {
          std::cout << "frcPinAccess" << std::endl;
          exit(1);
        }
        obj = pin->getPinAccess(id);
        break;
      }
      case frcAccessPoint: {
        frBlockObject* blockObj;
        serializeBlockObject(ar, blockObj);
        frPinAccess* pa = (frPinAccess*) blockObj;
        int id = 0;
        (ar) & id;
        if (!inBounds(id, pa->getAccessPoints().size())) {
          std::cout << "frcAccessPoint " << id << std::endl;
          exit(1);
        }
        obj = pa->getAccessPoints().at(id).get();
        break;
      }
      case frcBlock:
        break;
      default:
        exit(1);
        break;
    }
  } else {
    frBlockObjectEnum type;
    if (obj != nullptr) {
      type = obj->typeId();
    } else {
      type = frcBlock;
    }
    (ar) & type;
    switch (type) {
      case frcNet: {
        bool fake = ((frNet*) obj)->isFake();
        bool special = ((frNet*) obj)->isSpecial();
        int id = ((frNet*) obj)->getId();
        bool modified = ((frNet*) obj)->isModified();
        (ar) & fake;
        (ar) & special;
        if (fake) {
          if (((frNet*) obj)->getType() == odb::dbSigType::GROUND) {
            id = 0;
          } else {
            id = 1;
          }
        }
        (ar) & id;
        (ar) & modified;
        break;
      }
      case frcBTerm: {
        int id = ((frBTerm*) obj)->getIndexInOwner();
        (ar) & id;
        break;
      }
      case frcBlockage: {
        int id = ((frBlockage*) obj)->getIndexInOwner();
        (ar) & id;
        break;
      }
      case frcInst: {
        int inst_id = ((frInst*) obj)->getId();
        (ar) & inst_id;
        break;
      }
      case frcInstTerm: {
        int inst_id = ((frInstTerm*) obj)->getInst()->getId();
        int id = ((frInstTerm*) obj)->getIndexInOwner();
        (ar) & inst_id;
        (ar) & id;
        break;
      }
      case frcInstBlockage: {
        int inst_id = ((frInstBlockage*) obj)->getInst()->getId();
        int id = ((frInstBlockage*) obj)->getIndexInOwner();
        (ar) & inst_id;
        (ar) & id;
        break;
      }
      case frcMaster: {
        int id = ((frMaster*) obj)->getId();
        (ar) & id;
        break;
      }
      case frcMTerm: {
        frBlockObject* master = ((frMTerm*) obj)->getMaster();
        serializeBlockObject(ar, master);
        int id = ((frMTerm*) obj)->getIndexInOwner();
        (ar) & id;
        break;
      }
      case frcMPin: {
        frBlockObject* mterm = ((frMPin*) obj)->getTerm();
        serializeBlockObject(ar, mterm);
        int id = ((frMPin*) obj)->getId();
        (ar) & id;
        break;
      }
      case frcBPin: {
        frBlockObject* bterm = ((frBPin*) obj)->getTerm();
        serializeBlockObject(ar, bterm);
        int id = ((frBPin*) obj)->getId();
        (ar) & id;
        break;
      }
      case frcPinAccess: {
        frBlockObject* pin = ((frPinAccess*) obj)->getPin();
        serializeBlockObject(ar, pin);
        int id = ((frPinAccess*) obj)->getId();
        (ar) & id;
        break;
      }
      case frcAccessPoint: {
        frBlockObject* pa = ((frAccessPoint*) obj)->getPinAccess();
        serializeBlockObject(ar, pa);
        int id = ((frAccessPoint*) obj)->getId();
        (ar) & id;
        break;
      }
      case frcBlock:
        break;
      default:
        exit(1);
        break;
    }
  }
}

template <class Archive>
void serializeViaDef(Archive& ar, const frViaDef*& viadef)
{
  frDesign* design = ar.getDesign();
  if (is_loading(ar)) {
    int via_id = -1;
    (ar) & via_id;
    if (via_id >= 0) {
      viadef = design->getTech()->getVias().at(via_id).get();
    } else {
      viadef = nullptr;
    }
  } else {
    int via_id;
    if (viadef != nullptr) {
      via_id = viadef->getId();
    } else {
      via_id = -1;
    }
    (ar) & via_id;
  }
}

template <class Archive>
void serializeGlobals(Archive& ar, RouterConfiguration* router_cfg)
{
  (ar) & router_cfg->DBPROCESSNODE;
  (ar) & router_cfg->OUT_MAZE_FILE;
  (ar) & router_cfg->DRC_RPT_FILE;
  (ar) & router_cfg->CMAP_FILE;
  (ar) & router_cfg->OR_SEED;
  (ar) & router_cfg->OR_K;
  (ar) & router_cfg->MAX_THREADS;
  (ar) & router_cfg->BATCHSIZE;
  (ar) & router_cfg->BATCHSIZETA;
  (ar) & router_cfg->MTSAFEDIST;
  (ar) & router_cfg->DRCSAFEDIST;
  (ar) & router_cfg->VERBOSE;
  (ar) & router_cfg->BOTTOM_ROUTING_LAYER_NAME;
  (ar) & router_cfg->TOP_ROUTING_LAYER_NAME;
  (ar) & router_cfg->BOTTOM_ROUTING_LAYER;
  (ar) & router_cfg->TOP_ROUTING_LAYER;
  (ar) & router_cfg->ALLOW_PIN_AS_FEEDTHROUGH;
  (ar) & router_cfg->USENONPREFTRACKS;
  (ar) & router_cfg->USEMINSPACING_OBS;
  (ar) & router_cfg->ENABLE_BOUNDARY_MAR_FIX;
  (ar) & router_cfg->ENABLE_VIA_GEN;
  (ar) & router_cfg->VIAINPIN_BOTTOMLAYER_NAME;
  (ar) & router_cfg->VIAINPIN_TOPLAYER_NAME;
  (ar) & router_cfg->VIAINPIN_BOTTOMLAYERNUM;
  (ar) & router_cfg->VIAINPIN_TOPLAYERNUM;
  (ar) & router_cfg->VIA_ACCESS_LAYERNUM;
  (ar) & router_cfg->MINNUMACCESSPOINT_MACROCELLPIN;
  (ar) & router_cfg->MINNUMACCESSPOINT_STDCELLPIN;
  (ar) & router_cfg->ACCESS_PATTERN_END_ITERATION_NUM;
  (ar) & router_cfg->END_ITERATION;
  (ar) & router_cfg->NDR_NETS_RIPUP_HARDINESS;
  (ar) & router_cfg->CLOCK_NETS_TRUNK_RIPUP_HARDINESS;
  (ar) & router_cfg->CLOCK_NETS_LEAF_RIPUP_HARDINESS;
  (ar) & router_cfg->AUTO_TAPER_NDR_NETS;
  (ar) & router_cfg->TAPERBOX_RADIUS;
  (ar) & router_cfg->NDR_NETS_ABS_PRIORITY;
  (ar) & router_cfg->CLOCK_NETS_ABS_PRIORITY;
  (ar) & router_cfg->TAPINCOST;
  (ar) & router_cfg->TAALIGNCOST;
  (ar) & router_cfg->TADRCCOST;
  (ar) & router_cfg->TASHAPEBLOATWIDTH;
  (ar) & router_cfg->VIACOST;
  (ar) & router_cfg->GRIDCOST;
  (ar) & router_cfg->ROUTESHAPECOST;
  (ar) & router_cfg->MARKERCOST;
  (ar) & router_cfg->MARKERBLOATWIDTH;
  (ar) & router_cfg->BLOCKCOST;
  (ar) & router_cfg->GUIDECOST;
  (ar) & router_cfg->SHAPEBLOATWIDTH;
  (ar) & router_cfg->HISTCOST;
  (ar) & router_cfg->CONGCOST;
}

}  // namespace drt
