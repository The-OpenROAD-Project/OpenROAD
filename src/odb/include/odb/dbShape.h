// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"

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

  dbShape() = default;

  dbShape(dbVia* via, const Rect& r)
      : type_(VIA), rect_(r), via_((dbObject*) via)
  {
  }

  dbShape(dbTechVia* via, const Rect& r)
      : type_(TECH_VIA), rect_(r), via_((dbObject*) via)
  {
  }

  dbShape(dbTechLayer* layer, const Rect& r) : rect_(r), layer_(layer) {}

  dbShape(dbTechVia* via, dbTechLayer* layer, const Rect& r)
      : type_(TECH_VIA_BOX), rect_(r), layer_(layer), via_((dbObject*) via)
  {
  }

  dbShape(dbVia* via, dbTechLayer* layer, const Rect& r)
      : type_(VIA_BOX), rect_(r), layer_(layer), via_((dbObject*) via)
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
                  int default_ext,
                  dbTechLayer* layer);

  void setSegmentFromRect(int x1, int y1, int x2, int y2, dbTechLayer* layer);

  void setVia(dbVia* via, const Rect& r)
  {
    type_ = VIA;
    rect_ = r;
    layer_ = nullptr;
    via_ = (dbObject*) via;
  }

  void setVia(dbTechVia* via, const Rect& r)
  {
    type_ = TECH_VIA;
    rect_ = r;
    layer_ = nullptr;
    via_ = (dbObject*) via;
  }

  void setSegment(dbTechLayer* layer, const Rect& r)
  {
    type_ = SEGMENT;
    rect_ = r;
    layer_ = layer;
    via_ = nullptr;
  }

  void setViaBox(dbTechVia* via, dbTechLayer* layer, const Rect& r)
  {
    type_ = TECH_VIA_BOX;
    rect_ = r;
    layer_ = layer;
    via_ = (dbObject*) via;
  }

  void setViaBox(dbVia* via, dbTechLayer* layer, const Rect& r)
  {
    type_ = VIA_BOX;
    rect_ = r;
    layer_ = layer;
    via_ = (dbObject*) via;
  }

  bool operator==(const dbShape& s)
  {
    return (type_ == s.type_) && (rect_ == s.rect_) && (via_ == s.via_)
           && (layer_ == s.layer_);
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
  /// Return the placed location of this via.
  ///
  Point getViaXY() const;

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
  /// Returns nullptr if this shape does not represent a tech-via
  ///
  dbTechVia* getTechVia() const;

  ///
  /// Get via of this VIA.
  /// Returns nullptr if this shape does not represent a via
  ///
  dbVia* getVia() const;

  ///
  /// Get layer of this SEGMENT.
  /// Returns nullptr if this shape does not represent a segment
  ///
  dbTechLayer* getTechLayer() const;

  ///
  /// Get the box bounding points.
  ///
  Rect getBox() const;

  ///
  /// Get the width (xMax-xMin) of the box.
  ///
  uint32_t getDX() const;

  ///
  /// Get the height (yMax-yMin) of the box.
  ///
  uint32_t getDY() const;

  ///
  /// Get the length of the box
  ///
  int getLength() const;

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

 private:
  Type type_ = SEGMENT;
  Rect rect_;
  dbTechLayer* layer_ = nullptr;
  dbObject* via_ = nullptr;
};

///
/// dbWireShapeItr - Iterate the shapes of a dbWire.
///
/// RECT in the dbWire are treats as segments for convenience
///
class dbWireShapeItr
{
 public:
  dbWireShapeItr();

  void begin(dbWire* wire);
  bool next(dbShape& shape);
  int getShapeId();

  unsigned char nextOp(int& value);
  unsigned char peekOp();

  _dbWire* wire_;
  dbTech* tech_;
  dbBlock* block_;
  int idx_;
  int prev_x_;
  int prev_y_;
  int prev_ext_;
  bool has_prev_ext_;
  dbTechLayer* layer_;
  dbObject* via_;
  int dw_;
  int point_cnt_;
  int shape_id_;
  bool has_width_;
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
  dbBTerm* bterm;      // dbBTerm connected at this point, otherwise nullptr
  dbITerm* iterm;      // dbITerm connected at this point, otherwise nullptr
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
  dbBTerm* bterm;      // dbBTerm connected at this point, otherwise nullptr
  dbITerm* iterm;      // dbITerm connected at this point, otherwise nullptr
  dbShape shape;       // shape at this point

  void dump(utl::Logger* logger, const char* group, int level) const;
};

class dbWirePathItr
{
 public:
  dbWirePathItr();

  void begin(dbWire* wire);
  bool getNextPath(dbWirePath& path);
  bool getNextShape(dbWirePathShape& shape);

 private:
  void getTerms(dbWirePathShape& s);

  dbWireDecoder decoder_;
  dbWireDecoder::OpCode opcode_;
  int prev_x_;
  int prev_y_;
  int prev_ext_;
  bool has_prev_ext_;
  dbWire* wire_;
  int dw_;
  dbTechNonDefaultRule* rule_;
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

  dbInstShapeItr(bool expand_vias = false);
  void begin(dbInst* inst, IteratorType type);
  void begin(dbInst* inst, IteratorType type, const dbTransform& t);
  bool next(dbShape& shape);

 private:
  enum State
  {
    kInit = 0,
    kMtermItr = 1,
    kMpinItr = 2,
    kMboxItr = 3,
    kObsItr = 4,
    kViaBoxItr = 5,
    kPinsDone = 6
  };
  void getShape(dbBox* box, dbShape& shape);
  void getViaBox(dbBox* box, dbShape& shape);

  dbSet<dbBox> boxes_;
  dbSet<dbMPin> mpins_;
  dbSet<dbMTerm> mterms_;
  dbSet<dbBox>::iterator box_itr_;
  dbSet<dbMTerm>::iterator mterm_itr_;
  dbSet<dbMPin>::iterator mpin_itr_;
  State state_;
  dbInst* inst_;
  dbMaster* master_;
  dbMPin* _mpin_;
  dbTransform transform_;
  IteratorType type_;
  dbTechVia* via_;
  dbSet<dbBox> via_boxes_;
  dbSet<dbBox>::iterator via_box_itr_;
  Point via_pt_;
  bool expand_vias_;
  State prev_state_;
};

///////////////////////////////////////////////////////////////////////////////
///
/// dbITermShapeItr
///
///////////////////////////////////////////////////////////////////////////////
class dbITermShapeItr
{
 public:
  dbITermShapeItr(bool expand_vias = false);
  void begin(dbITerm* iterm);
  bool next(dbShape& shape);

 private:
  enum State
  {
    kInit,
    kMpinItr,
    kMboxItr,
    kViaBoxItr
  };

  void getShape(dbBox* box, dbShape& shape);
  void getViaBox(dbBox* box, dbShape& shape);

  dbSet<dbBox> boxes_;
  dbSet<dbMPin> mpins_;
  dbMTerm* mterm_;
  dbSet<dbBox>::iterator box_itr_;
  dbSet<dbMPin>::iterator mpin_itr_;
  State state_;
  dbITerm* iterm_;
  dbMPin* mpin_;
  dbTransform transform_;
  dbTechVia* via_;
  dbSet<dbBox> via_boxes_;
  dbSet<dbBox>::iterator via_box_itr_;
  Point via_pt_;
  bool expand_vias_;
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

  dbHierInstShapeItr(dbShapeItrCallback* callback);

  // filter = { PINS | OBSTRUCTIONS | ... }
  void iterate(dbInst* inst, unsigned filter = NONE);

 private:
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
  void push_transform(const dbTransform& t);
  void transform(dbShape& shape);

  std::vector<dbTransform> transforms_;
  dbShapeItrCallback* callback_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// inline functions
/////////////////////////////////////////////////////////////////////////////////////////////

inline int dbShape::xMin() const
{
  return rect_.xMin();
}
inline int dbShape::yMin() const
{
  return rect_.yMin();
}
inline int dbShape::xMax() const
{
  return rect_.xMax();
}
inline int dbShape::yMax() const
{
  return rect_.yMax();
}
inline bool dbShape::isVia() const
{
  return (type_ == VIA) || (type_ == TECH_VIA);
}
inline bool dbShape::isViaBox() const
{
  return (type_ == VIA_BOX) || (type_ == TECH_VIA_BOX);
}

inline dbShape::Type dbShape::getType() const
{
  return type_;
}

inline dbTechVia* dbShape::getTechVia() const
{
  if ((type_ != TECH_VIA) && (type_ != TECH_VIA_BOX)) {
    return nullptr;
  }

  return (dbTechVia*) via_;
}

inline dbVia* dbShape::getVia() const
{
  if ((type_ != VIA) && (type_ != VIA_BOX)) {
    return nullptr;
  }

  return (dbVia*) via_;
}

inline dbTechLayer* dbShape::getTechLayer() const
{
  return (dbTechLayer*) layer_;
}

inline Rect dbShape::getBox() const
{
  return rect_;
}

inline uint32_t dbShape::getDX() const
{
  return rect_.dx();
}

inline uint32_t dbShape::getDY() const
{
  return rect_.dy();
}

inline int dbShape::getLength() const
{
  return std::abs(rect_.dx() - rect_.dy());
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
                                int default_ext,
                                dbTechLayer* layer)
{
  int x1, x2, y1, y2;
  if (cur_x == prev_x)  // vert. path
  {
    x1 = cur_x - dw;
    x2 = cur_x + dw;

    if (cur_y > prev_y) {
      if (has_prev_ext) {
        y1 = prev_y - prev_ext;
      } else {
        y1 = prev_y - default_ext;
      }

      if (has_cur_ext) {
        y2 = cur_y + cur_ext;
      } else {
        y2 = cur_y + default_ext;
      }
    } else if (cur_y < prev_y) {
      if (has_cur_ext) {
        y1 = cur_y - cur_ext;
      } else {
        y1 = cur_y - default_ext;
      }

      if (has_prev_ext) {
        y2 = prev_y + prev_ext;
      } else {
        y2 = prev_y + default_ext;
      }
    } else {
      y1 = cur_y - dw;
      y2 = cur_y + dw;
    }
  } else if (cur_y == prev_y)  // horiz. path
  {
    y1 = cur_y - dw;
    y2 = cur_y + dw;

    if (cur_x > prev_x) {
      if (has_prev_ext) {
        x1 = prev_x - prev_ext;
      } else {
        x1 = prev_x - default_ext;
      }

      if (has_cur_ext) {
        x2 = cur_x + cur_ext;
      } else {
        x2 = cur_x + default_ext;
      }
    } else if (cur_x < prev_x) {
      if (has_cur_ext) {
        x1 = cur_x - cur_ext;
      } else {
        x1 = cur_x - default_ext;
      }

      if (has_prev_ext) {
        x2 = prev_x + prev_ext;
      } else {
        x2 = prev_x + default_ext;
      }
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

  type_ = dbShape::SEGMENT;
  rect_.reset(x1, y1, x2, y2);
  layer_ = layer;
  via_ = nullptr;
}

inline void dbShape::setSegmentFromRect(int x1,
                                        int y1,
                                        int x2,
                                        int y2,
                                        dbTechLayer* layer)
{
  type_ = dbShape::SEGMENT;
  rect_.reset(x1, y1, x2, y2);
  layer_ = layer;
  via_ = nullptr;
}

//
// Print utilities declared here
//
void dumpWirePaths4Net(dbNet* innet, const char* group, int level);

}  // namespace odb
