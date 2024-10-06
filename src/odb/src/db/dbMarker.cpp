///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Generator Code Begin Cpp
#include "dbMarker.h"

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbITerm.h"
#include "dbInst.h"
#include "dbMarkerCategory.h"
#include "dbMarkerGroup.h"
#include "dbNet.h"
#include "dbObstruction.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbVector.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbMarker>;

bool _dbMarker::operator==(const _dbMarker& rhs) const
{
  if (_parent != rhs._parent) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_layer != rhs._layer) {
    return false;
  }
  if (_comment != rhs._comment) {
    return false;
  }
  if (_line_number != rhs._line_number) {
    return false;
  }

  // User Code Begin ==
  if (shapes_ != rhs.shapes_) {
    return false;
  }
  // User Code End ==
  return true;
}

bool _dbMarker::operator<(const _dbMarker& rhs) const
{
  return true;
}

void _dbMarker::differences(dbDiff& diff,
                            const char* field,
                            const _dbMarker& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_parent);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_layer);
  DIFF_FIELD(_comment);
  DIFF_FIELD(_line_number);
  DIFF_END
}

void _dbMarker::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_parent);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_layer);
  DIFF_OUT_FIELD(_comment);
  DIFF_OUT_FIELD(_line_number);

  DIFF_END
}

_dbMarker::_dbMarker(_dbDatabase* db)
{
  _line_number = -1;
}

_dbMarker::_dbMarker(_dbDatabase* db, const _dbMarker& r)
{
  _parent = r._parent;
  _next_entry = r._next_entry;
  _layer = r._layer;
  _comment = r._comment;
  _line_number = r._line_number;
  // User Code Begin CopyConstructor
  shapes_ = r.shapes_;
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbMarker& obj)
{
  stream >> obj._parent;
  stream >> obj._next_entry;
  stream >> obj._layer;
  stream >> obj._member_insts;
  stream >> obj._member_nets;
  stream >> obj._member_obstructions;
  stream >> obj._member_iterms;
  stream >> obj._member_bterms;
  stream >> obj._comment;
  stream >> obj._line_number;
  // User Code Begin >>
  // handle shapes
  std::size_t shapes;
  stream >> shapes;
  for (std::size_t i = 0; i < shapes; i++) {
    int type;
    stream >> type;

    switch (type) {
      case 0: {
        Point pt;
        stream >> pt;
        obj.shapes_.push_back(pt);
        break;
      }
      case 1: {
        Line l;
        stream >> l;
        obj.shapes_.push_back(l);
        break;
      }
      case 2: {
        Rect r;
        stream >> r;
        obj.shapes_.push_back(r);
        break;
      }
      case 3: {
        Polygon p;
        stream >> p;
        obj.shapes_.push_back(p);
        break;
      }
    }
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbMarker& obj)
{
  stream << obj._parent;
  stream << obj._next_entry;
  stream << obj._layer;
  stream << obj._member_insts;
  stream << obj._member_nets;
  stream << obj._member_obstructions;
  stream << obj._member_iterms;
  stream << obj._member_bterms;
  stream << obj._comment;
  stream << obj._line_number;
  // User Code Begin <<
  // handle shapes
  stream << obj.shapes_.size();
  for (const dbMarker::MarkerShape& shape : obj.shapes_) {
    if (std::holds_alternative<Point>(shape)) {
      stream << 0;  // write 0 for Point
      stream << std::get<Point>(shape);
    } else if (std::holds_alternative<Line>(shape)) {
      stream << 1;  // write 1 for Line
      stream << std::get<Line>(shape);
    } else if (std::holds_alternative<Rect>(shape)) {
      stream << 2;  // write 2 for Rect
      stream << std::get<Rect>(shape);
    } else {
      stream << 3;  // write 3 for Polygon
      stream << std::get<Polygon>(shape);
    }
  }
  // User Code End <<
  return stream;
}

// User Code Begin PrivateMethods

_dbBlock* _dbMarker::getBlock() const
{
  dbMarker* marker = (dbMarker*) this;
  _dbMarkerGroup* group = (_dbMarkerGroup*) marker->getGroup();
  return (_dbBlock*) group->getOwner();
}

// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbMarker - Methods
//
////////////////////////////////////////////////////////////////////

void dbMarker::setComment(const std::string& comment)
{
  _dbMarker* obj = (_dbMarker*) this;

  obj->_comment = comment;
}

std::string dbMarker::getComment() const
{
  _dbMarker* obj = (_dbMarker*) this;
  return obj->_comment;
}

void dbMarker::setLineNumber(int line_number)
{
  _dbMarker* obj = (_dbMarker*) this;

  obj->_line_number = line_number;
}

int dbMarker::getLineNumber() const
{
  _dbMarker* obj = (_dbMarker*) this;
  return obj->_line_number;
}

// User Code Begin dbMarkerPublicMethods

dbMarkerGroup* dbMarker::getGroup() const
{
  return getCategory()->getGroup();
}

dbMarkerCategory* dbMarker::getCategory() const
{
  _dbMarker* marker = (_dbMarker*) this;
  return (dbMarkerCategory*) marker->getOwner();
}

std::vector<dbMarker::MarkerShape> dbMarker::getShapes() const
{
  _dbMarker* marker = (_dbMarker*) this;
  return marker->shapes_;
}

void dbMarker::addShape(const Point& pt)
{
  _dbMarker* marker = (_dbMarker*) this;
  marker->shapes_.push_back(pt);
}

void dbMarker::addShape(const Line& line)
{
  _dbMarker* marker = (_dbMarker*) this;
  marker->shapes_.push_back(line);
}

void dbMarker::addShape(const Rect& rect)
{
  _dbMarker* marker = (_dbMarker*) this;
  marker->shapes_.push_back(rect);
}

void dbMarker::addShape(const Polygon& polygon)
{
  _dbMarker* marker = (_dbMarker*) this;
  marker->shapes_.push_back(polygon);
}

void dbMarker::setTechLayer(dbTechLayer* layer)
{
  _dbMarker* marker = (_dbMarker*) this;
  if (layer == nullptr) {
    marker->_layer = 0;
  } else {
    _dbTechLayer* _layer = (_dbTechLayer*) layer;
    marker->_layer = _layer->getId();
  }
}

void dbMarker::addNet(dbNet* net)
{
  if (net == nullptr) {
    return;
  }
  _dbMarker* marker = (_dbMarker*) this;
  _dbNet* _net = (_dbNet*) net;
  marker->_member_nets.push_back(_net->getId());
}

void dbMarker::addInst(dbInst* inst)
{
  if (inst == nullptr) {
    return;
  }
  _dbMarker* marker = (_dbMarker*) this;
  _dbInst* _inst = (_dbInst*) inst;
  marker->_member_insts.push_back(_inst->getId());
}

void dbMarker::addITerm(dbITerm* iterm)
{
  if (iterm == nullptr) {
    return;
  }
  _dbMarker* marker = (_dbMarker*) this;
  _dbITerm* _iterm = (_dbITerm*) iterm;
  marker->_member_iterms.push_back(_iterm->getId());
}

void dbMarker::addBTerm(dbBTerm* bterm)
{
  if (bterm == nullptr) {
    return;
  }
  _dbMarker* marker = (_dbMarker*) this;
  _dbBTerm* _bterm = (_dbBTerm*) bterm;
  marker->_member_bterms.push_back(_bterm->getId());
}

void dbMarker::addObstruction(dbObstruction* obs)
{
  if (obs == nullptr) {
    return;
  }
  _dbMarker* marker = (_dbMarker*) this;
  _dbObstruction* _obs = (_dbObstruction*) obs;
  marker->_member_obstructions.push_back(_obs->getId());
}

dbTechLayer* dbMarker::getTechLayer() const
{
  _dbMarker* marker = (_dbMarker*) this;
  if (marker->_layer == 0) {
    return nullptr;
  }

  dbBlock* block = (dbBlock*) marker->getBlock();
  _dbTech* tech = (_dbTech*) block->getTech();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(marker->_layer);
}

Rect dbMarker::getBBox() const
{
  Rect bbox;
  bbox.mergeInit();

  for (const MarkerShape& shape : getShapes()) {
    if (std::holds_alternative<Point>(shape)) {
      Point pt = std::get<Point>(shape);
      bbox.merge(Rect(pt, pt));
    } else if (std::holds_alternative<Line>(shape)) {
      for (const Point& pt : std::get<Line>(shape).getPoints()) {
        bbox.merge(Rect(pt, pt));
      }
    } else if (std::holds_alternative<Rect>(shape)) {
      bbox.merge(std::get<Rect>(shape));
    } else {
      bbox.merge(std::get<Polygon>(shape).getEnclosingRect());
    }
  }

  return bbox;
}

std::set<dbNet*> dbMarker::getNets() const
{
  _dbMarker* marker = (_dbMarker*) this;
  _dbBlock* block = marker->getBlock();

  std::set<dbNet*> nets;
  for (dbId<_dbNet>& id : marker->_member_nets) {
    if (block->_net_tbl->validId(id)) {
      nets.insert((dbNet*) block->_net_tbl->getPtr(id));
    }
  }
  return nets;
}

std::set<dbInst*> dbMarker::getInsts() const
{
  _dbMarker* marker = (_dbMarker*) this;
  _dbBlock* block = marker->getBlock();

  std::set<dbInst*> insts;
  for (dbId<_dbInst>& id : marker->_member_insts) {
    if (block->_inst_tbl->validId(id)) {
      insts.insert((dbInst*) block->_inst_tbl->getPtr(id));
    }
  }
  return insts;
}

std::set<dbObstruction*> dbMarker::getObstructions() const
{
  _dbMarker* marker = (_dbMarker*) this;
  _dbBlock* block = marker->getBlock();

  std::set<dbObstruction*> obstructions;
  for (dbId<_dbObstruction>& id : marker->_member_obstructions) {
    if (block->_obstruction_tbl->validId(id)) {
      obstructions.insert((dbObstruction*) block->_obstruction_tbl->getPtr(id));
    }
  }
  return obstructions;
}

std::set<dbITerm*> dbMarker::getITerms() const
{
  _dbMarker* marker = (_dbMarker*) this;
  _dbBlock* block = marker->getBlock();

  std::set<dbITerm*> iterms;
  for (dbId<_dbITerm>& id : marker->_member_iterms) {
    if (block->_iterm_tbl->validId(id)) {
      iterms.insert((dbITerm*) block->_iterm_tbl->getPtr(id));
    }
  }
  return iterms;
}

std::set<dbBTerm*> dbMarker::getBTerms() const
{
  _dbMarker* marker = (_dbMarker*) this;
  _dbBlock* block = marker->getBlock();

  std::set<dbBTerm*> bterms;
  for (dbId<_dbBTerm>& id : marker->_member_bterms) {
    if (block->_bterm_tbl->validId(id)) {
      bterms.insert((dbBTerm*) block->_bterm_tbl->getPtr(id));
    }
  }
  return bterms;
}

dbMarker* dbMarker::create(dbMarkerCategory* category)
{
  _dbMarkerCategory* _category = (_dbMarkerCategory*) category;

  _dbMarker* marker = _category->marker_tbl_->create();

  return (dbMarker*) marker;
}

void dbMarker::destroy(dbMarker* marker)
{
  _dbMarker* _marker = (_dbMarker*) marker;
  _dbMarkerCategory* category = (_dbMarkerCategory*) _marker->getOwner();
  category->marker_tbl_->destroy(_marker);
}

// User Code End dbMarkerPublicMethods
}  // namespace odb
// Generator Code End Cpp