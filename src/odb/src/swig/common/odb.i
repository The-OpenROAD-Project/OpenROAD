// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#ifdef BAZEL
%module(package="src.odb") odb
#endif

%{
#define SWIG_FILE_WITH_INIT
#include "odb/geom.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbViaParams.h"
#include "odb/dbWireCodec.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbIterator.h"
#include "odb/dbTransform.h"
#include "odb/dbWireGraph.h"
#include "odb/dbMap.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/isotropy.h"
#include "odb/geom.h"
#include "odb/wOrder.h"
#include "odb/util.h"

using namespace odb;
%}

%include <stl.i>
%include <typemaps.i>
%include <std_string.i>
%include <std_vector.i>
%include <std_pair.i>
%include <std_map.i>

%typemap(in) (uint) = (int);
%typemap(out) (uint) = (int);
%typemap(in) (uint32_t) = (int);
%typemap(out) (uint32_t) = (int);
%typemap(out) (uint64) = (long);
%typemap(out) (int64_t) = (long);
%apply int* OUTPUT {int* x, int* y, int& ext};

%ignore odb::dbTechLayerAntennaRule::pwl_pair;
%ignore odb::dbTechLayerAntennaRule::getDiffPAR() const;
%ignore odb::dbTechLayerAntennaRule::getDiffCAR() const;
%ignore odb::dbTechLayerAntennaRule::getDiffPSR() const;
%ignore odb::dbTechLayerAntennaRule::getDiffCSR() const;
%ignore odb::dbTechLayerAntennaRule::getAreaDiffReduce() const;
%ignore odb::dbTechLayer::getArea() const;
%ignore odb::dbTechLayer::setArea(double area);

%extend odb::dbTechLayer {
  int64_t getArea() const
  {
    if ($self == nullptr) {
      return 0;
    }
    odb::dbTech* tech = $self->getTech();
    if (tech == nullptr) {
      return 0;
    }
    const int dbu_per_micron = tech->getDbUnitsPerMicron();
    if (dbu_per_micron <= 0) {
      return 0;
    }
    const int64_t dbu_per_square_micron
        = static_cast<int64_t>(dbu_per_micron) * dbu_per_micron;
    const double dbu_area = $self->getArea() * dbu_per_square_micron;
    return static_cast<int64_t>(std::round(dbu_area));
  }

  void setArea(int64_t area)
  {
    if ($self == nullptr) {
      return;
    }
    odb::dbTech* tech = $self->getTech();
    if (tech == nullptr) {
      return;
    }
    const int dbu_per_micron = tech->getDbUnitsPerMicron();
    if (dbu_per_micron <= 0) {
      return;
    }
    const int64_t dbu_per_square_micron
        = static_cast<int64_t>(dbu_per_micron) * dbu_per_micron;
    $self->setArea(static_cast<double>(area) / dbu_per_square_micron);
  }
}

// Swig can't handle non-assignable types
%ignore odb::Point::get(Orientation2D orient) const;
%ignore odb::Rect::low(Orientation2D orient) const;
%ignore odb::Rect::high(Orientation2D orient) const;
%ignore odb::Rect::get(Orientation2D orient, Direction1D dir) const;
%ignore odb::Rect::set(Orientation2D orient, Direction1D dir, int value);
%ignore odb::Point::set(Orientation2D orient, int value);
%ignore odb::Rect::bloat(int margin, Orientation2D orient) const;

%ignore odb::dbGDSStructure::operator[];

%include "dbenums.i"
%include "parserenums.i"
%include "dbtypes.i"
%include "dbtypes_common.i"

%include "odb/isotropy.h"
%include "odb/geom.h"
%include "polygon.i"
%include "odb/db.h"

// Prevent compiler problems when including dbShape.h.
%ignore odb::dbShapeItrCallback;
%ignore odb::dbWireShapeItr;

%include "odb/dbShape.h"

%include "dbhelpers.i"  

%rename(getPoint_ext) odb::dbWireDecoder::getPoint(int& x, int& y, int& ext) const;

%include "odb/dbViaParams.h"
%include "odb/dbWireCodec.h"
%include "odb/dbBlockCallBackObj.h"
%include "odb/dbIterator.h"
%include "odb/dbTransform.h"
%include "odb/dbWireGraph.h"
%include "odb/dbSet.h"
%include "odb/wOrder.h"

std::string generateMacroPlacementString(odb::dbBlock* block);
void set_bterm_top_layer_grid(odb::dbBlock* block,
                              odb::dbTechLayer* layer,
                              int x_step,
                              int y_step,
                              odb::Rect region,
                              int width,
                              int height,
                              int keepout);
