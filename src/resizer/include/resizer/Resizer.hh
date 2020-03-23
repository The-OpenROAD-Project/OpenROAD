// Resizer, LEF/DEF gate resizer
// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <array>
#include "db_sta/dbSta.hh"
#include "SteinerTree.hh"

namespace sta {

using std::array;

using odb::Rect;

class RebufferOption;

typedef Map<LibertyCell*, float> CellTargetLoadMap;
typedef Vector<RebufferOption*> RebufferOptionSeq;
enum class RebufferOptionType { sink, junction, wire, buffer };
typedef Map<Vertex*, float> VertexWeightMap;
typedef Vector<Vector<Pin*>> GroupedPins;
typedef array<Required, RiseFall::index_count> Requireds;
typedef array<Slew,  RiseFall::index_count> TgtSlews;

class Resizer : public StaState
{
public:
  Resizer();
  void init(Tcl_Interp *interp,
	    dbDatabase *db,
	    dbSta *sta);

  // Set the resistance and capacitance used for parasitics.
  // Make net wire parasitics based on DEF locations.
  void setWireRC(float wire_res, // ohms/meter
		 float wire_cap, // farads/meter
		 Corner *corner);
  // ohms/meter
  float wireResistance() { return wire_res_; }
  // farads/meter
  float wireCapacitance() { return wire_cap_; }

  // Core area (meters).
  double coreArea() const;
  // 0.0 - 1.0 (100%) of core size.
  double utilization();

  void setDontUse(LibertyCellSeq *dont_use);
  void setMaxUtilization(double max_utilization);
  void resizePreamble(LibertyLibrarySeq *resize_libs);
  void bufferInputs(LibertyCell *buffer_cell);
  void bufferOutputs(LibertyCell *buffer_cell);
  // Resize all instances in the network.
  // resizerPreamble() required.
  void resizeToTargetSlew();
  // Resize inst to target slew (for testing).
  // resizerPreamble() required.
  void resizeToTargetSlew(Instance *inst);

  // Insert buffers to fix max cap/slew violations.
  // resizerPreamble() required.
  void repairMaxCapSlew(bool repair_max_cap,
			bool repair_max_slew,
			LibertyCell *buffer_cell);
  void repairMaxFanout(int max_fanout,
		       LibertyCell *buffer_cell);
  // Rebuffer net (for testing).
  // Assumes buffer_cell->isBuffer() is true.
  // resizerPreamble() required.
  void rebuffer(Net *net,
		LibertyCell *buffer_cell);
  Slew targetSlew(const RiseFall *tr);
  float targetLoadCap(LibertyCell *cell);
  void repairHoldViolations(LibertyCell *buffer_cell);
  void repairHoldViolations(Pin *end_pin,
			    LibertyCell *buffer_cell);
  // Area of the design in meter^2.
  double designArea();
  // Caller owns return value.
  NetSeq *findFloatingNets();
  void repairTieFanout(LibertyPort *tie_port,
		       int max_fanout,
		       bool verbose);
  void makeNetParasitics();
  void makeNetParasitics(const Net *net);
  void makeNetParasitics(const dbNet *net);
  // Max distance from driver pin to load in meters.
  double maxLoadManhattenDistance(Pin *drvr_pin);

protected:
  void init();
  void ensureBlock();
  double findDesignArea();
  void ensureCorner();
  void initCorner(Corner *corner);
  void ensureClkNets();
  void findClkNets();
  bool isClock(Net *net);
  void ensureLevelDrvrVerticies();
  void bufferInput(Pin *top_pin,
		   LibertyCell *buffer_cell);
  void bufferOutput(Pin *top_pin,
		    LibertyCell *buffer_cell);
  void makeEquivCells(LibertyLibrarySeq *resize_libs);
  void findTargetLoads(LibertyLibrarySeq *resize_libs);
  void findTargetLoads(LibertyLibrary *library,
		       TgtSlews &slews);
  void findTargetLoad(LibertyCell *cell,
		      TgtSlews &slews);
  float findTargetLoad(LibertyCell *cell,
		       TimingArc *arc,
		       Slew in_slew,
		       Slew out_slew);
  void findBufferTargetSlews(LibertyLibrarySeq *resize_libs);
  void findBufferTargetSlews(LibertyLibrary *library,
			     // Return values.
			     Slew slews[],
			     int counts[]);
  ParasiticNode *findParasiticNode(SteinerTree *tree,
				   Parasitic *parasitic,
				   const Net *net,
				   const Pin *pin,
				   int steiner_pt);

  // Assumes buffer_cell->isBuffer() is true.
  void rebuffer(const Pin *drvr_pin,
		LibertyCell *buffer_cell);
  void checkMaxCapViolation(const Pin *pin,
			    // Return values
			    bool &violation,
			    float &limit_ratio);
  void checkMaxSlewViolation(const Pin *pin,
			     // Return values
			     bool &violation,
			     float &limit_ratio);
  void slewLimit(const Pin *pin,
		 const MinMax *min_max,
		 // Return values.
		 float &limit,
		 bool &exists) const;
			
  RebufferOptionSeq rebufferBottomUp(SteinerTree *tree,
				     SteinerPt k,
				     SteinerPt prev,
				     int level,
				     LibertyCell *buffer_cell);
  void rebufferTopDown(RebufferOption *choice,
		       Net *net,
		       int level,
		       LibertyCell *buffer_cell);
  RebufferOptionSeq
  addWireAndBuffer(RebufferOptionSeq Z,
		   SteinerTree *tree,
		   SteinerPt k,
		   SteinerPt prev,
		   int level,
		   LibertyCell *buffer_cell);
  float portCapacitance(const LibertyPort *port);
  float pinCapacitance(const Pin *pin);
  float bufferInputCapacitance(LibertyCell *buffer_cell);
  Requireds pinRequireds(const Pin *pin);
  float gateDelay(LibertyPort *out_port,
		  RiseFall *rf,
		  float load_cap);
  float bufferDelay(LibertyCell *buffer_cell,
		    RiseFall *rf,
		    float load_cap);
  string makeUniqueNetName();
  string makeUniqueBufferName();
  string makeUniqueInstName(const char *base_name);
  bool dontUse(LibertyCell *cell);
  bool overMaxArea();
  bool hasTopLevelOutputPort(Net *net);
  Point location(Instance *inst);
  void setLocation(Instance *inst,
		   Point pt);
  Pin *singleOutputPin(const Instance *inst);
  double area(dbMaster *master);
  double area(Cell *cell);
  double dbuToMeters(uint dist) const;

  // RebufferOption factory.
  RebufferOption *makeRebufferOption(RebufferOptionType type,
				     float cap,
				     Requireds requireds,
				     Pin *load_pin,
				     Point location,
				     RebufferOption *ref,
				     RebufferOption *ref2);
  void deleteRebufferOptions();

  void findFaninWeights(VertexSet &ends,
			// Return value.
			VertexWeightMap &weight_map);
  float slackGap(Vertex *vertex);
  void repairHoldViolations(VertexSet &ends,
			    LibertyCell *buffer_cell);
  void repairHoldPass(VertexSet &ends,
		      LibertyCell *buffer_cell);
  void sortFaninsByWeight(VertexWeightMap &weight_map,
			  // Return value.
			  VertexSeq &fanins);
  void repairHoldBuffer(Pin *drvr_pin,
			Slack hold_slack,
			LibertyCell *buffer_cell);
  void repairHoldResize(Pin *drvr_pin,
			Slack hold_slack);
  void findCellInstances(LibertyCell *cell,
			 // Return value.
			 InstanceSeq &insts);
  int fanout(Pin *drvr_pin);
  void bufferLoads(Pin *drvr_pin,
		   int buffer_count,
		   int max_fanout,
		   LibertyCell *buffer_cell);
  void findLoads(Pin *drvr_pin,
		 PinSeq &loads);
  void groupLoadsCluster(Pin *drvr_pin,
			 int group_count,
			 int group_size,
			 // Return value.
			 GroupedPins &grouped_loads);
  void groupLoadsSteiner(Pin *drvr_pin,
			 int group_count,
			 int group_size,
			 // Return value.
			 GroupedPins &grouped_loads);
  void groupLoadsSteiner(SteinerTree *tree,
			 SteinerPt pt,
			 int group_size,
			 int &group_index,
			 GroupedPins &grouped_loads);
  void reportGroupedLoads(GroupedPins &grouped_loads);
  Point findCenter(PinSeq &pins);

  float wire_res_;
  float wire_cap_;
  Corner *corner_;
  LibertyCellSet dont_use_;
  double max_area_;

  dbSta *sta_;
  dbNetwork *db_network_;
  dbDatabase *db_;
  dbBlock *block_;
  Rect core_;
  double design_area_;
  const MinMax *min_max_;
  const DcalcAnalysisPt *dcalc_ap_;
  const Pvt *pvt_;
  const ParasiticAnalysisPt *parasitics_ap_;
  NetSet clk_nets_;
  bool clk_nets__valid_;
  CellTargetLoadMap *target_load_map_;
  VertexSeq level_drvr_verticies_;
  bool level_drvr_verticies_valid_;
  TgtSlews tgt_slews_;
  int unique_net_index_;
  int unique_inst_index_;
  int resize_count_;
  int inserted_buffer_count_;
  int rebuffer_net_count_;
  RebufferOptionSeq rebuffer_options_;
  friend class RebufferOption;
};

} // namespace
