// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

#include "odb/db.h"
#include "odb/util.h"
#include "rcx/array1.h"
#include "rcx/extRCap.h"
#include "rcx/extSegment.h"
#include "rcx/grids.h"
#include "rcx/util.h"

namespace rcx {

// Configuration settings for coupling flow

// Tracks the state during coupling flow execution
struct CouplingState
{
  // Processing counters
  uint32_t wire_count{0};         // Total wires processed
  uint32_t not_ordered_count{0};  // Count of non-ordered segments
  uint32_t empty_table_count{0};  // Count of empty tables
  uint32_t one_count_table{0};    // Count of single-entry tables

  // Reset all counters
  void reset()
  {
    wire_count = 0;
    not_ordered_count = 0;
    empty_table_count = 0;
    one_count_table = 0;
  }

  // Update counts when processing tables
  void updateTableCounts(bool hasEmptyTable, bool hasOneCount)
  {
    if (hasEmptyTable) {
      empty_table_count++;
    }
    if (hasOneCount) {
      one_count_table++;
    }
  }
};
struct CouplingConfig
{
  // Metal layer settings
  const int metal_level_count;         // Number of metal layers
  const int metal_flag;                // Metal layer control flag
  const uint32_t limit_track_num{10};  // Track limit for neighbor search

  // Length settings
  static constexpr int LENGTH_BOUND
      = 7000;               // Threshold for length-based calculations
  bool length_flag{false};  // Whether to use length-based calculations

  // Calculation modes
  bool new_calc_flow{true};   // Use new calculation flow
  bool vertical_cap{true};    // Enable vertical capacitance calculation
  bool diag_cap{true};        // Enable diagonal capacitance calculation
  bool diag_cap_power{true};  // Enable power net diagonal capacitance

  // Debug settings
  const bool debug_enabled;          // Main debug flag
  bool debug_overlaps;               // Enable overlap debugging
  FILE* debug_fp{nullptr};           // Debug file pointer
  const uint32_t progress_interval;  // Progress update interval

  // Constructor to initialize all settings
  CouplingConfig(extMain* ext_main, uint32_t levelCnt)
      : metal_level_count(levelCnt),
        metal_flag(ext_main->_metal_flag_22),
        debug_enabled(ext_main->_dbgOption > 1),
        debug_overlaps(debug_enabled),
        progress_interval(ext_main->_wire_extracted_progress_count)
  {
  }
  void reset_calc_flow_flag(uint32_t level)
  {
    if (metal_flag > 0) {
      new_calc_flow = level <= metal_flag ? true : false;
    }
  }

  // Prevent copying
  CouplingConfig(const CouplingConfig&) = delete;
  CouplingConfig& operator=(const CouplingConfig&) = delete;

  // Allow moving
  CouplingConfig(CouplingConfig&& other) noexcept
      : metal_level_count(other.metal_level_count),
        metal_flag(other.metal_flag),
        limit_track_num(other.limit_track_num),
        length_flag(other.length_flag),
        new_calc_flow(other.new_calc_flow),
        vertical_cap(other.vertical_cap),
        diag_cap(other.diag_cap),
        diag_cap_power(other.diag_cap_power),
        debug_enabled(other.debug_enabled),
        debug_overlaps(other.debug_overlaps),
        debug_fp(other.debug_fp),
        progress_interval(other.progress_interval)
  {
    other.debug_fp = nullptr;
  }
};
struct CouplingDimensionParams
{
  uint32_t direction;          // Wire direction (horizontal/vertical)
  uint32_t metal_level;        // Metal layer level
  uint32_t max_distance;       // Maximum coupling distance to consider
  uint32_t coupling_distance;  // Target coupling distance
  uint32_t track_limit;        // Maximum number of tracks to search
  FILE* dbgFP;

  // Default constructor with typical values
  CouplingDimensionParams()
      : direction(0),
        metal_level(1),
        max_distance(0),
        coupling_distance(0),
        track_limit(10),
        dbgFP(nullptr)
  {
  }

  // Full constructor
  CouplingDimensionParams(uint32_t dir,
                          uint32_t level,
                          uint32_t maxDist,
                          uint32_t coupDist,
                          uint32_t limitTrack,
                          FILE* fp)
      : direction(dir),
        metal_level(level),
        max_distance(maxDist),
        coupling_distance(coupDist),
        track_limit(limitTrack),
        dbgFP(fp)

  {
  }

  // Create params with adjusted track limit
  CouplingDimensionParams withTrackLimit(uint32_t new_limit) const
  {
    return CouplingDimensionParams(direction,
                                   metal_level,
                                   max_distance,
                                   coupling_distance,
                                   new_limit,
                                   dbgFP);
  }

  // Create params for next metal level
  CouplingDimensionParams nextLevel() const
  {
    return CouplingDimensionParams(direction,
                                   metal_level + 1,
                                   max_distance,
                                   coupling_distance,
                                   track_limit,
                                   dbgFP);
  }

  // Create params with new distances
  CouplingDimensionParams withDistances(uint32_t maxDist,
                                        uint32_t coupDist) const
  {
    return CouplingDimensionParams(
        direction, metal_level, maxDist, coupDist, track_limit, dbgFP);
  }

  // Utility method to calculate if within distance bounds
  bool isWithinDistance(uint32_t distance) const
  {
    return distance <= max_distance;
  }

  // String representation for debugging
  std::string toString() const
  {
    return "Dir: " + std::to_string(direction)
           + " Level: " + std::to_string(metal_level)
           + " MaxDist: " + std::to_string(max_distance)
           + " CoupDist: " + std::to_string(coupling_distance)
           + " TrackLimit: " + std::to_string(track_limit);
  }
};

struct BoundaryData;

class SegmentTables
{
 public:
  Array1D<extSegment*> upTable;
  Array1D<extSegment*> downTable;
  Array1D<extSegment*> verticalUpTable;
  Array1D<extSegment*> verticalDownTable;
  Array1D<extSegment*> wireSegmentTable;
  Array1D<extSegment*> aboveTable;
  Array1D<extSegment*> belowTable;
  Array1D<extSegment*> whiteTable;

  // Default constructor - tables are auto-initialized
  SegmentTables() = default;

  // Reset all tables
  void resetAll()
  {
    upTable.resetCnt();
    downTable.resetCnt();
    verticalUpTable.resetCnt();
    verticalDownTable.resetCnt();
    wireSegmentTable.resetCnt();
    aboveTable.resetCnt();
    belowTable.resetCnt();
    whiteTable.resetCnt();
  }

  // Release memory for all segments in all tables
  void releaseAll()
  {
    Release(&upTable);
    Release(&downTable);
    Release(&verticalUpTable);
    Release(&verticalDownTable);
    Release(&wireSegmentTable);
    Release(&aboveTable);
    Release(&belowTable);
    Release(&whiteTable);
  }

 private:
  // Helper function to release segments from a single table
  static void Release(Array1D<extSegment*>* table)
  {
    for (uint32_t i = 0; i < table->getCnt(); i++) {
      delete table->get(i);
    }
    table->resetCnt();
  }
};
class extMeasureRC : public extMeasure
{
 public:
  // TODO std::unique_ptr<ExtProgressTracker> _progressTracker;
  extMeasureRC(utl::Logger* logger) : extMeasure(logger) {}
  FILE* _connect_wire_FP = nullptr;
  FILE* _connect_FP = nullptr;
  // Not dynamic arrays for debugging conveniencex
  uint32_t _trackLevelCnt = 32;
  uint32_t _lowTrackToExtract[2][32];  // 32 is the max layer level
  uint32_t _hiTrackToExtract[2][32];
  uint32_t _lowTrackToFree[2][32];
  uint32_t _hiTrackToFree[2][32];
  uint32_t _lowTrackSearch[2][32];
  uint32_t _hiTrackSearch[2][32];

  void resetTrackIndices(uint32_t dir);
  int ConnectWires(uint32_t dir, BoundaryData& bounds);
  int FindCouplingNeighbors(uint32_t dir, BoundaryData& bounds);
  int FindCouplingNeighbors_down_opt(uint32_t dir, BoundaryData& bounds);
  int FindDiagonalNeighbors_vertical_up_opt(uint32_t dir,
                                            uint32_t couplingDist,
                                            uint32_t diag_met_limit,
                                            uint32_t lookUpLevel,
                                            uint32_t limitTrackNum,
                                            bool skipCheckNeighbors);
  int FindDiagonalNeighbors_vertical_down_opt(uint32_t dir,
                                              uint32_t couplingDist,
                                              uint32_t diag_met_limit,
                                              uint32_t lookUpLevel,
                                              uint32_t limitTrackNum,
                                              bool skipCheckNeighbors);
  int CouplingFlow_opt(uint32_t dir,
                       BoundaryData& bounds,
                       int totWireCnt,
                       uint32_t& totalWiresExtracted,
                       float& previous_percent_extracted);
  //----------------------------------------------------------------------- v2
  //----- CLEANUP
  AthPool<extSegment>* _seqmentPool;
  void releaseAll(SegmentTables& segments);

  void allocateTables(uint32_t colCnt);
  void de_allocateTables(uint32_t colCnt);
  Array1D<Wire*>** allocTable_wire(uint32_t n);
  void DeleteTable_wire(Array1D<Wire*>** tbl, uint32_t n);
  uint32_t GetCoupleSegments(bool lookUp,
                             Wire* w,
                             uint32_t start_track,
                             CouplingDimensionParams& coupleOptions,
                             Array1D<Wire*>** firstWireTable,
                             Array1D<extSegment*>* UpSegTable);
  uint32_t FindCoupleWiresOnTracks_down(Wire* w,
                                        int start_track,
                                        CouplingDimensionParams& coupleOptions,
                                        Array1D<Wire*>** firstWireTable,
                                        Array1D<Wire*>* resTable);

  uint32_t FindCoupleWiresOnTracks_up(Wire* w,
                                      uint32_t start_track,
                                      CouplingDimensionParams& coupleOptions,
                                      Array1D<Wire*>** firstWireTable,
                                      Array1D<Wire*>* resTable);
  uint32_t makeCoupleSegments_up(Wire* w,
                                 uint32_t start_track,
                                 CouplingDimensionParams& coupleOptions,
                                 Array1D<Wire*>** firstWireTable,
                                 Array1D<extSegment*>* UpSegTable);

  bool FindDiagonalCoupleSegments(Wire* w,
                                  int current_level,
                                  int max_level,
                                  CouplingDimensionParams& opts,
                                  Array1D<Wire*>** firstWireTable);
  bool VerticalDiagonalCouplingAndCrossOverlap(Wire* w,
                                               extSegment* s,
                                               int overMet,
                                               SegmentTables& segments,
                                               CouplingConfig& config);
  bool CreateCouplingCaps_overUnder(extSegment* s, uint32_t overMet);
  bool CreateCouplingCaps_over(extSegment* s, uint32_t metalLevelCnt);
  void ReleaseSegTables(uint32_t metalLevelCnt);
  bool GetCouplingSegments(int tr,
                           Wire* w,
                           CouplingConfig& config,
                           CouplingDimensionParams& coupleOptions,
                           SegmentTables& segments,
                           Array1D<Wire*>** firstWireTable);

  int _ll_tgt[2];
  int _ur_tgt[2];
  int _diagResLen;

  int _diagResDist;
  bool _useWeighted = false;
  bool DebugDiagCoords(int met,
                       int targetMet,
                       int len1,
                       int diagDist,
                       int ll[2],
                       int ur[2]);

  extDistRC* getDiagUnderCC(extMetRCTable* rcModel,
                            uint32_t dist,
                            uint32_t overMet);
  uint32_t CalcDiag(uint32_t targetMet,
                    uint32_t diagDist,
                    uint32_t tgWidth,
                    uint32_t len1,
                    extSegment* s,
                    int rsegId);

  FILE* OpenFile(const char* name, const char* perms);
  FILE* OpenPrintFile(uint32_t dir, const char* name);
  void Release(Array1D<extSegment*>* segTable);

  void PrintCrossSeg(FILE* fp,
                     int x1,
                     int x2,
                     int met,
                     int metOver,
                     int metUnder,
                     const char* prefix = "");
  void GetOUname(char buf[200], int met, int metOver, int metUnder);
  void PrintCrossOvelaps(Wire* w,
                         uint32_t tgt_met,
                         int x1,
                         int x2,
                         Array1D<extSegment*>* segTable,
                         int totLen,
                         const char* prefix,
                         int metOver = -1,
                         int metUnder = -1);

  // dkf 09212023
  void OverSubRC_dist_new(odb::dbRSeg* rseg1,
                          odb::dbRSeg* rseg2,
                          int ouCovered,
                          int diagCovered,
                          int srcCovered);
  // dkf 10212023
  void PrintCrossOvelapsOU(Wire* w,
                           uint32_t tgt_met,
                           int x1,
                           int len,
                           Array1D<extSegment*>* segTable,
                           int totLen,
                           const char* prefix,
                           int metOver,
                           int metUnder);

  // dkf 10232023
  void PrintOverlapSeg(FILE* fp,
                       extSegment* s,
                       int tgt_met,
                       const char* prefix);
  void PrintOvelaps(extSegment* w,
                    uint32_t met,
                    uint32_t tgt_met,
                    Array1D<extSegment*>* segTable,
                    const char* ou);
  void PrintOUSeg(FILE* fp,
                  int x1,
                  int len,
                  int met,
                  int metOver,
                  int metUnder,
                  const char* prefix,
                  int up_dist,
                  int down_dist);

  // DKF 9142023
  float getOverR_weightedFringe(extMetRCTable* rcModel,
                                uint32_t width,
                                int met,
                                int metUnder,
                                int dist1,
                                int dist2);
  float getUnderRC_weightedFringe(extMetRCTable* rcModel,
                                  uint32_t width,
                                  int met,
                                  int metOver,
                                  int dist1,
                                  int dist2);
  float getOverUnderRC_weightedFringe(extMetRCTable* rcModel,
                                      uint32_t width,
                                      int met,
                                      int underMet,
                                      int metOver,
                                      int dist1,
                                      int dist2);
  extDistRC* getOverRC_Dist(extMetRCTable* rcModel,
                            uint32_t width,
                            int met,
                            int metUnder,
                            int dist,
                            int open = -1);
  extDistRC* getUnderRC_Dist(extMetRCTable* rcModel,
                             uint32_t width,
                             int met,
                             int metOver,
                             int dist,
                             int open = -1);
  extDistRC* getOverUnderRC_Dist(extMetRCTable* rcModel,
                                 int width,
                                 int met,
                                 int underMet,
                                 int overMet,
                                 int dist,
                                 int open = -1);
  static int getMetIndexOverUnder(int met,
                                  int mUnder,
                                  int mOver,
                                  int layerCnt,
                                  int maxCnt = 10000);
  // DKF 9202023
  extDistRC* getOverOpenRC_Dist(extMetRCTable* rcModel,
                                uint32_t width,
                                int met,
                                int metUnder,
                                int dist);
  float getOverRC_Open(extMetRCTable* rcModel,
                       uint32_t width,
                       int met,
                       int metUnder,
                       int dist1,
                       int dist2);
  extDistRC* addRC_new(extDistRC* rcUnit,
                       uint32_t len,
                       uint32_t jj,
                       bool addCC);
  float getOURC_Open(extMetRCTable* rcModel,
                     uint32_t width,
                     int met,
                     int metUnder,
                     int metOver,
                     int dist1,
                     int dist2);
  // DKF 9232023
  float getOver_over1(extMetRCTable* rcModel,
                      uint32_t width,
                      int met,
                      int metUnder,
                      int dist1,
                      int dist2,
                      int lenOverSub);
  extDistRC* computeOverUnderRC(extMetRCTable* rcModel, uint32_t len);
  float getOU_over1(extMetRCTable* rcModel,
                    int lenOverSub,
                    int dist1,
                    int dist2);

  // --------------- dkf 09142023
  bool measureRC_res_dist(Array1D<SEQ*>* tmpTable);
  void measureRC_ids_flags(CoupleOptions& options);  // dkf 09142023
  void measureRC_091423(CoupleOptions& options);     // dkf 09142023
  void measureRC(CoupleOptions& options);
  float getOverR_weightedFringe(extMetRCTable* rcModel, int dist1, int dist2);
  // dimitri 09172023
  float weightedFringe(extDistRC* rc1,
                       extDistRC* rc2,
                       bool use_weighted = true);
  bool useWeightedAvg(int& dist1, int& dist2, int underMet);
  int computeAndStoreRC_new(odb::dbRSeg* rseg1,
                            odb::dbRSeg* rseg2,
                            int srcCovered);
  //-----------------------------------------------------------
  bool updateCoupCap(odb::dbRSeg* rseg1,
                     odb::dbRSeg* rseg2,
                     int jj,
                     double v,
                     const char* dbg_msg);

  // dkf 09272023
  FILE* OpenDebugFile();
  bool DebugStart(FILE* fp, bool allNets = false);
  bool DebugEnd(FILE* fp, int OU_covered);
  const char* srcWord(int& rsegId);
  void printNetCaps(FILE* fp, const char* msg);
  void printCaps(FILE* fp,
                 double totCap,
                 double cc,
                 double fr,
                 double dfr,
                 double res,
                 const char* msg);
  void PrintCoords(FILE* fp, int x, int y, const char* xy);
  void PrintCoord(FILE* fp, int x, const char* xy);
  void PrintCoords(FILE* fp, int ll[2], const char* xy);
  bool PrintCurrentCoords(FILE* fp, const char* msg, uint32_t rseg);
  void segInfo(FILE* fp, const char* msg, uint32_t netId, int rsegId);
  double getCC(int rsegId);
  void DebugStart_res(FILE* fp);
  void DebugRes_calc(FILE* fp,
                     const char* msg,
                     int rsegId1,
                     const char* msg_len,
                     uint32_t len,
                     int dist1,
                     int dist2,
                     int tgtMet,
                     double tot,
                     double R,
                     double unit,
                     double prev);
  bool DebugDiagCoords(FILE* fp,
                       int met,
                       int targetMet,
                       int len1,
                       int diagDist,
                       int ll[2],
                       int ur[2],
                       const char* msg);
  void DebugEnd_res(FILE* fp, int rseg1, int len_covered, const char* msg);

  void GetPatName(int met, int overMet, int underMet, char tmp[50]);
  void printDebugRC(FILE* fp,
                    extDistRC* rc,
                    const char* msg,
                    const char* post = "");
  void printDebugRC(FILE* fp,
                    extDistRC* rc,
                    int met,
                    int overMet,
                    int underMet,
                    int width,
                    int dist,
                    int len,
                    const char* msg);
  void printDebugRC_diag(FILE* fp,
                         extDistRC* rc,
                         int met,
                         int overMet,
                         int underMet,
                         int width,
                         int dist,
                         int len,
                         const char* msg);
  // dkf 09292023
  void DebugUpdateValue(FILE* fp,
                        const char* msg,
                        const char* cap_type,
                        int rsegId,
                        double v,
                        double updated_v);
  void DebugPrintNetids(FILE* fp,
                        const char* msg,
                        int rsegId,
                        const char* eol = "\n");
  void DebugUpdateCC(FILE* fp,
                     const char* msg,
                     int rsegId,
                     int rseg2,
                     double v,
                     double tot);
  bool DebugCoords(FILE* fp, int rsegId, int ll[2], int ur[2], const char* msg);
  const char* GetSrcWord(int rsegId);
  void ResetRCs();

  //----------------------------------------------------------------------- v2

  // dkf 101052024 ---------------------
  uint32_t createContextGrid_dir(char* dirName,
                                 const int bboxLL[2],
                                 const int bboxUR[2],
                                 int met);
  // DKF 7/25/2024 -- 3d pattern generation
  int _simVersion;

  uint32_t FindSegments(bool lookUp,
                        uint32_t dir,
                        int maxDist,
                        Wire* w1,
                        int xy1,
                        int len1,
                        Wire* w2_next,
                        Array1D<extSegment*>* segTable);

  uint32_t FindSegments_org(bool lookUp,
                            uint32_t dir,
                            int maxDist,
                            Wire* w1,
                            int xy1,
                            int len1,
                            Wire* w2,
                            Array1D<extSegment*>* segTable);
  int GetDx1Dx2(int xy1, int len1, extSegment* w2, int& dx2);
  int GetDx1Dx2(Wire* w1, Wire* w2, int& dx2);
  int GetDx1Dx2(int xy1, int len1, Wire* w2, int& dx2);
  int GetDistance(Wire* w1, Wire* w2);

  // dkf 10162023
  bool OverlapOnly(int xy1, int len1, int xy2, int len2);
  bool Enclosed(int x1, int x2, int y1, int y2);

  // dkf 10202023
  // FILE *_segFP;

  // ------------------------------------------------------------------- v2

  // dkf 10012023
  int FindCouplingNeighbors(uint32_t dir,
                            uint32_t couplingDist,
                            uint32_t diag_met_limit);
  int FindCouplingNeighbors_down(uint32_t dir,
                                 uint32_t couplingDist,
                                 uint32_t diag_met_limit);
  void PrintCoupingNeighbors(FILE* fp, uint32_t upCount, uint32_t downCount);
  void PrintWire(FILE* fp,
                 Wire* w,
                 int level,
                 const char* prefix = "",
                 const char* postfix = "");
  void Print5wires(FILE* fp, Wire* w, uint32_t level = 0);
  Wire* FindOverlap(Wire* w, Wire* first_wire);
  void ResetFirstWires(Grid* netGrid,
                       Array1D<Wire*>* firstWireTable,
                       int tr1,
                       int trCnt,
                       uint32_t limitTrackNum);
  // dkf 10022023
  Wire* FindOverlap(Wire* w, Array1D<Wire*>* firstWireTable, int tr);
  // dkf 10032023
  int FindDiagonalNeighbors(uint32_t dir,
                            uint32_t couplingDist,
                            uint32_t diag_met_limit,
                            uint32_t lookUpLevel,
                            uint32_t limitTrackNum);
  bool IsSegmentOverlap(int x1, int len1, int x2, int len2);
  bool IsOverlap(Wire* w, Wire* w2);
  Wire* GetNextWire(Grid* netGrid, uint32_t tr, Array1D<Wire*>* firstWireTable);
  Wire* FindOverlap(Wire* w,
                    Grid* netGrid,
                    uint32_t tr,
                    Array1D<Wire*>* firstWireTable);
  bool CheckWithNeighbors(Wire* w, Wire* prev);
  Array1D<Wire*>** allocMarkTable(uint32_t n);
  void DeleteMarkTable(Array1D<Wire*>** tbl, uint32_t n);
  void ResetFirstWires(uint32_t m1,
                       uint32_t m2,
                       uint32_t dir,
                       Array1D<Wire*>** firstWireTable);
  int PrintAllGrids(uint32_t dir, FILE* fp, uint32_t mode);

  // dkf 10042023
  void PrintDiagwires(FILE* fp, Wire* w, uint32_t level);
  int CouplingFlow_new(uint32_t dir,
                       uint32_t couplingDist,
                       uint32_t diag_met_limit);

  // dkf 10052023
  void Print(FILE* fp, Array1D<extSegment*>* segTable, uint32_t d, bool lookUp);
  void Print(FILE* fp, extSegment* s, uint32_t d, bool lookUp);
  // void Release(Array1D<extSegment *> *segTable);

  // dkf 10062023
  bool CheckOrdered(Array1D<extSegment*>* segTable);
  bool measure_RC_new(int met,
                      uint32_t dir,
                      extSegment* up,
                      extSegment* down,
                      int xy1,
                      int len);
  bool measure_init(int met,
                    uint32_t dir,
                    extSegment* up,
                    extSegment* down,
                    int xy1,
                    int len);

  // dkf 10072023
  extSegment* CreateUpDownSegment(Wire* w,
                                  Wire* up,
                                  int xy1,
                                  int len1,
                                  Wire* down,
                                  Array1D<extSegment*>* segTable,
                                  int metOver = -1,
                                  int metUnder = -1);
  uint32_t FindUpDownSegments(Array1D<extSegment*>* upTable,
                              Array1D<extSegment*>* downTable,
                              Array1D<extSegment*>* segTable,
                              int metOver = -1,
                              int metUnder = -1);
  extSegment* GetNext(uint32_t ii,
                      int& xy1,
                      int& len1,
                      Array1D<extSegment*>* segTable);
  extSegment* GetNextSegment(uint32_t ii, Array1D<extSegment*>* segTable);
  uint32_t CopySegments(bool up,
                        Array1D<extSegment*>* upTable,
                        uint32_t start,
                        uint32_t end,
                        Array1D<extSegment*>* segTable,
                        int maxDist = 1000000000,
                        int metOver = -1,
                        int metUnder = -1);
  void PrintUpDown(FILE* fp, extSegment* s);
  void PrintUpDownNet(FILE* fp, Wire* s, int dist, const char* prefix);
  void PrintUpDown(FILE* fp, Array1D<extSegment*>* segTable);
  void BubbleSort(Array1D<extSegment*>* segTable);
  bool measure_init(extSegment* s);
  bool measure_RC_new(extSegment* s,
                      bool skip_res_calc = false);  // dkf 06182024
  // dkf 10082023
  bool measureRC_res_dist_new(extSegment* s);
  bool measureRC_res_init(uint32_t rsegId);
  bool measure_init_cap(extSegment* s, bool up);
  extSegment* _currentSeg;
  bool _newDiagFlow;
  // dkf 10092023
  int ConnectWires(uint32_t dir);
  // uint32_t CalcDiag( uint32_t targetMet, uint32_t diagDist, uint32_t tgWidth,
  // uint32_t len1, extSegment *s, int rsegId); dkf 10102023
  int FindDiagonalNeighbors_down(uint32_t dir,
                                 uint32_t couplingDist,
                                 uint32_t diag_met_limit,
                                 uint32_t lookUpLevel,
                                 uint32_t limitTrackNum);
  bool CheckWithNeighbors_below(Wire* w, Wire* prev);
  uint32_t CalcDiagBelow(extSegment* s, Wire* dw);
  // dkf 10112023
  int FindDiagonalNeighbors_vertical_up(uint32_t dir,
                                        uint32_t couplingDist,
                                        uint32_t diag_met_limit,
                                        uint32_t lookUpLevel,
                                        uint32_t limitTrackNum,
                                        bool skipCheckNeighbors);
  int FindDiagonalNeighbors_vertical_power(uint32_t dir,
                                           Wire* w,
                                           uint32_t couplingDist,
                                           uint32_t diag_met_limit,
                                           uint32_t limitTrackNum,
                                           Array1D<Wire*>** upWireTable);
  void Print(FILE* fp, Array1D<Wire*>* segTable, const char* msg);
  // dkf 10122023
  Array1D<Wire*>** _verticalPowerTable;

  // dkf 10132023
  // uint32_t FindSegments(bool lookUp, uint32_t dir, int maxDist, Wire *w1, int
  // xy1, int len1, Wire *w2, Array1D<extSegment *> *segTable);

  // dkf 10152023
  Wire* FindDiagonalNeighbors_vertical_up_down(Wire* w,
                                               bool& found,
                                               uint32_t dir,
                                               uint32_t level,
                                               uint32_t couplingDist,
                                               uint32_t limitTrackNum,
                                               Array1D<Wire*>** firstWireTable);
  int FindDiagonalNeighbors_vertical_down(uint32_t dir,
                                          uint32_t couplingDist,
                                          uint32_t diag_met_limit,
                                          uint32_t lookUpLevel,
                                          uint32_t limitTrackNum,
                                          bool skipCheckNeighbors);

  // dkf 10162023
  Wire* FindOverlap_found(Wire* w, Wire* first_wire, bool& found);
  Wire* SetUpDown(Wire* w2,
                  int next_tr,
                  bool found,
                  Wire* first_wire,
                  Array1D<Wire*>* firstWireTable);
  uint32_t FindAllNeigbors_up(Wire* w,
                              uint32_t start_track,
                              uint32_t dir,
                              uint32_t level,
                              uint32_t couplingDist,
                              uint32_t limitTrackNum,
                              Array1D<Wire*>** firstWireTable,
                              Array1D<Wire*>* resTable);
  Wire* FindOverlapWire(Wire* w, Wire* first_wire);

  // dkf 061824
  int CouplingFlow(uint32_t dir,
                   uint32_t couplingDist,
                   uint32_t diag_met_limit,
                   int totWireCnt,
                   uint32_t& totalWiresExtracted,
                   float& previous_percent_extracted);
  // dkf 10172023
  // dkf 061824 int CouplingFlow(uint32_t dir, uint32_t couplingDist, uint32_t
  // diag_met_limit);

  extSegment* CreateUpDownSegment(bool lookUp,
                                  Wire* w,
                                  int xy1,
                                  int len1,
                                  Wire* w2,
                                  Array1D<extSegment*>* segTable);
  void FindSegmentsTrack(Wire* w1,
                         int xy1,
                         int len1,
                         Wire* w2_next,
                         uint32_t ii,
                         Array1D<Wire*>* trackTable,
                         bool lookUp,
                         uint32_t dir,
                         int maxDist,
                         Array1D<extSegment*>* segTable);
  uint32_t FindAllNeigbors_down(Wire* w,
                                int start_track,
                                uint32_t dir,
                                uint32_t level,
                                uint32_t couplingDist,
                                uint32_t limitTrackNum,
                                Array1D<Wire*>** firstWireTable,
                                Array1D<Wire*>* resTable);
  bool PrintInit(FILE* fp, bool dbgOverlaps, Wire* w, int x, int y);
  void PrintTable_coupleWires(FILE* fp1,
                              Wire* w,
                              bool dbgOverlaps,
                              Array1D<Wire*>* UpTable,
                              const char* msg,
                              int level = -1);
  void PrintTable_segments(FILE* fp1,
                           Wire* w,
                           bool lookUp,
                           bool dbgOverlaps,
                           Array1D<extSegment*>* UpSegTable,
                           const char* msg,
                           int level = -1);
  bool DebugWire(Wire* w, int x, int y, int netId = -1);
  uint32_t CreateCouplingSEgments(Wire* w,
                                  Array1D<extSegment*>* segTable,
                                  Array1D<extSegment*>* upTable,
                                  Array1D<extSegment*>* downTable,
                                  bool dbgOverlaps,
                                  FILE* fp);
  void PrintTable_wires(FILE* fp,
                        bool dbgOverlaps,
                        uint32_t colCnt,
                        Array1D<Wire*>** verticalPowerTable,
                        const char* msg);

  // dkf 10182023
  Array1D<extSegment*>** _upSegTable = nullptr;
  Array1D<extSegment*>** _downSegTable = nullptr;
  Array1D<extSegment*>** allocTable(uint32_t n);
  void DeleteTable(Array1D<extSegment*>** tbl, uint32_t n);
  uint32_t FindAllSegments_up(FILE* fp,
                              Wire* w,
                              bool lookUp,
                              uint32_t start_track,
                              uint32_t dir,
                              uint32_t level,
                              uint32_t maxDist,
                              uint32_t couplingDist,
                              uint32_t limitTrackNum,
                              Array1D<Wire*>** firstWireTable,
                              Array1D<extSegment*>** UpSegTable);
  uint32_t FindAllSegments_vertical(FILE* fp,
                                    Wire* w,
                                    bool lookUp,
                                    uint32_t dir,
                                    uint32_t maxDist,
                                    Array1D<extSegment*>* aboveTable);

  // dkf 10192023
  odb::dbRSeg* GetRseg(int id);
  bool VerticalCap(uint32_t met,
                   uint32_t tgtMet,
                   int rsegId1,
                   uint32_t rsegId2,
                   uint32_t len,
                   uint32_t width,
                   uint32_t tgtWidth,
                   uint32_t diagDist);
  void VerticalCap(Array1D<extSegment*>* segTable, bool look_up);
  bool DiagCap(FILE* fp,
               Wire* w,
               bool lookUp,
               uint32_t maxDist,
               uint32_t trackLimitCnt,
               Array1D<extSegment*>* segTable,
               bool PowerOnly = false);
  bool DiagCouplingCap(uint32_t met,
                       uint32_t tgtMet,
                       int rsegId1,
                       uint32_t rsegId2,
                       uint32_t len,
                       uint32_t width,
                       uint32_t tgtWidth,
                       uint32_t diagDist);

  // dkf 10202023
  FILE* _segFP;
  Array1D<extSegment*>** _ovSegTable = nullptr;
  Array1D<extSegment*>** _whiteSegTable = nullptr;

  // void PrintCrossSeg(FILE *fp, int x1, int x2, int met, int metOver, int
  // metUnder, const char *prefix="");
  //  void GetOUname(char buf[20], int met, int metOver, int metUnder);
  bool GetCrossOvelaps(Wire* w,
                       uint32_t tgt_met,
                       int x1,
                       int x2,
                       uint32_t dir,
                       Array1D<extSegment*>* segTable,
                       Array1D<extSegment*>* whiteTable);
  // void PrintCrossOvelaps(Wire *w, uint32_t tgt_met, int x1, int x2,
  // Array1D<extSegment *> *segTable, int totLen, const char *prefix,
  // int metOver=-1, int metUnder=-1);

  // dkf 10212023
  // void PrintCrossOvelapsOU(Wire *w, uint32_t tgt_met, int x1, int len,
  // Array1D<extSegment *> *segTable, int totLen, const char *prefix,
  // int metOver, int metUnder);

  // dkf 10232023
  // void PrintOverlapSeg(FILE *fp, extSegment *s, int tgt_met, const char
  // *prefix); void PrintOvelaps(extSegment *w, uint32_t met, uint32_t tgt_met,
  // Array1D<extSegment *> *segTable, const char *ou); void
  // PrintOUSeg(FILE *fp, int x1, int len, int met, int metOver, int metUnder,
  // const char *prefix, int up_dist, int down_dist);
  void OverUnder(extSegment* cc,
                 uint32_t met,
                 int overMet,
                 int underMet,
                 Array1D<extSegment*>* segTable,
                 const char* ou);
  void OpenEnded2(extSegment* cc,
                  uint32_t len,
                  int met,
                  int overMet,
                  int underMet,
                  FILE* segFP);
  void OpenEnded1(extSegment* cc,
                  uint32_t len,
                  int met,
                  int overMet,
                  int underMet,
                  FILE* segFP);
  void Model1(extSegment* cc,
              uint32_t len,
              int met,
              int metUnder,
              int metOver,
              FILE* segFP);
  void OverUnder(extSegment* cc,
                 uint32_t len,
                 int met,
                 int metUnder,
                 int metOver,
                 FILE* segFP);

  // dkf 10242023
  extDistRC* OverUnderRC(extMetRCTable* rcModel,
                         int open,
                         uint32_t width,
                         int dist,
                         uint32_t len,
                         int met,
                         int metUnder,
                         int metOver,
                         FILE* segFP);
  odb::dbRSeg* GetRSeg(extSegment* cc);
  odb::dbRSeg* GetRSeg(uint32_t rsegId);
  double updateCoupCap(odb::dbRSeg* rseg1,
                       odb::dbRSeg* rseg2,
                       int jj,
                       double v);
  void OverlapDown(int overMet,
                   extSegment* coupSeg,
                   extSegment* overlapSeg,
                   uint32_t dir);

  // dkf 10252023
  int wireOverlap(int X1,
                  int DX,
                  int x1,
                  int dx,
                  int* len1,
                  int* len2,
                  int* len3);
  bool FindDiagonalSegments(extSegment* s,
                            extSegment* ww,
                            Array1D<extSegment*>* segDiagTable,
                            Array1D<extSegment*>* resultTable,
                            bool dbgOverlaps,
                            FILE* fp,
                            bool lookUp,
                            int tgt_met = -1);

  // dkf 10302023
  bool CalcRes(extSegment* s);

  // dkf 11012023
  uint32_t ConnectAllWires(Track* track);

  // dkf 061824
  bool printProgress(uint32_t totalWiresExtracted,
                     uint32_t totWireCnt,
                     float& previous_percent_extracted);
};

}  // namespace rcx
