// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/dbWireCodec.h"

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <optional>

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayerRule.h"
#include "dbVector.h"
#include "dbWire.h"
#include "dbWireOpcode.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {

//////////////////////////////////////////////////////////////////////////////////
//
// dbWireEncoder
//
//////////////////////////////////////////////////////////////////////////////////

#define DB_WIRE_ENCODER_INVALID_VIA_LAYER 0

inline unsigned char dbWireEncoder::getOp(int idx)
{
  return opcodes_[idx];
}

inline void dbWireEncoder::updateOp(int idx, unsigned char op)
{
  opcodes_[idx] = op;
}

inline void dbWireEncoder::addOp(unsigned char op, int value)
{
  data_.push_back(value);
  opcodes_.push_back(op);
  ++idx_;
}

inline unsigned char dbWireEncoder::getWireType(dbWireType type)
{
  unsigned char wire_type = 0;

  switch (type.getValue()) {
    case dbWireType::NONE:
      wire_type = kNone;
      break;

    case dbWireType::COVER:
      wire_type = kCover;
      break;

    case dbWireType::FIXED:
      wire_type = kFixed;
      break;

    case dbWireType::ROUTED:
      wire_type = kRouted;
      break;

    case dbWireType::NOSHIELD:
      wire_type = kNoShield;
      break;

    case dbWireType::SHIELD: {
      utl::Logger* logger = wire_->getImpl()->getLogger();
      logger->error(
          utl::ODB, 1113, "Shield type should not occur on a wire segment");
    } break;
  }

  return wire_type;
}

inline void dbWireEncoder::initPath(dbTechLayer* layer, dbWireType type)
{
  initPath(layer, getWireType(type));
}

inline void dbWireEncoder::initPath(dbTechLayer* layer,
                                    dbWireType type,
                                    dbTechLayerRule* rule)
{
  initPath(layer, getWireType(type), rule);
}

void dbWireEncoder::initPath(dbTechLayer* layer, unsigned char wire_type)
{
  wire_type_ = wire_type;
  point_cnt_ = 0;
  via_cnt_ = 0;
  layer_ = layer;
  non_default_rule_ = 0;
  rule_opcode_ = 0;
  prev_extended_colinear_pnt_ = false;
}

void dbWireEncoder::initPath(dbTechLayer* layer,
                             unsigned char wire_type,
                             dbTechLayerRule* rule)
{
  wire_type_ = wire_type;
  point_cnt_ = 0;
  via_cnt_ = 0;
  layer_ = layer;
  non_default_rule_ = rule->getImpl()->getOID();

  if (rule->isBlockRule()) {
    rule_opcode_ = kRule | WOP_BLOCK_RULE;
  } else {
    rule_opcode_ = kRule;
  }

  prev_extended_colinear_pnt_ = false;
  x_ = 0;
  y_ = 0;
}

dbWireEncoder::dbWireEncoder()
    : wire_(nullptr),
      tech_(nullptr),
      block_(nullptr),
      layer_(nullptr),
      idx_(0),
      x_(0),
      y_(0),
      non_default_rule_(0),
      point_cnt_(0),
      via_cnt_(0),
      prev_extended_colinear_pnt_(false),
      wire_type_(0),
      rule_opcode_(0)
{
}

void dbWireEncoder::begin(dbWire* wire)
{
  clear();
  wire_ = (_dbWire*) wire;
  block_ = wire->getBlock();
  tech_ = block_->getTech();
}

void dbWireEncoder::clear()
{
  wire_ = nullptr;
  block_ = nullptr;
  tech_ = nullptr;
  data_.clear();
  opcodes_.clear();
  layer_ = nullptr;
  idx_ = 0;
  x_ = 0;
  y_ = 0;
  non_default_rule_ = 0;
  rule_opcode_ = 0;
  point_cnt_ = 0;
  via_cnt_ = 0;
  prev_extended_colinear_pnt_ = false;
}

void dbWireEncoder::append(dbWire* wire)
{
  wire_ = (_dbWire*) wire;
  block_ = wire->getBlock();
  tech_ = block_->getDb()->getTech();
  data_ = wire_->data_;
  opcodes_ = wire_->opcodes_;
  layer_ = nullptr;
  idx_ = data_.size();
  x_ = 0;
  y_ = 0;
  non_default_rule_ = 0;
  rule_opcode_ = 0;
  point_cnt_ = 0;
  via_cnt_ = 0;
  prev_extended_colinear_pnt_ = false;
}

#define DB_WIRE_ENCODER_NON_ORTHOGANAL_SEGMENT 0

int dbWireEncoder::addPoint(int x, int y, uint32_t property)
{
  int jct_id = idx_;

  if (non_default_rule_ == 0) {
    if (point_cnt_ == 0) {
      addOp(kX | WOP_DEFAULT_WIDTH, x);
      addOp(kY | WOP_DEFAULT_WIDTH, y);
      x_ = x;
      y_ = y;
      jct_id++;
      point_cnt_++;
    } else if ((x_ == x) && (y_ == y)) {
      addOp(kColinear | WOP_DEFAULT_WIDTH, 0);
    } else if (y_ == y) {
      addOp(kX | WOP_DEFAULT_WIDTH, x);
      x_ = x;
      point_cnt_++;
    } else if (x_ == x) {
      addOp(kY | WOP_DEFAULT_WIDTH, y);
      y_ = y;
      point_cnt_++;
    } else {
      assert(DB_WIRE_ENCODER_NON_ORTHOGANAL_SEGMENT);
    }
  } else {
    if (point_cnt_ == 0) {
      addOp(kX, x);
      addOp(kY, y);
      x_ = x;
      y_ = y;
      point_cnt_++;
      jct_id++;
    } else if ((x_ == x) && (y_ == y)) {
      addOp(kColinear, 0);
    } else if (y_ == y) {
      addOp(kX, x);
      x_ = x;
      point_cnt_++;
    } else if (x_ == x) {
      addOp(kY, y);
      y_ = y;
      point_cnt_++;
    } else {
      assert(DB_WIRE_ENCODER_NON_ORTHOGANAL_SEGMENT);
    }

    if (point_cnt_
        && ((point_cnt_ & (WOP_NON_DEFAULT_WIDTH_POINT_CNT - 1)) == 0)) {
      addOp(rule_opcode_, non_default_rule_);
    }
  }
  if (point_cnt_ != 1) {
    addOp(kProperty, property);
  }

  prev_extended_colinear_pnt_ = false;
  return jct_id;
}

#define DB_WIRE_ENCODER_COLINEAR_EXT_RULE_1() \
  ((point_cnt_ > 1) || ((point_cnt_ > 0) && (via_cnt_ > 0)))
#define DB_WIRE_ENCODER_COLINEAR_EXT_RULE_2() \
  (prev_extended_colinear_pnt_ == false)

int dbWireEncoder::addPoint(int x, int y, int ext, uint32_t property)
{
  int jct_id = idx_;

  if (non_default_rule_ == 0) {
    if (point_cnt_ == 0) {
      addOp(kX | WOP_DEFAULT_WIDTH, x);
      addOp(kY | WOP_EXTENSION | WOP_DEFAULT_WIDTH, y);
      addOp(kOperand, ext);
      x_ = x;
      y_ = y;
      point_cnt_++;
      jct_id++;
      prev_extended_colinear_pnt_ = false;
    } else if ((x_ == x) && (y_ == y)) {
      assert(DB_WIRE_ENCODER_COLINEAR_EXT_RULE_1());
      assert(DB_WIRE_ENCODER_COLINEAR_EXT_RULE_2());
      addOp(kColinear | WOP_EXTENSION | WOP_DEFAULT_WIDTH, ext);
      prev_extended_colinear_pnt_ = true;
    } else if (y_ == y) {
      addOp(kX | WOP_EXTENSION | WOP_DEFAULT_WIDTH, x);
      addOp(kOperand, ext);
      x_ = x;
      point_cnt_++;
      prev_extended_colinear_pnt_ = false;
    } else if (x_ == x) {
      addOp(kY | WOP_EXTENSION | WOP_DEFAULT_WIDTH, y);
      addOp(kOperand, ext);
      y_ = y;
      point_cnt_++;
      prev_extended_colinear_pnt_ = false;
    } else {
      assert(DB_WIRE_ENCODER_NON_ORTHOGANAL_SEGMENT);
    }
  } else {
    if (point_cnt_ == 0) {
      addOp(kX, x);
      addOp(kY | WOP_EXTENSION, y);
      addOp(kOperand, ext);
      x_ = x;
      y_ = y;
      point_cnt_++;
      jct_id++;
      prev_extended_colinear_pnt_ = false;
    } else if ((x_ == x) && (y_ == y)) {
      assert(DB_WIRE_ENCODER_COLINEAR_EXT_RULE_1());
      assert(DB_WIRE_ENCODER_COLINEAR_EXT_RULE_2());
      addOp(kColinear | WOP_EXTENSION, ext);
      prev_extended_colinear_pnt_ = true;
    } else if (y_ == y) {
      addOp(kX | WOP_EXTENSION, x);
      addOp(kOperand, ext);
      x_ = x;
      point_cnt_++;
      prev_extended_colinear_pnt_ = false;
    } else if (x_ == x) {
      addOp(kY | WOP_EXTENSION, y);
      addOp(kOperand, ext);
      y_ = y;
      point_cnt_++;
      prev_extended_colinear_pnt_ = false;
    } else {
      assert(DB_WIRE_ENCODER_NON_ORTHOGANAL_SEGMENT);
    }

    if (point_cnt_
        && ((point_cnt_ & (WOP_NON_DEFAULT_WIDTH_POINT_CNT - 1)) == 0)) {
      addOp(rule_opcode_, non_default_rule_);
    }
  }
  if (point_cnt_ != 1) {
    addOp(kProperty, property);
  }

  return jct_id;
}

int dbWireEncoder::addVia(dbVia* via)
{
  int jct_id = idx_;
  assert(point_cnt_ != 0);
  dbTechLayer* top = via->getTopLayer();
  dbTechLayer* bot = via->getBottomLayer();

  if (top == layer_) {
    layer_ = bot;
    addOp(kVia, via->getImpl()->getOID());
  } else if (bot == layer_) {
    layer_ = top;
    addOp(kVia | WOP_VIA_EXIT_TOP, via->getImpl()->getOID());
  } else {
    assert(DB_WIRE_ENCODER_INVALID_VIA_LAYER);
    addOp(kVia, 0);
  }

  via_cnt_++;
  return jct_id;
}

int dbWireEncoder::addTechVia(dbTechVia* via)
{
  int jct_id = idx_;
  assert(point_cnt_ != 0);
  dbTechLayer* top = via->getTopLayer();
  dbTechLayer* bot = via->getBottomLayer();

  if (top == layer_) {
    layer_ = bot;
    addOp(kTechVia, via->getImpl()->getOID());
  } else if (bot == layer_) {
    layer_ = top;
    addOp(kTechVia | WOP_VIA_EXIT_TOP, via->getImpl()->getOID());
  } else {
    assert(DB_WIRE_ENCODER_INVALID_VIA_LAYER);
    addOp(kTechVia, 0);
  }
  clearColor();

  via_cnt_++;
  return jct_id;
}

void dbWireEncoder::addRect(int deltaX1, int deltaY1, int deltaX2, int deltaY2)
{
  // order must match dbWireDecoder::next
  assert(point_cnt_ != 0);
  addOp(kRect, deltaX1);
  addOp(kOperand, deltaY1);
  addOp(kOperand, deltaX2);
  addOp(kOperand, deltaY2);
}

void dbWireEncoder::addITerm(dbITerm* iterm)
{
  assert(point_cnt_ != 0);
  addOp(kIterm, iterm->getImpl()->getOID());
}

void dbWireEncoder::addBTerm(dbBTerm* bterm)
{
  assert(point_cnt_ != 0);
  addOp(kBterm, bterm->getImpl()->getOID());
}

void dbWireEncoder::newPath(dbTechLayer* layer, dbWireType type)
{
  initPath(layer, type);
  addOp(kPath | wire_type_, layer->getImpl()->getOID());
}

void dbWireEncoder::newPath(dbTechLayer* layer,
                            dbWireType type,
                            dbTechLayerRule* rule)
{
  initPath(layer, type, rule);
  addOp(kPath | wire_type_, layer->getImpl()->getOID());
  addOp(rule_opcode_, non_default_rule_);
}

void dbWireEncoder::newPath(int jct_id)
{
  assert((jct_id >= 0) && (jct_id < idx_));

  WirePoint pnt;
  getPrevPoint(tech_, block_, opcodes_, data_, jct_id, true, pnt);
  initPath(pnt.layer, wire_type_);
  x_ = pnt.x;
  y_ = pnt.y;
  point_cnt_ = 1;

  addOp(kJunction | wire_type_, jct_id);
  addOp(kColinear | WOP_DEFAULT_WIDTH, 0);
}

void dbWireEncoder::newPath(int jct_id, dbWireType type)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  WirePoint pnt;
  getPrevPoint(tech_, block_, opcodes_, data_, jct_id, true, pnt);
  initPath(pnt.layer, type);
  x_ = pnt.x;
  y_ = pnt.y;
  point_cnt_ = 1;
  addOp(kJunction | wire_type_, jct_id);
  addOp(kColinear | WOP_DEFAULT_WIDTH, 0);
}

void dbWireEncoder::newPathShort(int jct_id,
                                 dbTechLayer* layer,
                                 dbWireType type)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  initPath(layer, type);
  addOp(kShort | wire_type_, layer->getImpl()->getOID());
  addOp(kOperand, jct_id);
}

void dbWireEncoder::newPathVirtualWire(int jct_id,
                                       dbTechLayer* layer,
                                       dbWireType type)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  initPath(layer, type);
  addOp(kVwire | wire_type_, layer->getImpl()->getOID());
  addOp(kOperand, jct_id);
}

void dbWireEncoder::newPathExt(int jct_id, int ext)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  WirePoint pnt;
  getPrevPoint(tech_, block_, opcodes_, data_, jct_id, true, pnt);
  initPath(pnt.layer, wire_type_);
  x_ = pnt.x;
  y_ = pnt.y;
  point_cnt_ = 1;
  addOp(kJunction | wire_type_, jct_id);
  addOp(kColinear | WOP_EXTENSION | WOP_DEFAULT_WIDTH, ext);
}

void dbWireEncoder::newPathExt(int jct_id, int ext, dbWireType type)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  WirePoint pnt;
  getPrevPoint(tech_, block_, opcodes_, data_, jct_id, true, pnt);
  initPath(pnt.layer, type);
  x_ = pnt.x;
  y_ = pnt.y;
  point_cnt_ = 1;
  addOp(kJunction | wire_type_, jct_id);
  addOp(kColinear | WOP_EXTENSION | WOP_DEFAULT_WIDTH, ext);
}

void dbWireEncoder::newPath(int jct_id, dbTechLayerRule* rule)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  WirePoint pnt;
  getPrevPoint(tech_, block_, opcodes_, data_, jct_id, true, pnt);
  initPath(pnt.layer, wire_type_, rule);
  x_ = pnt.x;
  y_ = pnt.y;
  point_cnt_ = 1;
  addOp(kJunction | wire_type_, jct_id);
  addOp(rule_opcode_, non_default_rule_);
  addOp(kColinear, 0);
}

void dbWireEncoder::newPath(int jct_id, dbWireType type, dbTechLayerRule* rule)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  WirePoint pnt;
  getPrevPoint(tech_, block_, opcodes_, data_, jct_id, true, pnt);
  initPath(pnt.layer, type, rule);
  x_ = pnt.x;
  y_ = pnt.y;
  point_cnt_ = 1;
  addOp(kJunction | wire_type_, jct_id);
  addOp(rule_opcode_, non_default_rule_);
  addOp(kColinear, 0);
}

void dbWireEncoder::newPathShort(int jct_id,
                                 dbTechLayer* layer,
                                 dbWireType type,
                                 dbTechLayerRule* rule)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  initPath(layer, type, rule);
  addOp(kShort | wire_type_, layer->getImpl()->getOID());
  addOp(kOperand, jct_id);
  addOp(rule_opcode_, non_default_rule_);
}

void dbWireEncoder::newPathVirtualWire(int jct_id,
                                       dbTechLayer* layer,
                                       dbWireType type,
                                       dbTechLayerRule* rule)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  initPath(layer, type, rule);
  addOp(kVwire | wire_type_, layer->getImpl()->getOID());
  addOp(kOperand, jct_id);
  addOp(rule_opcode_, non_default_rule_);
}

void dbWireEncoder::newPathExt(int jct_id, int ext, dbTechLayerRule* rule)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  WirePoint pnt;
  getPrevPoint(tech_, block_, opcodes_, data_, jct_id, true, pnt);
  initPath(pnt.layer, wire_type_, rule);
  x_ = pnt.x;
  y_ = pnt.y;
  point_cnt_ = 1;
  addOp(kJunction | wire_type_, jct_id);
  addOp(rule_opcode_, non_default_rule_);
  addOp(kColinear | WOP_EXTENSION, ext);
}

void dbWireEncoder::newPathExt(int jct_id,
                               int ext,
                               dbWireType type,
                               dbTechLayerRule* rule)
{
  assert((jct_id >= 0) && (jct_id < idx_));
  WirePoint pnt;
  getPrevPoint(tech_, block_, opcodes_, data_, jct_id, true, pnt);
  initPath(pnt.layer, type, rule);
  x_ = pnt.x;
  y_ = pnt.y;
  point_cnt_ = 1;
  addOp(kJunction | wire_type_, jct_id);
  addOp(rule_opcode_, non_default_rule_);
  addOp(kColinear | WOP_EXTENSION, ext);
}

void dbWireEncoder::end()
{
  if (opcodes_.empty()) {
    return;
  }

  uint32_t n = opcodes_.size();

  // Free the old memory
  wire_->data_.~dbVector<int>();
  new (&wire_->data_) dbVector<int>();
  wire_->data_.reserve(n);
  wire_->data_ = data_;

  // Free the old memory
  wire_->opcodes_.~dbVector<unsigned char>();
  new (&wire_->opcodes_) dbVector<unsigned char>();
  wire_->opcodes_.reserve(n);
  wire_->opcodes_ = opcodes_;

  // Should we calculate the bbox???
  ((_dbBlock*) block_)->flags_.valid_bbox = 0;
  point_cnt_ = 0;

  for (auto callback : ((_dbBlock*) block_)->callbacks_) {
    callback->inDbWirePostModify((dbWire*) wire_);
  }
}

void dbWireEncoder::setColor(uint8_t mask_color)
{
  // LEF/DEF says 3 is the max number of supported masks per layer.
  // 0 is also not a valid mask.
  if (mask_color < 1 || mask_color > 3) {
    utl::Logger* logger = wire_->getImpl()->getLogger();
    logger->error(utl::ODB,
                  1102,
                  "Mask color: {}, but must be between 1 and 3",
                  mask_color);
  }

  addOp(kColor, mask_color);
}

void dbWireEncoder::clearColor()
{
  // 0 is a special value representing no mask color.
  addOp(kColor, 0);
}

void dbWireEncoder::setViaColor(uint8_t bottom_color,
                                uint8_t cut_color,
                                uint8_t top_color)
{
  // LEF/DEF says 3 is the max number of supported masks per layer.
  // 0 is also not a valid mask.
  for (const auto color : {bottom_color, cut_color, top_color}) {
    if (color > 3) {
      utl::Logger* logger = wire_->getImpl()->getLogger();
      logger->error(
          utl::ODB, 1103, "Mask color: {}, but must be between 0 and 3", color);
    }
  }

  // encode as XX BB CC TT
  const uint8_t mask_color = bottom_color << 4 | cut_color << 2 | top_color;

  addOp(kViaColor, mask_color);
}

void dbWireEncoder::clearViaColor()
{
  // 0 is a special value representing no mask color.
  addOp(kViaColor, 0);
}

//////////////////////////////////////////////////////////////////////////////////
//
// dbWireDecoder
//
//////////////////////////////////////////////////////////////////////////////////

#define DB_WIRE_DECODE_INVALID_OPCODE 0
#define DB_WIRE_DECODE_INVALID_OPCODE_SEQUENCE 0
#define DB_WIRE_DECODE_INVALID_VIA_LAYER 0

dbWireDecoder::dbWireDecoder()
{
  wire_ = nullptr;
  block_ = nullptr;
  tech_ = nullptr;
}

void dbWireDecoder::begin(dbWire* wire)
{
  wire_ = (_dbWire*) wire;
  block_ = wire->getBlock();
  tech_ = block_->getDb()->getTech();
  x_ = 0;
  y_ = 0;
  default_width_ = true;
  layer_ = nullptr;
  idx_ = 0;
  jct_id_ = -1;
  opcode_ = END_DECODE;
  wire_type_ = dbWireType::NONE;
  point_cnt_ = 0;
  property_ = 0;
  deltaX1_ = 0;
  deltaY1_ = 0;
  deltaX2_ = 0;
  deltaY2_ = 0;
}

inline unsigned char dbWireDecoder::nextOp(int& value)
{
  assert(idx_ < (int) wire_->length());
  value = wire_->data_[idx_];
  return wire_->opcodes_[idx_++];
}

inline unsigned char dbWireDecoder::nextOp(uint32_t& value)
{
  assert(idx_ < (int) wire_->length());
  value = (uint32_t) wire_->data_[idx_];
  return wire_->opcodes_[idx_++];
}

inline unsigned char dbWireDecoder::peekOp()
{
  assert(idx_ < (int) wire_->length());
  return wire_->opcodes_[idx_];
}

//
// Flush the rule opcode if it exists here. The rule opcode that follows a point
// is used to speed up shape-id searches.
//
inline void dbWireDecoder::flushRule()
{
  if (idx_ != (int) wire_->opcodes_.size()) {
    if ((wire_->opcodes_[idx_] & WOP_OPCODE_MASK) == kRule) {
      ++idx_;
    }
  }
}

dbWireDecoder::OpCode dbWireDecoder::peek() const
{
  int idx = idx_;

nextOpCode:

  if (idx == (int) wire_->opcodes_.size()) {
    return END_DECODE;
  }

  unsigned char opcode = wire_->opcodes_[idx];

  switch (opcode & WOP_OPCODE_MASK) {
    case kPath:
      return PATH;

    case kShort:
      return SHORT;

    case kJunction:
      return JUNCTION;

    case kRule:
      return RULE;

    case kX:
    case kY:
    case kColinear:
      if (opcode & WOP_EXTENSION) {
        return POINT_EXT;
      }

      return POINT;

    case kVia:
      return VIA;

    case kTechVia:
      return TECH_VIA;

    case kIterm:
      return ITERM;

    case kBterm:
      return BTERM;

    case kRect:
      return RECT;

    case kVwire:
      return VWIRE;

    default:
      ++idx;
      goto nextOpCode;
  }
}

dbWireDecoder::OpCode dbWireDecoder::next()
{
nextOpCode:

  if (idx_ == (int) wire_->opcodes_.size()) {
    return END_DECODE;
  }

  jct_id_ = idx_;
  unsigned char opcode = nextOp(operand_);
  const WireOp wire_op = static_cast<WireOp>(opcode & WOP_OPCODE_MASK);
  switch (wire_op) {
    case kPath: {
      layer_ = dbTechLayer::getTechLayer(tech_, operand_);
      point_cnt_ = 0;
      default_width_ = true;

      const WireType wire_type
          = static_cast<WireType>(opcode & WOP_WIRE_TYPE_MASK);
      switch (wire_type) {
        case kNone:
          wire_type_ = dbWireType::NONE;
          break;

        case kCover:
          wire_type_ = dbWireType::COVER;
          break;

        case kFixed:
          wire_type_ = dbWireType::FIXED;
          break;

        case kRouted:
          wire_type_ = dbWireType::ROUTED;
          break;

        case kNoShield:
          wire_type_ = dbWireType::NOSHIELD;
          break;
      }

      return opcode_ = PATH;
    }

    case kShort: {
      layer_ = dbTechLayer::getTechLayer(tech_, operand_);
      point_cnt_ = 0;
      default_width_ = true;

      const WireType wire_type
          = static_cast<WireType>(opcode & WOP_WIRE_TYPE_MASK);
      switch (wire_type) {
        case kNone:
          wire_type_ = dbWireType::NONE;
          break;

        case kCover:
          wire_type_ = dbWireType::COVER;
          break;

        case kFixed:
          wire_type_ = dbWireType::FIXED;
          break;

        case kRouted:
          wire_type_ = dbWireType::ROUTED;
          break;

        case kNoShield:
          wire_type_ = dbWireType::NOSHIELD;
          break;
      }

      // getjuction id
      nextOp(operand2_);
      return opcode_ = SHORT;
    }

    case kJunction: {
      WirePoint pnt;
      getPrevPoint(
          tech_, block_, wire_->opcodes_, wire_->data_, operand_, true, pnt);
      layer_ = pnt.layer;
      x_ = pnt.x;
      y_ = pnt.y;
      point_cnt_ = 0;
      default_width_ = true;

      const WireType wire_type
          = static_cast<WireType>(opcode & WOP_WIRE_TYPE_MASK);
      switch (wire_type) {
        case kNone:
          wire_type_ = dbWireType::NONE;
          break;

        case kCover:
          wire_type_ = dbWireType::COVER;
          break;

        case kFixed:
          wire_type_ = dbWireType::FIXED;
          break;

        case kRouted:
          wire_type_ = dbWireType::ROUTED;
          break;

        case kNoShield:
          wire_type_ = dbWireType::NOSHIELD;
          break;
      }

      return opcode_ = JUNCTION;
    }

    case kRule:
      default_width_ = false;
      _block_rule = (opcode & WOP_BLOCK_RULE);
      return opcode_ = RULE;

    case kX: {
      x_ = operand_;

      if (point_cnt_ == 0) {
        jct_id_ = idx_;
        opcode = nextOp(y_);
        point_cnt_ = 1;

        if (opcode & WOP_EXTENSION) {
          nextOp(operand2_);
          return opcode_ = POINT_EXT;
        }

        return opcode_ = POINT;
      }

      point_cnt_++;

      if (opcode & WOP_EXTENSION) {
        nextOp(operand2_);
        opcode_ = POINT_EXT;
      } else {
        opcode_ = POINT;
      }

      // if ( peekOp() == WOP_PROPERTY )
      //{
      //    opcode = nextOp(_property);
      //    assert(opcode == WOP_PROPERTY);
      //}
      // else
      property_ = 0;

      flushRule();
      return opcode_;
    }

    case kY: {
      assert(point_cnt_ != 0);
      point_cnt_++;
      y_ = operand_;

      if (opcode & WOP_EXTENSION) {
        nextOp(operand2_);
        opcode_ = POINT_EXT;
      } else {
        opcode_ = POINT;
      }

      // if ( peekOp() == WOP_PROPERTY )
      //{
      //    opcode = nextOp(_property);
      //    assert(opcode == WOP_PROPERTY);
      //}
      // else
      property_ = 0;

      flushRule();
      return opcode_;
    }

    case kColinear: {
      point_cnt_++;

      if (opcode & WOP_EXTENSION) {
        operand2_ = operand_;
        opcode_ = POINT_EXT;
      } else {
        opcode_ = POINT;
      }

      // if ( peekOp() == WOP_PROPERTY )
      //{
      //    opcode = nextOp(_property);
      //    assert(opcode == WOP_PROPERTY);
      //}
      // else
      property_ = 0;

      flushRule();
      return opcode_;
    }

    case kVia: {
      dbVia* via = dbVia::getVia(block_, operand_);

      if (opcode & WOP_VIA_EXIT_TOP) {
        layer_ = via->getTopLayer();
      } else {
        layer_ = via->getBottomLayer();
      }

      return opcode_ = VIA;
    }

    case kTechVia: {
      dbTechVia* via = dbTechVia::getTechVia(tech_, operand_);

      if (opcode & WOP_VIA_EXIT_TOP) {
        layer_ = via->getTopLayer();
      } else {
        layer_ = via->getBottomLayer();
      }

      return opcode_ = TECH_VIA;
    }

    case kRect:
      // order matches dbWireEncoder::addRect
      deltaX1_ = operand_;
      nextOp(deltaY1_);
      nextOp(deltaX2_);
      nextOp(deltaY2_);
      return opcode_ = RECT;

    case kIterm:
      return opcode_ = ITERM;

    case kBterm:
      return opcode_ = BTERM;

    case kOperand:
      assert(DB_WIRE_DECODE_INVALID_OPCODE_SEQUENCE);
      goto nextOpCode;

    case kProperty:
      goto nextOpCode;

    case kVwire: {
      layer_ = dbTechLayer::getTechLayer(tech_, operand_);
      point_cnt_ = 0;
      default_width_ = true;

      const WireType wire_type
          = static_cast<WireType>(opcode & WOP_WIRE_TYPE_MASK);
      switch (wire_type) {
        case kNone:
          wire_type_ = dbWireType::NONE;
          break;

        case kCover:
          wire_type_ = dbWireType::COVER;
          break;

        case kFixed:
          wire_type_ = dbWireType::FIXED;
          break;

        case kRouted:
          wire_type_ = dbWireType::ROUTED;
          break;

        case kNoShield:
          wire_type_ = dbWireType::NOSHIELD;
          break;
      }

      // getjuction id
      nextOp(operand2_);
      return opcode_ = VWIRE;
    }

    case kNop:
      goto nextOpCode;

    case kColor: {
      // 3 MSB bits of the opcode represent the color
      color_ = static_cast<uint8_t>(operand_);

      if (color_.value() == 0) {
        color_ = std::nullopt;
      }

      goto nextOpCode;
    }

    case kViaColor: {
      uint8_t viacolor = static_cast<uint8_t>(operand_);
      if (viacolor == 0) {
        viacolor_ = std::nullopt;
      } else {
        viacolor_ = ViaColor();
        viacolor_.value().bottom_color = (viacolor & 0x30) >> 4;
        viacolor_.value().cut_color = (viacolor & 0x0c) >> 2;
        viacolor_.value().top_color = (viacolor & 0x03);
      }

      goto nextOpCode;
    }

    default:
      assert(DB_WIRE_DECODE_INVALID_OPCODE);
      goto nextOpCode;
  }
}

dbTechLayer* dbWireDecoder::getLayer() const
{
  return layer_;
}

dbTechLayerRule* dbWireDecoder::getRule() const
{
  assert(opcode_ == RULE);

  if (_block_rule) {
    return dbTechLayerRule::getTechLayerRule(block_, operand_);
  }
  return dbTechLayerRule::getTechLayerRule(tech_, operand_);
}

void dbWireDecoder::getPoint(int& x, int& y) const
{
  assert(opcode_ == POINT);
  x = x_;
  y = y_;
}

void dbWireDecoder::getPoint(int& x, int& y, int& ext) const
{
  assert(opcode_ == POINT_EXT);
  x = x_;
  y = y_;
  ext = operand2_;
}

uint32_t dbWireDecoder::getProperty() const
{
  assert(((opcode_ == POINT) || (opcode_ == POINT_EXT)) && (point_cnt_ > 1));
  return property_;
}

dbVia* dbWireDecoder::getVia() const
{
  assert(opcode_ == VIA);
  dbVia* via = dbVia::getVia(block_, operand_);
  return via;
}

dbTechVia* dbWireDecoder::getTechVia() const
{
  assert(opcode_ == TECH_VIA);
  dbTechVia* via = dbTechVia::getTechVia(tech_, operand_);
  return via;
}

std::optional<uint8_t> dbWireDecoder::getColor() const
{
  return color_;
}

std::optional<dbWireDecoder::ViaColor> dbWireDecoder::getViaColor() const
{
  return viacolor_;
}

void dbWireDecoder::getRect(int& deltaX1,
                            int& deltaY1,
                            int& deltaX2,
                            int& deltaY2) const
{
  assert(opcode_ == RECT);
  deltaX1 = deltaX1_;
  deltaY1 = deltaY1_;
  deltaX2 = deltaX2_;
  deltaY2 = deltaY2_;
}

dbITerm* dbWireDecoder::getITerm() const
{
  assert(opcode_ == ITERM);
  dbITerm* iterm = dbITerm::getITerm(block_, operand_);
  return iterm;
}

dbBTerm* dbWireDecoder::getBTerm() const
{
  assert(opcode_ == BTERM);
  dbBTerm* bterm = dbBTerm::getBTerm(block_, operand_);
  return bterm;
}

dbWireType dbWireDecoder::getWireType() const
{
  assert((opcode_ == PATH) || (opcode_ == JUNCTION) || (opcode_ == SHORT)
         || (opcode_ == VWIRE));
  return dbWireType((dbWireType::Value) wire_type_);
}

int dbWireDecoder::getJunctionId() const
{
  return jct_id_;
}

int dbWireDecoder::getJunctionValue() const
{
  assert((opcode_ == JUNCTION) || (opcode_ == SHORT) || (opcode_ == VWIRE));

  if (opcode_ == SHORT || opcode_ == VWIRE) {
    return operand2_;
  }

  return operand_;
}

void dumpDecoder4Net(dbNet* innet)
{
  if (!innet) {
    return;
  }

  const char* prfx = "dumpDecoder:";
  dbWire* wire0 = innet->getWire();
  utl::Logger* logger = innet->getImpl()->getLogger();
  if (!wire0) {
    logger->warn(
        utl::ODB, 63, "{} No wires for net {}", prfx, innet->getName());
    return;
  }

  dbWireDecoder decoder;
  decoder.begin(wire0);
  logger->info(
      utl::ODB, 64, "{} begin decoder for net {}", prfx, innet->getName());

  dbTechLayerRule* lyr_rule = nullptr;
  while (true) {
    dbWireDecoder::OpCode opcode = decoder.next();
    if (opcode == dbWireDecoder::END_DECODE) {
      logger->info(
          utl::ODB, 65, "{} End decoder for net {}", prfx, innet->getName());
      break;
    }

    switch (opcode) {
      case dbWireDecoder::PATH: {
        dbTechLayer* layer = decoder.getLayer();
        dbWireType wtype = decoder.getWireType();
        if (decoder.peek() == dbWireDecoder::RULE) {
          decoder.next();
          lyr_rule = decoder.getRule();
          logger->info(utl::ODB,
                       66,
                       "{} New path: layer {} type {}  non-default rule {}",
                       prfx,
                       layer->getName(),
                       wtype.getString(),
                       lyr_rule->getNonDefaultRule()->getName());

        } else {
          logger->info(utl::ODB,
                       67,
                       "{} New path: layer {} type {}\n",
                       prfx,
                       layer->getName(),
                       wtype.getString());
        }
        break;
      }

      case dbWireDecoder::JUNCTION: {
        uint32_t jct = decoder.getJunctionValue();
        lyr_rule = nullptr;
        opcode = decoder.peek();
        if (opcode == dbWireDecoder::RULE) {
          decoder.next();
          lyr_rule = decoder.getRule();
          opcode = decoder.peek();
        }
        if (opcode == dbWireDecoder::POINT_EXT) {
          decoder.next();
          int x, y, ext;
          decoder.getPoint(x, y, ext);
          if (lyr_rule) {
            logger->info(
                utl::ODB,
                68,
                "{} New path at junction {}, point(ext) {} {} {}, with rule "
                "{}",
                prfx,
                jct,
                x,
                y,
                ext,
                lyr_rule->getNonDefaultRule()->getName());
          } else {
            logger->info(utl::ODB,
                         69,
                         "{} New path at junction {}, point(ext) {} {} {}",
                         prfx,
                         jct,
                         x,
                         y,
                         ext);
          }
        } else if (opcode == dbWireDecoder::POINT) {
          decoder.next();
          int x, y;
          decoder.getPoint(x, y);
          if (lyr_rule) {
            logger->info(
                utl::ODB,
                70,
                "{} New path at junction {}, point {} {}, with rule {}",
                prfx,
                jct,
                x,
                y,
                lyr_rule->getNonDefaultRule()->getName());
          } else {
            logger->info(utl::ODB,
                         71,
                         "{} New path at junction {}, point {} {}",
                         prfx,
                         jct,
                         x,
                         y);
          }

        } else {
          logger->warn(utl::ODB,
                       72,
                       "{} opcode after junction is not point or point_ext??\n",
                       prfx);
        }
        break;
      }

      case dbWireDecoder::SHORT: {
        uint32_t jval = decoder.getJunctionValue();
        lyr_rule = nullptr;
        opcode = decoder.peek();
        if (opcode == dbWireDecoder::RULE) {
          decoder.next();
          lyr_rule = decoder.getRule();
        }
        if (lyr_rule) {
          logger->info(utl::ODB,
                       73,
                       "{} Short at junction {}, with rule {}",
                       prfx,
                       jval,
                       lyr_rule->getNonDefaultRule()->getName());
        } else {
          logger->info(utl::ODB, 74, "{} Short at junction {}", prfx, jval);
        }
        break;
      }

      case dbWireDecoder::VWIRE: {
        uint32_t jval = decoder.getJunctionValue();
        lyr_rule = nullptr;
        opcode = decoder.peek();
        if (opcode == dbWireDecoder::RULE) {
          decoder.next();
          lyr_rule = decoder.getRule();
        }
        if (lyr_rule) {
          logger->info(utl::ODB,
                       75,
                       "{} Virtual wire at junction {}, with rule {}",
                       prfx,
                       jval,
                       lyr_rule->getNonDefaultRule()->getName());
        } else {
          logger->info(
              utl::ODB, 76, "{} Virtual wire at junction {}", prfx, jval);
        }
        break;
      }

      case dbWireDecoder::POINT: {
        int x, y;
        decoder.getPoint(x, y);
        logger->info(utl::ODB, 77, "{} Found point {} {}", prfx, x, y);
        break;
      }

      case dbWireDecoder::POINT_EXT: {
        int x, y, ext;
        decoder.getPoint(x, y, ext);
        logger->info(
            utl::ODB, 78, "{} Found point(ext){} {} {}", prfx, x, y, ext);
        break;
      }

      case dbWireDecoder::TECH_VIA: {
        logger->info(utl::ODB,
                     79,
                     "{} Found via {}",
                     prfx,
                     decoder.getTechVia()->getName());
        break;
      }

      case dbWireDecoder::VIA: {
        logger->info(utl::ODB, 80, "block via found in signal net!");
        break;
      }

      case dbWireDecoder::RECT: {
        int deltaX1;
        int deltaY1;
        int deltaX2;
        int deltaY2;
        decoder.getRect(deltaX1, deltaY1, deltaX2, deltaY2);
        break;
      }

      case dbWireDecoder::ITERM: {
        logger->info(utl::ODB, 81, "{} Found Iterm", prfx);
        break;
      }

      case dbWireDecoder::BTERM: {
        logger->info(utl::ODB, 82, "{} Found Bterm", prfx);
        break;
      }

      case dbWireDecoder::RULE: {
        if (lyr_rule->getNonDefaultRule()->getName()
            != (decoder.getRule())->getNonDefaultRule()->getName()) {
          logger->error(
              utl::ODB,
              83,
              "{} GOT RULE {}, EXPECTED RULE {}",
              prfx,
              lyr_rule->getNonDefaultRule()->getName().c_str(),
              decoder.getRule()->getNonDefaultRule()->getName().c_str());
        }
        lyr_rule = decoder.getRule();
        logger->info(utl::ODB,
                     84,
                     "{} Found Rule {} in middle of path",
                     prfx,
                     lyr_rule->getNonDefaultRule()->getName().c_str());
        break;
      }

      case dbWireDecoder::END_DECODE:
        logger->info(utl::ODB,
                     85,
                     "{} End decoder for net {}",
                     prfx,
                     innet->getName().c_str());
        break;

      default: {
        logger->error(utl::ODB, 86, "{} Hit default!", prfx);
        break;
      }
    }  // switch opcode
  }  // while
}

void dumpDecoder(dbBlock* inblk, const char* net_name_or_id)
{
  const char* prfx = "dumpDecoder:";

  if (!inblk || !net_name_or_id) {
    // error(0, "%s Must specify DB Block and either net name or ID\n", prfx);
    return;
  }

  dbNet* innet = nullptr;
  const char* ckdigit;
  for (ckdigit = net_name_or_id; *ckdigit; ckdigit++) {
    if (!isdigit(*ckdigit)) {
      break;
    }
  }

  innet = (*ckdigit == '\0') ? dbNet::getNet(inblk, atoi(net_name_or_id))
                             : inblk->findNet(net_name_or_id);
  if (!innet) {
    inblk->getImpl()->getLogger()->error(
        utl::ODB, 0, "{} Net {} not found", prfx, net_name_or_id);
    return;
  }

  dumpDecoder4Net(innet);
}

}  // namespace odb
