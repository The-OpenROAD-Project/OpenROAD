///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#pragma once

#include "ZException.h"
#include "dbObject.h"
#include "dbSet.h"
#include "dbTransform.h"
#include "dbWireCodec.h"
#include "geom.h"
#include "odb.h"
namespace utl {
class Logger;
}
namespace odb {

class _dbWire;
class dbBox;
class dbNet;
class dbMPin;
class dbBPin;
class dbObstruction;
class dbMTerm;
class dbMaster;
class dbInst;
class dbTechNonDefaultRule;
class dbTechLayer;
class dbWire;
class dbSWire;
class dbObstruction;

///////////////////////////////////////////////////////////////////////////////
///
/// dbShape
///
///////////////////////////////////////////////////////////////////////////////
class dbShape
{
  friend class dbHierInstShapeItr;

 public:
  enum Type
  {
    VIA,
    TECH_VIA,
    SEGMENT,
    TECH_VIA_BOX,
    VIA_BOX
  };

 private:
  Type _type;
  Rect _rect;
  dbTechLayer* _layer;
  dbObject* _via;

 public:
  dbShape() : _type(SEGMENT), _layer(NULL), _via(NULL) {}

  dbShape(dbVia* via, const Rect& r)
      : _type(VIA), _rect(r), _layer(NULL), _via((dbObject*) via)
  {
  }

  dbShape(dbTechVia* via, const Rect& r)
      : _type(TECH_VIA), _rect(r), _layer(NULL), _via((dbObject*) via)
  {
  }

  dbShape(dbTechLayer* layer, const Rect& r)
      : _type(SEGMENT), _rect(r), _layer(layer), _via(NULL)
  {
  }

  dbShape(dbTechVia* via, dbTechLayer* layer, const Rect& r)
      : _type(TECH_VIA_BOX), _rect(r), _layer(layer), _via((dbObject*) via)
  {
  }

  dbShape(dbVia* via, dbTechLayer* layer, const Rect& r)
      : _type(VIA_BOX), _rect(r), _layer(layer), _via((dbObject*) via)
  {
  }

  void setSegment(int prev_x,
                  int prev_y,
                  int prev_ext,
                  bool has_prev_ext,
                  int cur_x,
                  int cur_y,
                  int cur_ext,
                  bool has_cur_ext,
                  int dw,
                  dbTechLayer* layer);

  void setSegmentFromRect(int x1, int y1, int x2, int y2, dbTechLayer* layer);

  void setVia(dbVia* via, const Rect& r)
  {
    _type = VIA;
    _rect = r;
    _layer = NULL;
    _via = (dbObject*) via;
  }

  void setVia(dbTechVia* via, const Rect& r)
  {
    _type = TECH_VIA;
    _rect = r;
    _layer = NULL;
    _via = (dbObject*) via;
  }

  void setSegment(dbTechLayer* layer, const Rect& r)
  {
    _type = SEGMENT;
    _rect = r;
    _layer = layer;
    _via = NULL;
  }

  void setViaBox(dbTechVia* via, dbTechLayer* layer, const Rect& r)
  {
    _type = TECH_VIA_BOX;
    _rect = r;
    _layer = layer;
    _via = (dbObject*) via;
  }

  void setViaBox(dbVia* via, dbTechLayer* layer, const Rect& r)
  {
    _type = VIA_BOX;
    _rect = r;
    _layer = layer;
    _via = (dbObject*) via;
  }

  bool operator==(const dbShape& s)
  {
    return (_type == s._type) && (_rect == s._rect) && (_via == s._via)
           && (_layer == s._layer);
  }

  bool operator!=(const dbShape& s) { return !operator==(s); }

  bool operator<(const dbShape& rhs);

  ///
  /// Get the lower coordinate.
  ///
  int xMin() const;

  ///
  /// Get the lower y coordinate.
  ///
  int yMin() const;

  ///
  /// Get the high x coordinate.
  ///
  int xMax() const;

  ///
  /// Get the high y coordinate.
  ///
  int yMax() const;

  ///
  /// Get the placed coordinate of this via
  ///
  void getViaXY(int& x, int& y) const;

  ///
  /// Returns true if this object is a via
  ///
  bool isVia() const;

  ///
  /// Returns true if this object is a via box
  ///
  bool isViaBox() const;

  ///
  /// Returns the type this shape represents..
  ///
  Type getType() const;

  ///
  /// Get tech-via of this TECH_VIA.
  /// Returns NULL if this shape does not represent a tech-via
  ///
  dbTechVia* getTechVia() const;

  ///
  /// Get via of this VIA.
  /// Returns NULL if this shape does not represent a via
  ///
  dbVia* getVia() const;

  ///
  /// Get layer of this SEGMENT.
  /// Returns NULL if this shape does not represent a segment
  ///
  dbTechLayer* getTechLayer() const;

  ///
  /// Get the box bounding points.
  ///
  void getBox(Rect& rect) const;

  ///
  /// Get the width (xMax-xMin) of the box.
  ///
  uint getDX() const;

  ///
  /// Get the height (yMax-yMin) of the box.
  ///
  uint getDY() const;

  //
  //  Dump contents into logger
  //
  void dump(utl::Logger* logger, const char* group, int level) const;

  //
  // Get the via-boxes of this via-shape.
  //
  // WARNING: This method only works for shapes generated from a dbWire.
  //
  static void getViaBoxes(const dbShape& via, std::vector<dbShape>& boxes);
};

///
/// dbWireShapeItr - Iterate the shapes of a dbWire.
///
/// RECT in the dbWire are treats as segments for convenience
///
class dbWireShapeItr
{
 public:
  _dbWire* _wire;
  dbTech* _tech;
  dbBlock* _block;
  int _idx;
  int _prev_x;
  int _prev_y;
  int _prev_ext;
  bool _has_prev_ext;
  dbTechLayer* _layer;
  dbObject* _via;
  int _dw;
  int _point_cnt;
  int _shape_id;
  bool _has_width;

  unsigned char nextOp(int& value);
  unsigned char peekOp();

 public:
  dbWireShapeItr();
  ~dbWireShapeItr();

  void begin(dbWire* wire);
  bool next(dbShape& shape);
  int getShapeId();
};

///////////////////////////////////////////////////////////////////////////////
///
/// dbWirePathItr - Iterate the paths of a dbWire.
///
///////////////////////////////////////////////////////////////////////////////
struct dbWirePath
{
  int junction_id;     // junction id of this point
  Point point;         // starting point of path
  dbTechLayer* layer;  // starting layer of path
  dbBTerm* bterm;      // dbBTerm connected at this point, otherwise NULL
  dbITerm* iterm;      // dbITerm connected at this point, otherwise NULL
  bool is_branch;      // true if this path is a branch from the current tree
  bool is_short;  // true if this path is a virtual short to a previous junction
  int short_junction;          // junction id of the virtual short.
  dbTechNonDefaultRule* rule;  // nondefaultrule for this path
  void dump(utl::Logger* logger, const char* group, int level) const;
};

struct dbWirePathShape
{
  int junction_id;     // junction id of this point
  Point point;         // starting point of path
  dbTechLayer* layer;  // layer of shape, or exit layer of via
  dbBTerm* bterm;      // dbBTerm connected at this point, otherwise NULL
  dbITerm* iterm;      // dbITerm connected at this point, otherwise NULL
  dbShape shape;       // shape at this point

  void dump(utl::Logger* logger, const char* group, int level) const;
};

class dbWirePathItr
{
  dbWireDecoder _decoder;
  dbWireDecoder::OpCode _opcode;
  int _prev_x;
  int _prev_y;
  int _prev_ext;
  bool _has_prev_ext;
  dbWire* _wire;
  int _dw;
  dbTechNonDefaultRule* _rule;

  void getTerms(dbWirePathShape& s);

 public:
  dbWirePathItr();
  ~dbWirePathItr();

  void begin(dbWire* wire);
  bool getNextPath(dbWirePath& path);
  bool getNextShape(dbWirePathShape& shape);
};

///////////////////////////////////////////////////////////////////////////////
///
/// dbInstShapeItr
///
///////////////////////////////////////////////////////////////////////////////
class dbInstShapeItr
{
 public:
  enum IteratorType
  {
    ALL,
    PINS,
    OBSTRUCTIONS
  };

 private:
  dbSet<dbBox> _boxes;
  dbSet<dbMPin> _mpins;
  dbSet<dbMTerm> _mterms;
  dbSet<dbBox>::iterator _box_itr;
  dbSet<dbMTerm>::iterator _mterm_itr;
  dbSet<dbMPin>::iterator _mpin_itr;
  int _state;
  dbInst* _inst;
  dbMaster* _master;
  dbMPin* _mpin;
  dbTransform _transform;
  IteratorType _type;
  dbTechVia* _via;
  dbSet<dbBox> _via_boxes;
  dbSet<dbBox>::iterator _via_box_itr;
  int _via_x;
  int _via_y;
  bool _expand_vias;
  int _prev_state;

  void getShape(dbBox* box, dbShape& shape);
  void getViaBox(dbBox* box, dbShape& shape);

 public:
  dbInstShapeItr(bool expand_vias = false);
  void begin(dbInst* inst, IteratorType type);
  void begin(dbInst* inst, IteratorType type, const dbTransform& t);
  bool next(dbShape& shape);
};

///////////////////////////////////////////////////////////////////////////////
///
/// dbITermShapeItr
///
///////////////////////////////////////////////////////////////////////////////
class dbITermShapeItr
{
 private:
  dbSet<dbBox> _boxes;
  dbSet<dbMPin> _mpins;
  dbMTerm* _mterm;
  dbSet<dbBox>::iterator _box_itr;
  dbSet<dbMPin>::iterator _mpin_itr;
  int _state;
  dbITerm* _iterm;
  dbMPin* _mpin;
  dbTransform _transform;
  dbTechVia* _via;
  dbSet<dbBox> _via_boxes;
  dbSet<dbBox>::iterator _via_box_itr;
  int _via_x;
  int _via_y;
  bool _expand_vias;

  void getShape(dbBox* box, dbShape& shape);
  void getViaBox(dbBox* box, dbShape& shape);

 public:
  dbITermShapeItr(bool expand_vias = false);
  void begin(dbITerm* iterm);
  bool next(dbShape& shape);
};

class dbShapeItrCallback
{
 public:
  // Overide the following methods of interest...
  virtual void beginInst(dbInst*, int) {}
  virtual void endInst() {}

  virtual void beginObstruction(dbObstruction*) {}
  virtual void endObstruction() {}

  virtual void beginBPin(dbBPin*) {}
  virtual void endBPin() {}

  virtual void beginNet(dbNet*) {}
  virtual void endNet() {}

  virtual void beginWire(dbWire*) {}
  virtual void endWire() {}

  virtual void beginSWire(dbSWire*) {}
  virtual void endSWire() {}

  virtual void beginObstructions(dbMaster*) {}
  virtual void endObstructions() {}

  virtual void beginMTerm(dbMTerm*) {}
  virtual void endMTerm() {}

  virtual void beginMPin(dbMPin*) {}
  virtual void endMPin() {}

  // Called for all shapes except dbWire shapes.
  virtual bool nextBoxShape(dbBox*, dbShape&) { return true; }

  // Called only for dbWireShapes.
  virtual bool nextWireShape(dbWire*, int, dbShape&) { return true; }
};

class dbHierInstShapeItr
{
 public:
  enum Filter
  {
    NONE = 0,
    BLOCK_PIN = 0x00001,
    BLOCK_OBS = 0x00002,
    INST_OBS = 0x00004,
    INST_PIN = 0x00008,
    INST_VIA = 0x00010,
    SIGNAL_WIRE = 0x00020,
    SIGNAL_VIA = 0x00040,
    POWER_WIRE = 0x00080,
    POWER_VIA = 0x00100,
    CLOCK_WIRE = 0x00200,
    CLOCK_VIA = 0x00400,
    RESET_WIRE = 0x00800,
    RESET_VIA = 0x01000,
    NET_WIRE = 0x02000,
    NET_SWIRE = 0x04000,
    NET_WIRE_SHAPE = 0x08000,
    NET_SBOX = 0x10000
  };

 private:
  std::vector<dbTransform> _transforms;
  dbShapeItrCallback* _callback;

  bool drawNet(unsigned filter, dbNet* net, bool& draw_via, bool& draw_segment);
  void getShape(dbBox* box, dbShape& shape);
  void getViaBox(dbBox* box, dbShape& shape);
  void push_transform(Point origin, dbTransform& transform);
  void pop_transform();
  bool iterate_inst(dbInst* inst, unsigned filter, int level);
  bool iterate_swires(unsigned filter,
                      dbNet* net,
                      bool draw_vias,
                      bool draw_segments);
  bool iterate_swire(unsigned filter,
                     dbSWire* swire,
                     bool draw_vias,
                     bool draw_segments);
  bool iterate_wire(unsigned filter,
                    dbNet* net,
                    bool draw_vias,
                    bool draw_segments);
  bool iterate_leaf(dbInst* inst, unsigned filter, int level);
  void push_transform(dbTransform t);
  void transform(dbShape& shape);

 public:
  dbHierInstShapeItr(dbShapeItrCallback* callback);

  // filter = { PINS | OBSTRUCTIONS | ... }
  void iterate(dbInst* inst, unsigned filter = NONE);
};

/////////////////////////////////////////////////////////////////////////////////////////////
// inline functions
/////////////////////////////////////////////////////////////////////////////////////////////

inline int dbShape::xMin() const
{
  return _rect.xMin();
}
inline int dbShape::yMin() const
{
  return _rect.yMin();
}
inline int dbShape::xMax() const
{
  return _rect.xMax();
}
inline int dbShape::yMax() const
{
  return _rect.yMax();
}
inline bool dbShape::isVia() const
{
  return (_type == VIA) || (_type == TECH_VIA);
}
inline bool dbShape::isViaBox() const
{
  return (_type == VIA_BOX) || (_type == TECH_VIA_BOX);
}

inline dbShape::Type dbShape::getType() const
{
  return _type;
}

inline dbTechVia* dbShape::getTechVia() const
{
  if ((_type != TECH_VIA) && (_type != TECH_VIA_BOX))
    return NULL;

  return (dbTechVia*) _via;
}

inline dbVia* dbShape::getVia() const
{
  if ((_type != VIA) && (_type != VIA_BOX))
    return NULL;

  return (dbVia*) _via;
}

inline dbTechLayer* dbShape::getTechLayer() const
{
  return (dbTechLayer*) _layer;
}

inline void dbShape::getBox(Rect& rect) const
{
  rect = _rect;
}
inline uint dbShape::getDX() const
{
  return _rect.dx();
}
inline uint dbShape::getDY() const
{
  return _rect.dy();
}

inline void dbShape::setSegment(int prev_x,
                                int prev_y,
                                int prev_ext,
                                bool has_prev_ext,
                                int cur_x,
                                int cur_y,
                                int cur_ext,
                                bool has_cur_ext,
                                int dw,
                                dbTechLayer* layer)
{
  int x1, x2, y1, y2;
  if (cur_x == prev_x)  // vert. path
  {
    x1 = cur_x - dw;
    x2 = cur_x + dw;

    if (cur_y > prev_y) {
      if (has_prev_ext)
        y1 = prev_y - prev_ext;
      else
        y1 = prev_y - dw;

      if (has_cur_ext)
        y2 = cur_y + cur_ext;
      else
        y2 = cur_y + dw;
    } else if (cur_y < prev_y) {
      if (has_cur_ext)
        y1 = cur_y - cur_ext;
      else
        y1 = cur_y - dw;

      if (has_prev_ext)
        y2 = prev_y + prev_ext;
      else
        y2 = prev_y + dw;
    } else {
      y1 = cur_y - dw;
      y2 = cur_y + dw;
    }
  } else if (cur_y == prev_y)  // horiz. path
  {
    y1 = cur_y - dw;
    y2 = cur_y + dw;

    if (cur_x > prev_x) {
      if (has_prev_ext)
        x1 = prev_x - prev_ext;
      else
        x1 = prev_x - dw;

      if (has_cur_ext)
        x2 = cur_x + cur_ext;
      else
        x2 = cur_x + dw;
    } else if (cur_x < prev_x) {
      if (has_cur_ext)
        x1 = cur_x - cur_ext;
      else
        x1 = cur_x - dw;

      if (has_prev_ext)
        x2 = prev_x + prev_ext;
      else
        x2 = prev_x + dw;
    } else {
      x1 = cur_x - dw;
      x2 = cur_x + dw;
    }

  } else {
    x1 = 0;
    y1 = 0;
    x2 = 0;
    y2 = 0;
    assert(0);  // illegal: non-orthogonal-path
  }

  _type = dbShape::SEGMENT;
  _rect.reset(x1, y1, x2, y2);
  _layer = layer;
  _via = NULL;
}

inline void dbShape::setSegmentFromRect(int x1,
                                        int y1,
                                        int x2,
                                        int y2,
                                        dbTechLayer* layer)
{
  _type = dbShape::SEGMENT;
  _rect.reset(x1, y1, x2, y2);
  _layer = layer;
  _via = nullptr;
}

//
// Print utilities declared here
//
void dumpWirePaths4Net(dbNet* innet, const char* group, int level);

}  // namespace odb
