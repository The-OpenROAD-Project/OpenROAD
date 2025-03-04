///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, IC BENCH, Dimitris Fotakis
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

#include "extRCap.h"

namespace rcx {

using namespace odb;

// Configuration settings for coupling flow

// Tracks the state during coupling flow execution
struct CouplingState
{
  // Processing counters
  uint wire_count;         // Total wires processed
  uint not_ordered_count;  // Count of non-ordered segments
  uint empty_table_count;  // Count of empty tables
  uint one_count_table;    // Count of single-entry tables

  // Constructor initializes all counters to 0
  CouplingState()
      : wire_count(0),
        not_ordered_count(0),
        empty_table_count(0),
        one_count_table(0)
  {
  }

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
    if (hasEmptyTable)
      empty_table_count++;
    if (hasOneCount)
      one_count_table++;
  }

  // Print statistics
  void printStats(FILE* fp, uint dir) const
  {
    if (fp) {
      fprintf(fp,
              "\nDir=%d  wireCnt=%d  NotOrderedCnt=%d  oneEmptyTable=%d  "
              "oneCntTable=%d\n",
              dir,
              wire_count,
              not_ordered_count,
              empty_table_count,
              one_count_table);
    }
  }
};
struct CouplingConfig
{
  // Metal layer settings
  const int metal_level_count;  // Number of metal layers
  const int metal_flag;         // Metal layer control flag
  const uint limit_track_num;   // Track limit for neighbor search

  // Length settings
  static constexpr int LENGTH_BOUND
      = 7000;        // Threshold for length-based calculations
  bool length_flag;  // Whether to use length-based calculations

  // Calculation modes
  bool new_calc_flow;   // Use new calculation flow
  bool vertical_cap;    // Enable vertical capacitance calculation
  bool diag_cap;        // Enable diagonal capacitance calculation
  bool diag_cap_power;  // Enable power net diagonal capacitance

  // Debug settings
  const bool debug_enabled;      // Main debug flag
  bool debug_overlaps;           // Enable overlap debugging
  FILE* debug_fp;                // Debug file pointer
  const uint progress_interval;  // Progress update interval

  // Constructor to initialize all settings
  CouplingConfig(extMain* ext_main, uint levelCnt)
      : metal_level_count(levelCnt),
        metal_flag(ext_main->_metal_flag_22),
        limit_track_num(10),
        length_flag(false),
        new_calc_flow(true),
        vertical_cap(true),
        diag_cap(true),
        diag_cap_power(true),
        debug_enabled(ext_main->_dbgOption > 1),
        debug_overlaps(debug_enabled),
        debug_fp(nullptr),
        progress_interval(ext_main->_wire_extracted_progress_count)
  {
  }
  void reset_calc_flow_flag(uint level)
  {
    if (metal_flag > 0)
      new_calc_flow = level <= metal_flag ? true : false;
  }
  // Destructor to clean up resources
  ~CouplingConfig() {}

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
  uint direction;          // Wire direction (horizontal/vertical)
  uint metal_level;        // Metal layer level
  uint max_distance;       // Maximum coupling distance to consider
  uint coupling_distance;  // Target coupling distance
  uint track_limit;        // Maximum number of tracks to search
  FILE* dbgFP;

  // Default constructor with typical values
  CouplingDimensionParams()
      : direction(0),
        metal_level(1),
        max_distance(0),
        coupling_distance(0),
        track_limit(10),
        dbgFP(NULL)
  {
  }

  // Full constructor
  CouplingDimensionParams(uint dir,
                          uint level,
                          uint maxDist,
                          uint coupDist,
                          uint limitTrack,
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
  CouplingDimensionParams withTrackLimit(uint new_limit) const
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
  CouplingDimensionParams withDistances(uint maxDist, uint coupDist) const
  {
    return CouplingDimensionParams(
        direction, metal_level, maxDist, coupDist, track_limit, dbgFP);
  }

  // Utility method to calculate if within distance bounds
  bool isWithinDistance(uint distance) const
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
  Ath__array1D<extSegment*> upTable;
  Ath__array1D<extSegment*> downTable;
  Ath__array1D<extSegment*> verticalUpTable;
  Ath__array1D<extSegment*> verticalDownTable;
  Ath__array1D<extSegment*> wireSegmentTable;
  Ath__array1D<extSegment*> aboveTable;
  Ath__array1D<extSegment*> belowTable;
  Ath__array1D<extSegment*> whiteTable;

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
  static void Release(Ath__array1D<extSegment*>* table)
  {
    for (uint i = 0; i < table->getCnt(); i++) {
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
  uint _trackLevelCnt = 32;
  uint _lowTrackToExtract[2][32];  // 32 is the max layer level
  uint _hiTrackToExtract[2][32];
  uint _lowTrackToFree[2][32];
  uint _hiTrackToFree[2][32];
  uint _lowTrackSearch[2][32];
  uint _hiTrackSearch[2][32];

  void resetTrackIndices(uint dir);
  int ConnectWires(uint dir, BoundaryData& bounds);
  int FindCouplingNeighbors(uint dir, BoundaryData& bounds);
  int FindCouplingNeighbors_down_opt(uint dir, BoundaryData& bounds);
  int FindDiagonalNeighbors_vertical_up_opt(uint dir,
                                            uint couplingDist,
                                            uint diag_met_limit,
                                            uint lookUpLevel,
                                            uint limitTrackNum,
                                            bool skipCheckNeighbors);
  int FindDiagonalNeighbors_vertical_down_opt(uint dir,
                                              uint couplingDist,
                                              uint diag_met_limit,
                                              uint lookUpLevel,
                                              uint limitTrackNum,
                                              bool skipCheckNeighbors);
  int CouplingFlow_opt(uint dir,
                       BoundaryData& bounds,
                       int totWireCnt,
                       uint& totalWiresExtracted,
                       float& previous_percent_extracted);
  //----------------------------------------------------------------------- v2
  //----- CLEANUP
  AthPool<extSegment>* _seqmentPool;
  void releaseAll(SegmentTables& segments);

  void allocateTables(uint colCnt);
  void de_allocateTables(uint colCnt);
  Ath__array1D<Wire*>** allocTable_wire(uint n);
  void DeleteTable_wire(Ath__array1D<Wire*>** tbl, uint n);
  uint GetCoupleSegments(bool lookUp,
                         Wire* w,
                         uint start_track,
                         CouplingDimensionParams& coupleOptions,
                         Ath__array1D<Wire*>** firstWireTable,
                         Ath__array1D<extSegment*>* UpSegTable);
  uint FindCoupleWiresOnTracks_down(Wire* w,
                                    int start_track,
                                    CouplingDimensionParams& coupleOptions,
                                    Ath__array1D<Wire*>** firstWireTable,
                                    Ath__array1D<Wire*>* resTable);

  uint FindCoupleWiresOnTracks_up(Wire* w,
                                  uint start_track,
                                  CouplingDimensionParams& coupleOptions,
                                  Ath__array1D<Wire*>** firstWireTable,
                                  Ath__array1D<Wire*>* resTable);
  uint makeCoupleSegments_up(Wire* w,
                             uint start_track,
                             CouplingDimensionParams& coupleOptions,
                             Ath__array1D<Wire*>** firstWireTable,
                             Ath__array1D<extSegment*>* UpSegTable);

  bool FindDiagonalCoupleSegments(Wire* w,
                                  int current_level,
                                  int max_level,
                                  CouplingDimensionParams& opts,
                                  Ath__array1D<Wire*>** firstWireTable);
  bool VerticalDiagonalCouplingAndCrossOverlap(Wire* w,
                                               extSegment* s,
                                               int overMet,
                                               SegmentTables& segments,
                                               CouplingConfig& config);
  bool CreateCouplingCaps_overUnder(extSegment* s, uint overMet);
  bool CreateCouplingCaps_over(extSegment* s, uint metalLevelCnt);
  void ReleaseSegTables(uint metalLevelCnt);
  bool GetCouplingSegments(int tr,
                           Wire* w,
                           CouplingConfig& config,
                           CouplingDimensionParams& coupleOptions,
                           SegmentTables& segments,
                           Ath__array1D<Wire*>** firstWireTable);

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

  extDistRC* getDiagUnderCC(extMetRCTable* rcModel, uint dist, uint overMet);
  uint CalcDiag(uint targetMet,
                uint diagDist,
                uint tgWidth,
                uint len1,
                extSegment* s,
                int rsegId);

  FILE* OpenFile(const char* name, const char* perms);
  FILE* OpenPrintFile(uint dir, const char* name);
  void Release(Ath__array1D<extSegment*>* segTable);

  void PrintCrossSeg(FILE* fp,
                     int x1,
                     int x2,
                     int met,
                     int metOver,
                     int metUnder,
                     const char* prefix = "");
  void GetOUname(char buf[20], int met, int metOver, int metUnder);
  void PrintCrossOvelaps(Wire* w,
                         uint tgt_met,
                         int x1,
                         int x2,
                         Ath__array1D<extSegment*>* segTable,
                         int totLen,
                         const char* prefix,
                         int metOver = -1,
                         int metUnder = -1);

  // dkf 09212023
  void OverSubRC_dist_new(dbRSeg* rseg1,
                          dbRSeg* rseg2,
                          int ouCovered,
                          int diagCovered,
                          int srcCovered);
  // dkf 10212023
  void PrintCrossOvelapsOU(Wire* w,
                           uint tgt_met,
                           int x1,
                           int len,
                           Ath__array1D<extSegment*>* segTable,
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
                    uint met,
                    uint tgt_met,
                    Ath__array1D<extSegment*>* segTable,
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
                                uint width,
                                int met,
                                int metUnder,
                                int dist1,
                                int dist2);
  float getUnderRC_weightedFringe(extMetRCTable* rcModel,
                                  uint width,
                                  int met,
                                  int metOver,
                                  int dist1,
                                  int dist2);
  float getOverUnderRC_weightedFringe(extMetRCTable* rcModel,
                                      uint width,
                                      int met,
                                      int underMet,
                                      int metOver,
                                      int dist1,
                                      int dist2);
  extDistRC* getOverRC_Dist(extMetRCTable* rcModel,
                            uint width,
                            int met,
                            int metUnder,
                            int dist,
                            int open = -1);
  extDistRC* getUnderRC_Dist(extMetRCTable* rcModel,
                             uint width,
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
                                uint width,
                                int met,
                                int metUnder,
                                int dist);
  float getOverRC_Open(extMetRCTable* rcModel,
                       uint width,
                       int met,
                       int metUnder,
                       int dist1,
                       int dist2);
  extDistRC* addRC_new(extDistRC* rcUnit, uint len, uint jj, bool addCC);
  float getOURC_Open(extMetRCTable* rcModel,
                     uint width,
                     int met,
                     int metUnder,
                     int metOver,
                     int dist1,
                     int dist2);
  // DKF 9232023
  float getOver_over1(extMetRCTable* rcModel,
                      uint width,
                      int met,
                      int metUnder,
                      int dist1,
                      int dist2,
                      int lenOverSub);
  extDistRC* computeOverUnderRC(extMetRCTable* rcModel, uint len);
  float getOU_over1(extMetRCTable* rcModel,
                    int lenOverSub,
                    int dist1,
                    int dist2);

  // --------------- dkf 09142023
  bool measureRC_res_dist(Ath__array1D<SEQ*>* tmpTable);
  void measureRC_ids_flags(CoupleOptions& options);  // dkf 09142023
  void measureRC_091423(CoupleOptions& options);     // dkf 09142023
  void measureRC(CoupleOptions& options);
  float getOverR_weightedFringe(extMetRCTable* rcModel, int dist1, int dist2);
  // dimitri 09172023
  float weightedFringe(extDistRC* rc1,
                       extDistRC* rc2,
                       bool use_weighted = true);
  bool useWeightedAvg(int& dist1, int& dist2, int underMet);
  int computeAndStoreRC_new(dbRSeg* rseg1, dbRSeg* rseg2, int srcCovered);
  //-----------------------------------------------------------
  bool updateCoupCap(dbRSeg* rseg1,
                     dbRSeg* rseg2,
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
  bool PrintCurrentCoords(FILE* fp, const char* msg, uint rseg);
  void segInfo(FILE* fp, const char* msg, uint netId, int rsegId);
  double getCC(int rsegId);
  void DebugStart_res(FILE* fp);
  void DebugRes_calc(FILE* fp,
                     const char* msg,
                     int rsegId1,
                     const char* msg_len,
                     uint len,
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
  uint createContextGrid_dir(char* dirName,
                             const int bboxLL[2],
                             const int bboxUR[2],
                             int met);
  // DKF 7/25/2024 -- 3d pattern generation
  int _simVersion;

  uint FindSegments(bool lookUp,
                    uint dir,
                    int maxDist,
                    Wire* w1,
                    int xy1,
                    int len1,
                    Wire* w2_next,
                    Ath__array1D<extSegment*>* segTable);

  uint FindSegments_org(bool lookUp,
                        uint dir,
                        int maxDist,
                        Wire* w1,
                        int xy1,
                        int len1,
                        Wire* w2,
                        Ath__array1D<extSegment*>* segTable);
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
  int FindCouplingNeighbors(uint dir, uint couplingDist, uint diag_met_limit);
  int FindCouplingNeighbors_down(uint dir,
                                 uint couplingDist,
                                 uint diag_met_limit);
  void PrintCoupingNeighbors(FILE* fp, uint upCount, uint downCount);
  void PrintWire(FILE* fp,
                 Wire* w,
                 int level,
                 const char* prefix = "",
                 const char* postfix = "");
  void Print5wires(FILE* fp, Wire* w, uint level = 0);
  Wire* FindOverlap(Wire* w, Wire* first_wire);
  void ResetFirstWires(Grid* netGrid,
                       Ath__array1D<Wire*>* firstWireTable,
                       int tr1,
                       int trCnt,
                       uint limitTrackNum);
  // dkf 10022023
  Wire* FindOverlap(Wire* w, Ath__array1D<Wire*>* firstWireTable, int tr);
  // dkf 10032023
  int FindDiagonalNeighbors(uint dir,
                            uint couplingDist,
                            uint diag_met_limit,
                            uint lookUpLevel,
                            uint limitTrackNum);
  bool IsSegmentOverlap(int x1, int len1, int x2, int len2);
  bool IsOverlap(Wire* w, Wire* w2);
  Wire* GetNextWire(Grid* netGrid,
                    uint tr,
                    Ath__array1D<Wire*>* firstWireTable);
  Wire* FindOverlap(Wire* w,
                    Grid* netGrid,
                    uint tr,
                    Ath__array1D<Wire*>* firstWireTable);
  bool CheckWithNeighbors(Wire* w, Wire* prev);
  Ath__array1D<Wire*>** allocMarkTable(uint n);
  void DeleteMarkTable(Ath__array1D<Wire*>** tbl, uint n);
  void ResetFirstWires(uint m1,
                       uint m2,
                       uint dir,
                       Ath__array1D<Wire*>** firstWireTable);
  int PrintAllGrids(uint dir, FILE* fp, uint mode);

  // dkf 10042023
  void PrintDiagwires(FILE* fp, Wire* w, uint level);
  int CouplingFlow_new(uint dir, uint couplingDist, uint diag_met_limit);

  // dkf 10052023
  void Print(FILE* fp,
             Ath__array1D<extSegment*>* segTable,
             uint d,
             bool lookUp);
  void Print(FILE* fp, extSegment* s, uint d, bool lookUp);
  // void Release(Ath__array1D<extSegment *> *segTable);

  // dkf 10062023
  bool CheckOrdered(Ath__array1D<extSegment*>* segTable);
  bool measure_RC_new(int met,
                      uint dir,
                      extSegment* up,
                      extSegment* down,
                      int xy1,
                      int len);
  bool measure_init(int met,
                    uint dir,
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
                                  Ath__array1D<extSegment*>* segTable,
                                  int metOver = -1,
                                  int metUnder = -1);
  uint FindUpDownSegments(Ath__array1D<extSegment*>* upTable,
                          Ath__array1D<extSegment*>* downTable,
                          Ath__array1D<extSegment*>* segTable,
                          int metOver = -1,
                          int metUnder = -1);
  extSegment* GetNext(uint ii,
                      int& xy1,
                      int& len1,
                      Ath__array1D<extSegment*>* segTable);
  extSegment* GetNextSegment(uint ii, Ath__array1D<extSegment*>* segTable);
  uint CopySegments(bool up,
                    Ath__array1D<extSegment*>* upTable,
                    uint start,
                    uint end,
                    Ath__array1D<extSegment*>* segTable,
                    int maxDist = 1000000000,
                    int metOver = -1,
                    int metUnder = -1);
  void PrintUpDown(FILE* fp, extSegment* s);
  void PrintUpDownNet(FILE* fp, Wire* s, int dist, const char* prefix);
  void PrintUpDown(FILE* fp, Ath__array1D<extSegment*>* segTable);
  void BubbleSort(Ath__array1D<extSegment*>* segTable);
  bool measure_init(extSegment* s);
  bool measure_RC_new(extSegment* s,
                      bool skip_res_calc = false);  // dkf 06182024
  // dkf 10082023
  bool measureRC_res_dist_new(extSegment* s);
  bool measureRC_res_init(uint rsegId);
  bool measure_init_cap(extSegment* s, bool up);
  extSegment* _currentSeg;
  bool _newDiagFlow;
  // dkf 10092023
  int ConnectWires(uint dir);
  // uint CalcDiag( uint targetMet, uint diagDist, uint tgWidth, uint len1,
  // extSegment *s, int rsegId); dkf 10102023
  int FindDiagonalNeighbors_down(uint dir,
                                 uint couplingDist,
                                 uint diag_met_limit,
                                 uint lookUpLevel,
                                 uint limitTrackNum);
  bool CheckWithNeighbors_below(Wire* w, Wire* prev);
  uint CalcDiagBelow(extSegment* s, Wire* dw);
  // dkf 10112023
  int FindDiagonalNeighbors_vertical_up(uint dir,
                                        uint couplingDist,
                                        uint diag_met_limit,
                                        uint lookUpLevel,
                                        uint limitTrackNum,
                                        bool skipCheckNeighbors);
  int FindDiagonalNeighbors_vertical_power(uint dir,
                                           Wire* w,
                                           uint couplingDist,
                                           uint diag_met_limit,
                                           uint limitTrackNum,
                                           Ath__array1D<Wire*>** upWireTable);
  void Print(FILE* fp, Ath__array1D<Wire*>* segTable, const char* msg);
  // dkf 10122023
  Ath__array1D<Wire*>** _verticalPowerTable;

  // dkf 10132023
  // uint FindSegments(bool lookUp, uint dir, int maxDist, Wire *w1, int
  // xy1, int len1, Wire *w2, Ath__array1D<extSegment *> *segTable);

  // dkf 10152023
  Wire* FindDiagonalNeighbors_vertical_up_down(
      Wire* w,
      bool& found,
      uint dir,
      uint level,
      uint couplingDist,
      uint limitTrackNum,
      Ath__array1D<Wire*>** firstWireTable);
  int FindDiagonalNeighbors_vertical_down(uint dir,
                                          uint couplingDist,
                                          uint diag_met_limit,
                                          uint lookUpLevel,
                                          uint limitTrackNum,
                                          bool skipCheckNeighbors);

  // dkf 10162023
  Wire* FindOverlap_found(Wire* w, Wire* first_wire, bool& found);
  Wire* SetUpDown(Wire* w2,
                  int next_tr,
                  bool found,
                  Wire* first_wire,
                  Ath__array1D<Wire*>* firstWireTable);
  uint FindAllNeigbors_up(Wire* w,
                          uint start_track,
                          uint dir,
                          uint level,
                          uint couplingDist,
                          uint limitTrackNum,
                          Ath__array1D<Wire*>** firstWireTable,
                          Ath__array1D<Wire*>* resTable);
  Wire* FindOverlapWire(Wire* w, Wire* first_wire);

  // dkf 061824
  int CouplingFlow(uint dir,
                   uint couplingDist,
                   uint diag_met_limit,
                   int totWireCnt,
                   uint& totalWiresExtracted,
                   float& previous_percent_extracted);
  // dkf 10172023
  // dkf 061824 int CouplingFlow(uint dir, uint couplingDist, uint
  // diag_met_limit);

  extSegment* CreateUpDownSegment(bool lookUp,
                                  Wire* w,
                                  int xy1,
                                  int len1,
                                  Wire* w2,
                                  Ath__array1D<extSegment*>* segTable);
  void FindSegmentsTrack(Wire* w1,
                         int xy1,
                         int len1,
                         Wire* w2_next,
                         uint ii,
                         Ath__array1D<Wire*>* trackTable,
                         bool lookUp,
                         uint dir,
                         int maxDist,
                         Ath__array1D<extSegment*>* segTable);
  uint FindAllNeigbors_down(Wire* w,
                            int start_track,
                            uint dir,
                            uint level,
                            uint couplingDist,
                            uint limitTrackNum,
                            Ath__array1D<Wire*>** firstWireTable,
                            Ath__array1D<Wire*>* resTable);
  bool PrintInit(FILE* fp, bool dbgOverlaps, Wire* w, int x, int y);
  void PrintTable_coupleWires(FILE* fp1,
                              Wire* w,
                              bool dbgOverlaps,
                              Ath__array1D<Wire*>* UpTable,
                              const char* msg,
                              int level = -1);
  void PrintTable_segments(FILE* fp1,
                           Wire* w,
                           bool lookUp,
                           bool dbgOverlaps,
                           Ath__array1D<extSegment*>* UpSegTable,
                           const char* msg,
                           int level = -1);
  bool DebugWire(Wire* w, int x, int y, int netId = -1);
  uint CreateCouplingSEgments(Wire* w,
                              Ath__array1D<extSegment*>* segTable,
                              Ath__array1D<extSegment*>* upTable,
                              Ath__array1D<extSegment*>* downTable,
                              bool dbgOverlaps,
                              FILE* fp);
  void PrintTable_wires(FILE* fp,
                        bool dbgOverlaps,
                        uint colCnt,
                        Ath__array1D<Wire*>** verticalPowerTable,
                        const char* msg);

  // dkf 10182023
  Ath__array1D<extSegment*>** _upSegTable = nullptr;
  Ath__array1D<extSegment*>** _downSegTable = nullptr;
  Ath__array1D<extSegment*>** allocTable(uint n);
  void DeleteTable(Ath__array1D<extSegment*>** tbl, uint n);
  uint FindAllSegments_up(FILE* fp,
                          Wire* w,
                          bool lookUp,
                          uint start_track,
                          uint dir,
                          uint level,
                          uint maxDist,
                          uint couplingDist,
                          uint limitTrackNum,
                          Ath__array1D<Wire*>** firstWireTable,
                          Ath__array1D<extSegment*>** UpSegTable);
  uint FindAllSegments_vertical(FILE* fp,
                                Wire* w,
                                bool lookUp,
                                uint dir,
                                uint maxDist,
                                Ath__array1D<extSegment*>* aboveTable);

  // dkf 10192023
  dbRSeg* GetRseg(int id);
  bool VerticalCap(uint met,
                   uint tgtMet,
                   int rsegId1,
                   uint rsegId2,
                   uint len,
                   uint width,
                   uint tgtWidth,
                   uint diagDist);
  void VerticalCap(Ath__array1D<extSegment*>* segTable, bool look_up);
  bool DiagCap(FILE* fp,
               Wire* w,
               bool lookUp,
               uint maxDist,
               uint trackLimitCnt,
               Ath__array1D<extSegment*>* segTable,
               bool PowerOnly = false);
  bool DiagCouplingCap(uint met,
                       uint tgtMet,
                       int rsegId1,
                       uint rsegId2,
                       uint len,
                       uint width,
                       uint tgtWidth,
                       uint diagDist);

  // dkf 10202023
  FILE* _segFP;
  Ath__array1D<extSegment*>** _ovSegTable = nullptr;
  Ath__array1D<extSegment*>** _whiteSegTable = nullptr;

  // void PrintCrossSeg(FILE *fp, int x1, int x2, int met, int metOver, int
  // metUnder, const char *prefix="");
  //  void GetOUname(char buf[20], int met, int metOver, int metUnder);
  bool GetCrossOvelaps(Wire* w,
                       uint tgt_met,
                       int x1,
                       int x2,
                       uint dir,
                       Ath__array1D<extSegment*>* segTable,
                       Ath__array1D<extSegment*>* whiteTable);
  // void PrintCrossOvelaps(Wire *w, uint tgt_met, int x1, int x2,
  // Ath__array1D<extSegment *> *segTable, int totLen, const char *prefix, int
  // metOver=-1, int metUnder=-1);

  // dkf 10212023
  // void PrintCrossOvelapsOU(Wire *w, uint tgt_met, int x1, int len,
  // Ath__array1D<extSegment *> *segTable, int totLen, const char *prefix, int
  // metOver, int metUnder);

  // dkf 10232023
  // void PrintOverlapSeg(FILE *fp, extSegment *s, int tgt_met, const char
  // *prefix); void PrintOvelaps(extSegment *w, uint met, uint tgt_met,
  // Ath__array1D<extSegment *> *segTable, const char *ou); void PrintOUSeg(FILE
  // *fp, int x1, int len, int met, int metOver, int metUnder, const char
  // *prefix, int up_dist, int down_dist);
  void OverUnder(extSegment* cc,
                 uint met,
                 int overMet,
                 int underMet,
                 Ath__array1D<extSegment*>* segTable,
                 const char* ou);
  void OpenEnded2(extSegment* cc,
                  uint len,
                  int met,
                  int overMet,
                  int underMet,
                  FILE* segFP);
  void OpenEnded1(extSegment* cc,
                  uint len,
                  int met,
                  int overMet,
                  int underMet,
                  FILE* segFP);
  void Model1(extSegment* cc,
              uint len,
              int met,
              int metUnder,
              int metOver,
              FILE* segFP);
  void OverUnder(extSegment* cc,
                 uint len,
                 int met,
                 int metUnder,
                 int metOver,
                 FILE* segFP);

  // dkf 10242023
  extDistRC* OverUnderRC(extMetRCTable* rcModel,
                         int open,
                         uint width,
                         int dist,
                         uint len,
                         int met,
                         int metUnder,
                         int metOver,
                         FILE* segFP);
  dbRSeg* GetRSeg(extSegment* cc);
  dbRSeg* GetRSeg(uint rsegId);
  double updateCoupCap(dbRSeg* rseg1, dbRSeg* rseg2, int jj, double v);
  void OverlapDown(int overMet,
                   extSegment* coupSeg,
                   extSegment* overlapSeg,
                   uint dir);

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
                            Ath__array1D<extSegment*>* segDiagTable,
                            Ath__array1D<extSegment*>* resultTable,
                            bool dbgOverlaps,
                            FILE* fp,
                            bool lookUp,
                            int tgt_met = -1);

  // dkf 10302023
  bool CalcRes(extSegment* s);

  // dkf 11012023
  uint ConnectAllWires(Track* track);

  // dkf 061824
  bool printProgress(uint totalWiresExtracted,
                     uint totWireCnt,
                     float& previous_percent_extracted);
};

}  // namespace rcx
