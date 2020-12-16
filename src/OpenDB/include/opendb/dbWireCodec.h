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

#include <vector>

#include "dbTypes.h"
#include "odb.h"

namespace odb {

class dbTech;
class dbITerm;
class dbTechLayer;
class dbWire;
class dbNet;
class dbVia;
class dbTechVia;
class dbTechLayerRule;
class dbBlock;
class dbBTerm;
class _dbWire;

///
/// dbWireEncoder - This class is used to create net-wire. The wire is a
/// collection of from-to's, which can optionally describe connections to
/// ITERMS, or BTERMS. The from-to's may either form a disjoint or non-disjoint
/// set of path-segments.
///
/// Example 1 ***************************************************************
///
///    (x1,y1)      M1             (x2,y1,VIA)         M1        (x3,y1)
///        +-----------------------------+-------------------------+
///                                      |
///                                      |
///                                      |
///                                      | M2
///                                      |
///                                      |
///                                      |
///                                      + (X2,Y2)
///
/// This wire can be described in two-paths as follows:
///
///    (x1,y1)      M1             (x2,y1,JCT)         M1        (x3,y1)
///        +-----------------------------+-------------------------+
///
/// And:
///                                (JCT-ID, VIA)
///                                      +
///                                      |
///                                      |
///                                      |
///                                      | M2
///                                      |
///                                      |
///                                      |
///                                      + (x2,y2)
///
///  Code:
///
///      dbNet * net = dbNet::create( block, "net0" );
///      dbTechLayer * M1 = tech->findLayer("M1");
///      dbTechLayer * VIA = tech->findVia("VIA_1_2");
///      dbWire * wire = dbWire::create( net );
///      dbWireEncoder encoder;
///      encoder.begin( wire );
///      encoder.newPath( M1, dbWireType::ROUTED );
///      encoder.addPoint( x1, y1 );
///      int jct_1 = encoder.addPoint( x2, y1 );
///      encoder.addPoint( x3, y1 );
///      encoder.newPath( jct_1 );
///      encoder.addVia( VIA );
///      encoder.addPoint( x2,y2 );
///      encoder.end();
///
/// Example 2 ***************************************************************
///
/// Alternatively, the example above can be decribed as follows:
///
///    (x1,y1)      M1             (x2,y1,JCT)
///        +-----------------------------+
///                                      |
///                                      |
///                                      |
///                                      | M2
///                                      |
///                                      |
///                                      |
///                                      + (X2,Y2)
///
/// And:
///                                      +-------------------------+
/// Code:
///
///      dbNet * net = dbNet::create( block, "net0" );
///      dbTechLayer * M1 = tech->findLayer("M1");
///      dbTechLayer * VIA = tech->findVia("VIA_1_2");
///      dbWire * wire = dbWire::create( net );
///      dbWireEncoder encoder;
///      encoder.begin( wire );
///      encoder.newPath( M1, dbWireType::ROUTED );
///      encoder.addPoint( x1, y1 );
///      int jct_1 = encoder.addPoint( x2, y1 );
///      encoder.addVia( VIA );
///      encoder.addPoint( x2,y2 );
///      encoder.newPath( jct_1 );
///      encoder.addPoint( x3, y1 );
///      encoder.end();
///
/// Example 3 ***************************************************************
///
/// The following example illustrates how to specify a VIA-JCT.
///
///                                      + (x2,y3)
///                                      |
///                                      |
///                                      | M2
///                                      |
///    (x1,y1)      M1        (x2,y1,VIA)|            M1        (x3,y1)
///        +-----------------------------+-------------------------+
///                                      |
///                                      | M2
///                                      |
///                                      |
///                                      |
///                                      + (x2,y2)
///
/// This path is encode in three sections:
///
///                                      + (x2,y3) (VIA)
///                                      |
///                                      | M2
///                                      |
///                                      |
///                                      |
///                                      + (x2,y1)
///  And:
///
///    (x1,y1)      M1             (x2,y1,JCT)         M1        (x3,y1)
///        +-----------------------------+-------------------------+
///
///  And:
///                                      + (x2,y1) (VIA,JCT)
///                                      |
///                                      | M2
///                                      |
///                                      |
///                                      |
///                                      + (x2,y2)
///
/// Example Code:
///
///      dbNet * net = dbNet::create( block, "net0" );
///      dbTechLayer * M1 = tech->findLayer("M1");
///      dbTechLayer * VIA = tech->findVia("VIA_1_2");
///      dbWire * wire = dbWire::create( net );
///      dbWireEncoder encoder;
///      encoder.begin( wire );
///      encoder.newPath( M1, dbWireType::ROUTED );
///      encoder.addPoint( x1, y1 );
///      int jct_1 = encoder.addPoint( x2, y1 );
///      encoder.addPoint( x3, y1 );
///      encoder.newPath( jct_1 );
///      int jct_2 encoder.addVia( VIA );
///      encoder.addPoint( x2,y2 );
///      encoder.newPath( jct_2 );
///      encoder.addPoint( x2,y3 );
///      encoder.end();
///

class dbWireEncoder
{
  _dbWire*                   _wire;
  dbTech*                    _tech;
  dbBlock*                   _block;
  std::vector<int>           _data;
  std::vector<unsigned char> _opcodes;
  dbTechLayer*               _layer;
  int                        _idx;
  int                        _x;
  int                        _y;
  uint                       _non_default_rule;
  int                        _point_cnt;
  int                        _via_cnt;
  bool                       _prev_extended_colinear_pnt;
  unsigned char              _wire_type;
  unsigned char              _rule_opcode;

  unsigned char getOp(int idx);
  void          updateOp(int idx, unsigned char op);
  void          addOp(unsigned char op, int value);
  dbWireEncoder(const dbWireEncoder&);
  dbWireEncoder& operator=(const dbWireEncoder&);
  unsigned char  getWireType(dbWireType type);
  void           initPath(dbTechLayer* layer, unsigned char type);
  void initPath(dbTechLayer* layer, unsigned char type, dbTechLayerRule* rule);
  void initPath(dbTechLayer* layer, dbWireType type);
  void initPath(dbTechLayer* layer, dbWireType type, dbTechLayerRule* rule);

 public:
  ///
  /// dbWireEncoder constructor:
  ///
  dbWireEncoder();

  ///
  /// dbWireEncoder destructor:
  ///
  ~dbWireEncoder();

  ///
  /// Begin a new encoding.
  ///
  void begin(dbWire* wire);

  ///
  /// Append to encoding.
  ///
  void append(dbWire* wire);

  ///
  /// Add a path-point. The junction-id of this point is returned.
  ///
  int addPoint(int x, int y, uint property = 0);

  ///
  /// Add a path-point with an extension. The junction-id of this point is
  /// returned.
  ///
  /// An extension is used to extend the wire at a given point.
  /// For example the following path sequence,
  ///
  ///     ( 10 10 ) ( 20 10 e ) ( 20 20 )
  ///
  /// creates two non-default extended segments:
  ///
  ///                  ###
  ///                  #+# (20,20)
  ///                  #|#
  ///                  #|#
  ///                  #|#
  ///       ############|####
  ///       #+----------+eee# (20,10)
  ///       ############e####
  ///                  #e#
  ///      (10,10)     #e#
  ///                  ###
  ///
  /// To generate a path with different extensions at a give point, use an
  /// extended-colinear-point.
  ///
  ///     ( 10 10 ) ( 20 10 e1 ) ( * * e2 ) ( 20 20 )
  ///
  /// To cancel the effect of a previous extension, use a
  /// non-extended-colinear-point.
  ///
  ///     ( 10 10 ) ( 20 10 e1 ) ( * * ) ( 20 20 )
  ///
  /// DEFINITION: An extended-colinear-point starts a new segment in a path
  /// sequence.
  ///
  /// COLINEAR-EXT-RULE-1: 1) A path must contain at least two non-colinear
  /// points (except for beginging
  ///                      a new path) before an extended-colinear-point can be
  ///                      used. 2) Or a path must contain at least one
  ///                      non-colinear-point and a VIA before an
  ///                      extended-colinear-point can be used.
  /// COLINEAR-EXT-RULE-2: An extended-colinear-point can not preceed an
  /// extended-colinear-point.
  ///
  /// Illegal path sequences:
  ///
  ///     ( 10 10 ) ( * * e ) ( 10 20 )
  ///     ( 10 10 ) ( * 20 e ) ( * * e ) ( * * e ) ( 20 20 )
  ///
  /// Legal path sequences:
  ///
  ///     ( 10 10 ) ( * 20 e ) ( * * e ) ( 20 20 )
  ///     ( 10 10 ) VIA ( * * e ) ( 20 20 )
  ///     ( 10 10 ) ( * 20 e ) VIA ( * * e ) ( 20 20 )
  ///     ( 10 10 ) ( * 20 e ) ( * * ) ( * * e ) ( 20 20 )   !!! Legal, but
  ///     redundant !!! ( 10 10 e ) ( * 20 e )
  ///
  /// If a path needs to be extended at junction, use the method
  /// "newPathExt(jct,...)" instead of "newPath(jct,...)".
  ///
  /// For example, the following call sequence is illegal:
  ///
  ///     dbWireEncoder encoder;
  ///     ...
  ///     int j1 = encoder.addPoint( x, y );
  ///     ...
  ///     encoder.newPath(j1);
  ///     encoder.addPoint(x,y,ext); // Illegal due to COLINEAR-EXT-RULE-1.
  ///     ...
  ///
  /// Use this sequence instead:
  ///
  ///     dbWireEncoder encoder;
  ///     ...
  ///     int j1 = encoder.addPoint( x, y );
  ///     ...
  ///     encoder.newPath(j1,ext);
  ///     ...
  ///
  int addPoint(int x, int y, int ext, uint property = 0);

  ///
  /// Add a via at the previous point
  ///
  int addVia(dbVia* via);

  ///
  /// Add a tech-via at the previous point
  ///
  int addTechVia(dbTechVia* via);

  ///
  /// Add a rect (aka patch wire) relative the previous point
  ///
  void addRect(int deltaX1, int deltaY1, int deltaX2, int deltaY2);

  ///
  /// Connect an iterm to the previous point.
  ///
  void addITerm(dbITerm* iterm);

  ///
  /// Connect a bterm to the previous point.
  ///
  void addBTerm(dbBTerm* bterm);

  ///
  /// Start a disjoint path from this point using default width.
  ///
  void newPath(dbTechLayer* layer, dbWireType type);

  ///
  /// Start a disjoint path from this point using default width.
  ///
  void newPath(dbTechLayer* layer, dbWireType type, dbTechLayerRule* rule);

  ///
  /// Start a non-disjoint path from this junction using the default width and
  /// specified wire-type.
  ///
  void newPath(int junction_id, dbWireType type);

  ///
  /// Start a non-disjoint path from this junction using the nondefaultrule and
  /// specified wire-type.
  ///
  void newPath(int junction_id, dbWireType type, dbTechLayerRule* rule);

  ///
  /// Start a non-disjoint path from this junction using the default width and
  /// extending the path with a non-default extension and the specified wire
  /// type
  ///
  void newPathExt(int junction_id, int ext, dbWireType type);

  ///
  /// Start a non-disjoint path from this junction using the default width and
  /// extending the path with a non-default extension and the specified wire
  /// type.
  ///
  void newPathExt(int              junction_id,
                  int              ext,
                  dbWireType       type,
                  dbTechLayerRule* rule);

  ///
  /// Start a physical-disjoint path from this junction using the default width.
  /// The first point of this path has an implied short to the point of the
  /// junction id.
  ///
  void newPathShort(int junction_id, dbTechLayer* layer, dbWireType type);

  ///
  /// Start a physical-disjoint path from this junction using a non-default path
  /// width. The first point of this path has an implied short to the point of
  /// the junction id.
  ///
  void newPathShort(int              junction_id,
                    dbTechLayer*     layer,
                    dbWireType       type,
                    dbTechLayerRule* rule);

  ///
  /// Start a physical-disjoint path from this junction using the default width.
  /// This method models a virtual wire from the juntion-id to the first point
  /// of this path.
  ///
  void newPathVirtualWire(int junction_id, dbTechLayer* layer, dbWireType type);

  ///
  /// Start a physical-disjoint path from this junction using a non-default path
  /// width. This method models a virtual wire from the juntion-id to the first
  /// point of this path.
  ///
  void newPathVirtualWire(int              junction_id,
                          dbTechLayer*     layer,
                          dbWireType       type,
                          dbTechLayerRule* rule);

  ///
  /// End the encoding and apply the result to the dbWire.
  ///
  void end();

  ///
  /// Clear the encoder, no changes are applied to current wire. You can call
  /// this to abort an encoding run.
  ///
  void clear();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // Depreciated methods:
  //
  // The "newPath" methods below have been depreciated, because they do not have
  // a "dbWirePath" argument. These methods will use the last "dbWirePath" that
  // was set from a "newPath" call which had the agrument. The "lack" of the
  // dbWireType arguement is deficient and may result in the incorrect wire type
  // being applied to a wire segment.
  //
  // Use the "explict" methods declared above.
  //
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////

  ///
  /// Start a non-disjoint path from this junction using the default width and
  /// current wire type.
  ///
  void newPath(int junction_id);

  ///
  /// Start a non-disjoint path from this junction using the nondefaultrule and
  /// current wire type.
  ///
  void newPath(int junction_id, dbTechLayerRule* rule);

  ///
  /// Start a non-disjoint path from this junction using the default width and
  /// extending the path with a non-default extension and current wire type
  ///
  void newPathExt(int junction_id, int ext);

  ///
  /// Start a non-disjoint path from this junction using the default width and
  /// extending the path with a non-default extension and current wire type.
  ///
  void newPathExt(int junction_id, int ext, dbTechLayerRule* rule);
};

///
/// dbWireDecoder - This class decodes a dbWire encoded by dbWireEncoder.
///
class dbWireDecoder
{
 public:
  enum OpCode
  {
    PATH,      /// A new path
    JUNCTION,  /// A new path spawned from a previous point
    SHORT,  /// A new path offset from a previous point, implied virutal short
    VWIRE,  /// A new path spawned from a previous point, non-exsistant virtual
            /// wire from previous point to first point of path
    POINT,  /// A point on a path.
    POINT_EXT,     /// A point on a path with an extension.
    VIA,           /// A VIA instance on a path.
    TECH_VIA,      /// A TECH-VIA instance on a path with an extension.
    RECT,          /// A rect / patch wire
    ITERM,         /// A dbITerm connected to the previous point/via
    BTERM,         /// A dbBTerm connected to the previous point/via
    RULE,          /// Use non-default rule
    END_DECODE     /// No more path elements to decode.
  };

 private:
  _dbWire*     _wire;
  dbBlock*     _block;
  dbTech*      _tech;
  int          _x;
  int          _y;
  bool         _default_width;
  bool         _block_rule;
  dbTechLayer* _layer;
  int          _operand;
  int          _operand2;
  int          _idx;
  int          _jct_id;
  int          _wire_type;
  int          _point_cnt;
  OpCode       _opcode;
  uint         _property;
  int          _deltaX1;
  int          _deltaY1;
  int          _deltaX2;
  int          _deltaY2;

  unsigned char nextOp(int& value);
  unsigned char nextOp(uint& value);
  unsigned char peekOp();
  void          flushRule();

 public:
  ///
  /// Create a decoder.
  ///
  dbWireDecoder();

  ///
  /// Destroy a decoder.
  ///
  ~dbWireDecoder();

  ///
  /// Begin decoding a wire.
  ///
  void begin(dbWire* wire);

  ///
  /// Get the next OpCode. This function will return END_DECODE
  /// when there are no more OpCodes to decode.
  ///
  OpCode next();

  ///
  /// Peek at the next OpCode. This function does not change the state of
  /// the iterator.
  ///
  OpCode peek() const;

  ///
  /// Returns the current layer. The current layer is set by a
  /// PATH, JUNCTION, SHORT, VIA, or TECH_VIA OpCode.
  ///
  dbTechLayer* getLayer() const;

  ///
  /// Get value of the POINT
  ///
  void getPoint(int& x, int& y) const;

  ///
  /// Get value of the POINT_EXT.
  ///
  void getPoint(int& x, int& y, int& ext) const;

  ///
  /// Get the property of this POINT, POINT_EXT
  ///
  uint getProperty() const;

  ///
  /// Get value of the VIA OpCode.
  ///
  dbVia* getVia() const;

  ///
  /// Get value of the TECH_VIA OpCode.
  ///
  dbTechVia* getTechVia() const;

  ///
  /// Get deltas of the RECT.
  ///
  void getRect(int& deltaX1, int& deltaY1, int& deltaX2, int& deltaY2) const;

  ///
  /// Get value of the ITERM OpCode.
  ///
  dbITerm* getITerm() const;

  ///
  /// Get value of the BTERM OpCode.
  ///
  dbBTerm* getBTerm() const;

  ///
  /// Get the wire-type of a PATH, SHORT, or JUNCTION opcode.
  /// NOTE: The wire type will only change when a new PATH or SHORT opcode
  /// appears. The JUNCTION opcode does not change the wire-type.
  ///
  dbWireType getWireType() const;

  ///
  /// Get the non-default rule of the RULE opcode.
  ///
  dbTechLayerRule* getRule() const;

  ///
  /// Get the junction-id of this opcode
  ///
  int getJunctionId() const;

  ///
  /// Get the junction-id of the JUNCTION or SHORT opcode. This is the
  /// junction-id of the previous point from which this branch emerges.
  ///
  int getJunctionValue() const;
};

///
///  Utility to dump out internal representation of a net
///
void dumpDecoder(dbBlock* inblk, const char* net_name_or_id);
void dumpDecoder4Net(dbNet* innet);

}  // namespace odb
