// BSD 3-Clause License

// Copyright (c) 2019, SCALE Lab, Brown University
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.

// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.

// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.

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

#include "OpenPhySyn/PathPoint.hpp"
#include "OpenPhySyn/Types.hpp"

#include <bitset>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace sta
{
class TimingArc;
class RiseFall;
class ParasiticAnalysisPt;
class Pvt;
class Corner;
class Path;
class Vertex;
class FuncExpr;
class MinMax;
class PathEnd;
class ParasiticNode;
class DcalcAnalysisPt;
class Parasitic;
} // namespace sta

namespace psn
{
class Psn;
class SteinerTree;
class LibraryCellMapping;
typedef int                               SteinerPoint;
typedef std::function<bool(int)>          Legalizer;
typedef std::function<float()>            ParasticsCallback;
typedef std::function<bool(LibraryCell*)> DontUseCallback;
typedef std::function<void(Net*)>         ComputeParasiticsCallback;
typedef std::function<float()>            MaxAreaCallback;
typedef std::function<void(float)>        UpdateDesignAreaCallback;

enum ElectircalViolation
{
    None,
    Capacitance,
    Transition,
    CapacitanceAndTransition
};

class DatabaseHandler
{

public:
    DatabaseHandler(Psn* psn_inst, DatabaseSta* sta);

    std::vector<InstanceTerm*> inputPins(Instance* inst,
                                         bool include_top_level = false) const;
    std::vector<InstanceTerm*> outputPins(Instance* inst,
                                          bool include_top_level = false) const;
    std::vector<InstanceTerm*> filterPins(std::vector<InstanceTerm*>& terms,
                                          PinDirection*               direction,
                                          bool include_top_level = false) const;
    std::vector<InstanceTerm*> fanoutPins(Net* net,
                                          bool include_top_level = false) const;
    std::vector<Instance*>     fanoutInstances(Net* net) const;
    std::vector<InstanceTerm*>
                               levelDriverPins(bool                              reverse = false,
                                               std::unordered_set<InstanceTerm*> filter_pins =
                                                   std::unordered_set<InstanceTerm*>()) const;
    std::vector<Instance*>     driverInstances() const;
    InstanceTerm*              faninPin(Net* net) const;
    InstanceTerm*              faninPin(InstanceTerm* term) const;
    std::vector<InstanceTerm*> pins(Net* net) const;
    std::vector<InstanceTerm*> pins(Instance* inst) const;
    Net*                       net(InstanceTerm* term) const;
    Term*                      term(InstanceTerm* term) const;
    Net*                       net(Term* term) const;
    std::vector<InstanceTerm*> connectedPins(Net* net) const;
    std::set<InstanceTerm*>    clockPins() const;
    std::set<Net*>             clockNets() const;
    Point                      location(InstanceTerm* term);
    Point                      location(Instance* inst);
    float                      area(LibraryCell* cell) const;
    float                      area(Instance* inst) const;
    float                      area() const;
    float                      power(std::vector<Instance*>& insts);
    float                      power();
    void                       setLocation(Instance* inst, Point pt);
    LibraryTerm*               libraryPin(InstanceTerm* term) const;
    Port*                      topPort(InstanceTerm* term) const;
    LibraryCell*               libraryCell(InstanceTerm* term) const;
    LibraryCell*               libraryCell(Instance* inst) const;
    LibraryCell*               libraryCell(const char* name) const;
    LibraryCell*               largestLibraryCell(LibraryCell* cell);
    double                     dbuToMeters(int dist) const;
    double                     dbuToMicrons(int dist) const;
    bool                       isPlaced(InstanceTerm* term) const;
    bool                       isPlaced(Instance* inst) const;
    bool                       isDriver(InstanceTerm* term) const;
    bool                       isTopLevel(InstanceTerm* term) const;
    Instance*                  instance(InstanceTerm* term) const;
    Instance*                  instance(const char* name) const;
    BlockTerm*                 port(const char* name) const;
    InstanceTerm*              pin(const char* name) const;
    float                      pinCapacitance(InstanceTerm* term) const;
    float                      pinCapacitance(LibraryTerm* term) const;
    void  ripupBuffers(std::unordered_set<Instance*> buffers);
    void  ripupBuffer(Instance* buffer);
    float pinSlewLimit(InstanceTerm* term, bool* exists = nullptr) const;
    float pinAverageRise(LibraryTerm* from, LibraryTerm* to) const;
    float pinAverageFall(LibraryTerm* from, LibraryTerm* to) const;
    float pinAverageRiseTransition(LibraryTerm* from, LibraryTerm* to) const;
    float pinAverageFallTransition(LibraryTerm* from, LibraryTerm* to) const;
    float loadCapacitance(InstanceTerm* term) const;
    std::vector<std::vector<PathPoint>> getNegativeSlackPaths() const;
    float                               maxLoad(LibraryCell* cell);
    float       capacitanceLimit(InstanceTerm* term) const;
    float       targetLoad(LibraryCell* cell);
    float       coreArea() const;
    bool        maximumUtilizationViolation() const;
    void        setMaximumArea(float area);
    void        setMaximumArea(MaxAreaCallback maximum_area_callback);
    float       maximumArea() const;
    std::string unitScaledArea(float ar) const;
    void
          setUpdateDesignArea(UpdateDesignAreaCallback update_design_area_callback);
    void  notifyDesignAreaChanged(float new_area);
    void  notifyDesignAreaChanged();
    bool  hasMaximumArea() const;
    float gateDelay(Instance* inst, InstanceTerm* to, float in_slew = 0,
                    LibraryTerm* from = nullptr, float* drvr_slew = nullptr,
                    int rise_fall = -1);
    float gateDelay(LibraryCell* cell, InstanceTerm* to, float in_slew = 0,
                    LibraryTerm* from = nullptr, float* drvr_slew = nullptr,
                    int rise_fall = -1);
    float gateDelay(InstanceTerm* out_port, float load_cap,
                    float* tr_slew = nullptr);
    float gateDelay(LibraryTerm* out_port, float load_cap,
                    float* tr_slew = nullptr);
    float bufferChainDelayPenalty(float load_cap);
    float inverterInputCapacitance(LibraryCell* buffer_cell);
    float bufferInputCapacitance(LibraryCell* buffer_cell) const;
    float bufferOutputCapacitance(LibraryCell* buffer_cell);
    LibraryTerm*  largestInputCapacitanceLibraryPin(Instance* cell);
    InstanceTerm* largestLoadCapacitancePin(Instance* cell);
    float         largestInputCapacitance(Instance* cell);
    float         largestInputCapacitance(LibraryCell* cell);
    float portCapacitance(const LibraryTerm* port, bool isMax = true) const;
    float bufferDelay(psn::LibraryCell* buffer_cell, float load_cap);
    float maxLoad(LibraryTerm* term);
    Net*  net(const char* name) const;
    LibraryTerm* libraryPin(const char* cell_name, const char* pin_name) const;
    LibraryTerm* libraryPin(LibraryCell* cell, const char* pin_name) const;
    LibraryTerm* bufferInputPin(LibraryCell* buffer_cell) const;
    LibraryTerm* bufferOutputPin(LibraryCell* buffer_cell) const;
    std::unordered_set<InstanceTerm*> commutativePins(InstanceTerm* term);
    std::vector<LibraryTerm*>         libraryPins(Instance* inst) const;
    std::vector<LibraryTerm*>         libraryPins(LibraryCell* cell) const;
    std::vector<LibraryTerm*>         libraryInputPins(LibraryCell* cell) const;
    std::vector<LibraryTerm*> libraryOutputPins(LibraryCell* cell) const;
    std::vector<LibraryCell*> tiehiCells() const;
    std::vector<LibraryCell*> tieloCells() const;
    std::vector<LibraryCell*> inverterCells() const;
    std::vector<LibraryCell*> bufferCells() const;
    std::vector<LibraryCell*> nandCells(int in_size = 0);
    std::vector<LibraryCell*> andCells(int in_size = 0);
    std::vector<LibraryCell*> orCells(int in_size = 0);
    std::vector<LibraryCell*> norCells(int in_size = 0);
    std::vector<LibraryCell*> xorCells(int in_size = 0);
    std::vector<LibraryCell*> xnorCells(int in_size = 0);
    int                       isAND(LibraryCell* cell);
    int                       isNAND(LibraryCell* cell);
    int                       isOR(LibraryCell* cell);
    int                       isNOR(LibraryCell* cell);
    int                       isXOR(LibraryCell* cell);
    int                       isXNOR(LibraryCell* cell);
    bool                      isXORXNOR(LibraryCell* cell);
    bool                      isANDOR(LibraryCell* cell);
    bool                      isNANDNOR(LibraryCell* cell);
    bool                      isAnyANDOR(LibraryCell* cell);
    LibraryCell*              closestDriver(LibraryCell*              cell,
                                            std::vector<LibraryCell*> candidates,
                                            float                     scale = 1.0);
    LibraryCell*              halfDrivingPowerCell(Instance* inst);
    LibraryCell*              halfDrivingPowerCell(LibraryCell* cell);

    std::vector<LibraryCell*> cellSuperset(LibraryCell* cell, int in_size = 0);

    std::vector<LibraryCell*> inverseCells(LibraryCell* cell);
    std::vector<LibraryCell*> invertedEquivalent(LibraryCell* cell);
    LibraryCell*              minimumDrivingInverter(LibraryCell* cell,
                                                     float        extra_cap = 0.0);
    std::pair<std::vector<LibraryCell*>, std::vector<LibraryCell*>>
                              bufferClusters(float cluster_threshold, bool find_superior = true,
                                             bool include_inverting = true);
    std::vector<LibraryCell*> equivalentCells(LibraryCell* cell);
    LibraryCell*              smallestInverterCell() const;
    LibraryCell*              smallestBufferCell() const;
    bool                      isClocked(InstanceTerm* term) const;
    bool                      isClock(Net* net) const;
    bool                      isPrimary(Net* net) const;
    bool                      isInput(InstanceTerm* term) const;
    bool                      isOutput(InstanceTerm* term) const;
    bool                      isAnyInput(InstanceTerm* term) const;
    bool                      isAnyOutput(InstanceTerm* term) const;
    bool                      isBiDirect(InstanceTerm* term) const;
    bool                      isTriState(InstanceTerm* term) const;
    bool                      isInput(LibraryTerm* term) const;
    bool                      isOutput(LibraryTerm* term) const;
    bool                      isAnyInput(LibraryTerm* term) const;
    bool                      isAnyOutput(LibraryTerm* term) const;
    bool                      isBiDirect(LibraryTerm* term) const;
    bool                      isTriState(LibraryTerm* term) const;
    bool                      isCombinational(Instance* inst) const;
    bool                      isCombinational(LibraryCell* cell) const;
    bool                      isSpecial(Net* net) const;
    bool                      isTieHi(Instance* inst) const;
    bool                      isTieHi(LibraryCell* cell) const;
    bool                      isTieLo(Instance* inst) const;
    bool                      isTieLo(LibraryCell* cell) const;
    bool                      isTieHiLo(Instance* inst) const;
    bool                      isTieHiLo(LibraryCell* cell) const;
    bool                      isTieCell(Instance* inst) const;
    bool                      isTieCell(LibraryCell* cell) const;
    bool                      isSingleOutputCombinational(Instance* inst) const;
    bool isSingleOutputCombinational(LibraryCell* cell) const;
    void replaceInstance(Instance* inst, LibraryCell* cell);
    bool violatesMaximumCapacitance(InstanceTerm* term, float load_cap,
                                    float limit_scale_factor) const;
    bool violatesMaximumCapacitance(InstanceTerm* term,
                                    float limit_scale_factor = 1.0) const;
    bool violatesMaximumTransition(InstanceTerm* term,
                                   float         limit_scale_factor = 1.0);
    ElectircalViolation hasElectricalViolation(InstanceTerm* term,
                                               float cap_scale_factor   = 1.0,
                                               float trans_scale_factor = 1.0);
    std::vector<InstanceTerm*>
    maximumTransitionViolations(float limit_scale_factor = 1.0) const;
    std::vector<InstanceTerm*>
              maximumCapacitanceViolations(float limit_scale_factor = 1.0) const;
    bool      isLoad(InstanceTerm* term) const;
    Instance* createInstance(const char* inst_name, LibraryCell* cell);
    void      createClock(const char* clock_name, std::vector<BlockTerm*> ports,
                          float period);
    void  createClock(const char* clock_name, std::vector<std::string> ports,
                      float period);
    Net*  createNet(const char* net_name);
    float resistance(LibraryTerm* term) const;
    float resistancePerMicron() const;
    float capacitancePerMicron() const;
    void  connect(Net* net, InstanceTerm* term) const;
    void  connect(Net* net, Instance* inst, LibraryTerm* port) const;
    void  connect(Net* net, Instance* inst, Port* port) const;
    void  disconnect(InstanceTerm* term) const;
    int   disconnectAll(Net* net) const;
    Net*  bufferNet(Net* b_net, LibraryCell* buffer, std::string buffer_name,
                    std::string net_name, Point location);
    void  swapPins(InstanceTerm* first, InstanceTerm* second);
    void  del(Net* net) const;
    void  del(Instance* inst) const;
    void  clear();
    unsigned int fanoutCount(Net* net, bool include_top_level = false) const;
    std::vector<PathPoint>              criticalPath(int path_count = 1) const;
    std::vector<std::vector<PathPoint>> criticalPaths(int path_count = 1) const;
    std::vector<std::vector<PathPoint>> bestPath(int path_count = 1) const;
    std::vector<PathPoint>              worstSlackPath(InstanceTerm* term,
                                                       bool          trim = false) const;
    InstanceTerm*                       worstSlackPin() const;
    std::vector<PathPoint> worstArrivalPath(InstanceTerm* term) const;
    std::vector<PathPoint> bestSlackPath(InstanceTerm* term) const;
    std::vector<PathPoint> bestArrivalPath(InstanceTerm* term) const;
    float pinSlack(InstanceTerm* term, bool is_rise, bool worst) const;
    float pinSlack(InstanceTerm* term, bool worst = true) const;
    float slack(InstanceTerm* term);
    float worstSlack() const;
    float worstSlack(InstanceTerm* term) const;
    float arrival(InstanceTerm* term, int ap_index, bool is_rise = true) const;
    float slew(InstanceTerm* term) const;
    float slew(InstanceTerm* term, bool is_rise) const;
    float slew(LibraryTerm* term, float cap, float* tr_slew = nullptr);
    float required(InstanceTerm* term) const;
    float required(InstanceTerm* term, bool is_rise,
                   PathAnalysisPoint* path_ap) const;
    bool  isCommutative(InstanceTerm* first, InstanceTerm* second) const;
    bool  isCommutative(LibraryTerm* first, LibraryTerm* second) const;
    bool  isBuffer(LibraryCell* cell) const;
    bool  isInverter(LibraryCell* cell) const;
    bool  dontUse(LibraryCell* cell) const;
    bool  dontTouch(Instance* cell) const;
    bool  dontSize(Instance* cell) const;
    void  resetDelays();
    void  resetDelays(InstanceTerm* term);
    void  buildLibraryMappings(int                        max_length,
                               std::vector<LibraryCell*>& buffer_lib,
                               std::vector<LibraryCell*>& inverter_lib);
    void  buildLibraryMappings(int max_length);
    void  resetLibraryMapping();
    std::shared_ptr<LibraryCellMapping>
                                        getLibraryCellMapping(LibraryCell* cell);
    std::shared_ptr<LibraryCellMapping> getLibraryCellMapping(Instance* inst);
    std::unordered_set<LibraryCell*>    truthTableToCells(std::string table_id);
    std::string                         cellToTruthTable(LibraryCell* cell);
    std::vector<Net*>                   nets() const;
    std::vector<Instance*>              instances() const;
    Block*                              top() const;
    Library*                            library() const;
    LibraryTechnology*                  technology() const;
    bool                                hasLiberty() const;
    std::string                         name(Block* object) const;
    std::string                         name(Net* object) const;
    std::string                         name(Instance* object) const;
    std::string                         name(BlockTerm* object) const;
    std::string                         name(Library* object) const;
    std::string                         name(LibraryCell* object) const;
    std::string                         name(LibraryTerm* object) const;
    std::string                         topName() const;
    void        setDontUse(std::vector<std::string>& cell_names);
    std::string generateNetName(int& start_index);
    std::string generateInstanceName(const std::string& prefix,
                                     int&               start_index);
    int         evaluateFunctionExpression(
                InstanceTerm*                          term,
                std::unordered_map<LibraryTerm*, int>& inputs) const;
    int evaluateFunctionExpression(
        LibraryTerm* term, std::unordered_map<LibraryTerm*, int>& inputs) const;
    void setWireRC(float res_per_micron, float cap_per_micron,
                   bool reset_delays = true);
    void setWireRC(ParasticsCallback res_per_micron,
                   ParasticsCallback cap_per_micron);
    void setDontUseCallback(DontUseCallback dont_use_callback);
    void setComputeParasiticsCallback(
        ComputeParasiticsCallback compute_parasitics_callback);

    bool        hasWireRC();
    HandlerType handlerType() const;
    void        calculateParasitics();
    void        calculateParasitics(Net* net);
    void        resetCache();
    void        setLegalizer(Legalizer legalizer);
    bool        legalize(int max_displacement = 0);
    float       bufferFixedInputSlew(LibraryCell* buffer_cell, float cap);

    DatabaseStaNetwork* network() const;
    DatabaseSta*        sta() const;
    ~DatabaseHandler();

    int evaluateFunctionExpression(
        sta::FuncExpr*                         func,
        std::unordered_map<LibraryTerm*, int>& inputs) const;
    Vertex* vertex(InstanceTerm* term) const;

private:
    std::vector<Liberty*> allLibs() const;

    DatabaseSta* sta_;
    Database*    db_;

    const sta::MinMax*        min_max_;
    bool                      has_buffer_inverter_seq_;
    bool                      has_equiv_cells_;
    bool                      has_target_loads_;
    float                     res_per_micron_;
    float                     cap_per_micron_;
    bool                      has_wire_rc_;
    Psn*                      psn_;
    std::vector<LibraryCell*> buffer_inverter_seq_;
    float                     maximum_area_;
    bool                      maximum_area_valid_;

    std::unordered_set<LibraryCell*> nand_cells_;
    std::unordered_set<LibraryCell*> and_cells_;
    std::unordered_set<LibraryCell*> nor_cells_;
    std::unordered_set<LibraryCell*> or_cells_;
    std::unordered_set<LibraryCell*> xor_cells_;
    std::unordered_set<LibraryCell*> xnor_cells_;

    std::unordered_set<LibraryCell*> dont_use_;

    std::unordered_map<LibraryCell*, float> buffer_penalty_map_;
    std::unordered_map<LibraryCell*, float> inverting_buffer_penalty_map_;
    std::unordered_set<LibraryCell*>        non_inverting_buffer_;
    std::unordered_set<LibraryCell*>        inverting_buffer_;
    std::unordered_map<LibraryTerm*, std::unordered_set<LibraryTerm*>>
        commutative_pins_cache_;

    std::unordered_map<float, float> penalty_cache_;

    std::unordered_map<std::string, std::shared_ptr<LibraryCellMapping>>
                                                  library_cell_mappings_;
    std::unordered_map<LibraryCell*, std::string> truth_tables_;
    std::unordered_map<std::string, std::unordered_set<LibraryCell*>>
        function_to_cell_; // Mapping from truth table to cells

    bool has_library_cell_mappings_;

    void populatePrimitiveCellCache();

    int computeTruthTable(LibraryCell* cell);

    std::unordered_map<LibraryCell*, float> target_load_map_;

    // Vertex* vertex(InstanceTerm* term) const;

    void computeBuffersDelayPenalty(bool include_inverting = true);

    /* The following code is borrowed from James Cherry's Resizer Code */
    const sta::Corner*              corner_;
    const sta::DcalcAnalysisPt*     dcalc_ap_;
    const sta::Pvt*                 pvt_;
    const sta::ParasiticAnalysisPt* parasitics_ap_;
    float                           target_slews_[2];
    float pinTableAverage(LibraryTerm* from, LibraryTerm* to,
                          bool is_delay = true, bool is_rise = true) const;
    float pinTableLookup(LibraryTerm* from, LibraryTerm* to, float slew,
                         float cap, bool is_delay = true,
                         bool is_rise = true) const;
    std::vector<std::vector<PathPoint>> getPaths(bool get_max,
                                                 int  path_count = 1) const;
    std::vector<PathPoint>              expandPath(sta::PathEnd* path_end,
                                                   bool          enumed = false) const;
    std::vector<PathPoint>              expandPath(sta::Path* path,
                                                   bool       enumed = false) const;
    void                                findTargetLoads();
    void                                makeEquivalentCells();

    void  findTargetLoads(std::vector<Liberty*>* resize_libs);
    void  findTargetLoads(Liberty* library, float slews[]);
    void  findTargetLoad(LibraryCell* cell, float slews[]);
    float findTargetLoad(LibraryCell* cell, sta::TimingArc* arc, float in_slew,
                         float out_slew);
    float targetSlew(const sta::RiseFall* rf);
    void  findBufferTargetSlews(std::vector<Liberty*>* resize_libs);
    void  findBufferTargetSlews(Liberty* library, float slews[], int counts[]);
    void  slewLimit(InstanceTerm* pin, sta::MinMax* min_max, float& limit,
                    bool& exists) const;
    sta::ParasiticNode* findParasiticNode(std::unique_ptr<SteinerTree>& tree,
                                          sta::Parasitic*     parasitic,
                                          const Net*          net,
                                          const InstanceTerm* pin,
                                          SteinerPoint        pt);
    Legalizer           legalizer_;
    ParasticsCallback   res_per_micron_callback_;
    ParasticsCallback   cap_per_micron_callback_;
    DontUseCallback     dont_use_callback_;
    ComputeParasiticsCallback compute_parasitics_callback_;
    MaxAreaCallback           maximum_area_callback_;
    UpdateDesignAreaCallback  update_design_area_callback_;
};

} // namespace psn
