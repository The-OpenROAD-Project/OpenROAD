// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <set>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbITerm;
class _dbBTerm;
class _dbWire;
class _dbSWire;
class _dbCapNode;
class _dbRSeg;
class _dbCCSeg;
class _dbTechNonDefaultRule;
class _dbDatabase;
class _dbGroup;
class _dbGuide;
class _dbNetTrack;
class dbIStream;
class dbOStream;
class dbNet;
class dbModNet;

struct _dbNetFlags
{
  dbSigType::Value sig_type : 4;
  dbWireType::Value wire_type : 4;
  uint32_t special : 1;
  uint32_t wild_connect : 1;
  uint32_t wire_ordered : 1;
  uint32_t unused2 : 1;       // free to reuse
  uint32_t disconnected : 1;  // this flag is only valid if wire_ordered == true
  uint32_t spef : 1;
  uint32_t select : 1;
  uint32_t mark : 1;
  uint32_t mark_1 : 1;
  uint32_t wire_altered : 1;
  uint32_t extracted : 1;
  uint32_t rc_graph : 1;
  uint32_t unused : 1;  // free to reuse
  uint32_t set_io : 1;
  uint32_t io : 1;
  uint32_t dont_touch : 1;
  uint32_t fixed_bump : 1;
  dbSourceType::Value source : 4;
  uint32_t rc_disconnected : 1;
  uint32_t block_rule : 1;
  uint32_t has_jumpers : 1;
};

class _dbNet : public _dbObject
{
 public:
  enum Field  // dbJournal field name
  {
    kFlags,
    kNonDefaultRule,
    kTermExtId,
    kHeadCapNode,
    kHeadRSeg,
    kReverseRSeg,
    kName
  };

  _dbNet(_dbDatabase*);
  _dbNet(_dbDatabase*, const _dbNet& n);
  ~_dbNet();

  bool operator==(const _dbNet& rhs) const;
  bool operator!=(const _dbNet& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbNet& rhs) const;
  void collectMemInfo(MemInfo& info);

  static void dumpConnectivityRecursive(const dbObject* obj,
                                        int max_level,
                                        int level,
                                        std::set<const dbObject*>& visited,
                                        utl::Logger* logger);
  static void dumpNetConnectivity(const dbNet* net,
                                  int max_level,
                                  int level,
                                  std::set<const dbObject*>& visited,
                                  utl::Logger* logger);
  static void dumpModNetConnectivity(const dbModNet* modnet,
                                     int max_level,
                                     int level,
                                     std::set<const dbObject*>& visited,
                                     utl::Logger* logger);

  // PERSISTANT-MEMBERS
  _dbNetFlags flags_;
  char* name_;
  union
  {
    float gndc_calibration_factor_;
    float ref_cc_;
  };
  union
  {
    float cc_calibration_factor_;
    float db_cc_;
    float cc_match_ratio_;
  };
  dbId<_dbNet> next_entry_;
  dbId<_dbITerm> iterms_;
  dbId<_dbBTerm> bterms_;
  dbId<_dbWire> wire_;
  dbId<_dbWire> global_wire_;
  dbId<_dbSWire> swires_;
  dbId<_dbCapNode> cap_nodes_;
  dbId<_dbRSeg> r_segs_;
  dbId<_dbTechNonDefaultRule> non_default_rule_;
  dbId<_dbGuide> guides_;
  dbId<_dbNetTrack> tracks_;
  dbVector<dbId<_dbGroup>> groups_;
  int weight_;
  int xtalk_;
  float cc_adjust_factor_;
  uint32_t cc_adjust_order_;
  // NON PERSISTANT-MEMBERS
  int driving_iterm_;
};

dbOStream& operator<<(dbOStream& stream, const _dbNet& net);
dbIStream& operator>>(dbIStream& stream, _dbNet& net);

}  // namespace odb
