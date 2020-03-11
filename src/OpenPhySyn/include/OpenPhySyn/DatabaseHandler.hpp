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
#ifndef __PSN_DATABASE_HANDLER__
#define __PSN_DATABASE_HANDLER__
#include <OpenPhySyn/PathPoint.hpp>
#include <OpenPhySyn/PsnGlobal.hpp>
#include <OpenPhySyn/Types.hpp>

#include <PsnLogger.hpp>
#include <unordered_map>
#include <vector>
namespace psn
{
class PathPoint;
class OpenStaHandler
{
public:
    OpenStaHandler(DatabaseSta* sta);

    virtual std::vector<InstanceTerm*>
    inputPins(Instance* inst, bool include_top_level = false) const;
    virtual std::vector<InstanceTerm*>
    outputPins(Instance* inst, bool include_top_level = false) const;
    virtual std::vector<InstanceTerm*>
    filterPins(std::vector<InstanceTerm*>& terms, PinDirection* direction,
               bool include_top_level = false) const;
    virtual std::vector<InstanceTerm*>
                                       fanoutPins(Net* net, bool include_top_level = false) const;
    virtual std::vector<Instance*>     fanoutInstances(Net* net) const;
    virtual std::vector<InstanceTerm*> levelDriverPins() const;
    virtual std::vector<Instance*>     driverInstances() const;
    virtual InstanceTerm*              faninPin(Net* net) const;
    virtual InstanceTerm*              faninPin(InstanceTerm* term) const;
    virtual std::vector<InstanceTerm*> pins(Net* net) const;
    virtual std::vector<InstanceTerm*> pins(Instance* inst) const;
    virtual Net*                       net(InstanceTerm* term) const;
    virtual Term*                      term(InstanceTerm* term) const;
    virtual Net*                       net(Term* term) const;
    virtual std::vector<InstanceTerm*> connectedPins(Net* net) const;
    virtual std::set<BlockTerm*>       clockPins() const;
    virtual Point                      location(InstanceTerm* term);
    virtual Point                      location(Instance* inst);
    virtual float                      area(LibraryCell* cell) const;
    virtual float                      area(Instance* inst) const;
    virtual float                      area() const;
    virtual float                      power(std::vector<Instance*>& insts);
    virtual float                      power();
    virtual void                       setLocation(Instance* inst, Point pt);
    virtual LibraryTerm*               libraryPin(InstanceTerm* term) const;
    virtual LibraryCell*               libraryCell(InstanceTerm* term) const;
    virtual LibraryCell*               libraryCell(Instance* inst) const;
    virtual LibraryCell*               libraryCell(const char* name) const;
    virtual LibraryCell*               largestLibraryCell(LibraryCell* cell);
    virtual double                     dbuToMeters(uint dist) const;
    virtual double                     dbuToMicrons(uint dist) const;
    virtual bool                       isPlaced(InstanceTerm* term) const;
    virtual bool                       isPlaced(Instance* inst) const;
    virtual bool                       isDriver(InstanceTerm* term) const;
    virtual bool                       isTopLevel(InstanceTerm* term) const;
    virtual Instance*                  instance(InstanceTerm* term) const;
    virtual Instance*                  instance(const char* name) const;
    virtual BlockTerm*                 port(const char* name) const;
    virtual float                      pinCapacitance(InstanceTerm* term) const;
    virtual float                      pinCapacitance(LibraryTerm* term) const;
    virtual float pinAverageRise(LibraryTerm* from, LibraryTerm* to) const;
    virtual float pinAverageFall(LibraryTerm* from, LibraryTerm* to) const;
    virtual float pinAverageRiseTransition(LibraryTerm* from,
                                           LibraryTerm* to) const;
    virtual float pinAverageFallTransition(LibraryTerm* from,
                                           LibraryTerm* to) const;
    virtual float loadCapacitance(InstanceTerm* term) const;
    virtual float maxLoad(LibraryCell* cell);
    virtual float targetLoad(LibraryCell* cell);
    virtual float gateDelay(Instance* inst, InstanceTerm* to, float in_slew = 0,
                            LibraryTerm* from = nullptr,
                            float* drvr_slew = nullptr, int rise_fall = -1);
    virtual float gateDelay(LibraryTerm* out_port, float load_cap);
    virtual float bufferInputCapacitance(LibraryCell* buffer_cell);
    virtual float bufferOutputCapacitance(LibraryCell* buffer_cell);
    virtual float portCapacitance(const LibraryTerm* port, bool isMax = true);
    virtual float bufferDelay(psn::LibraryCell* buffer_cell, float load_cap);
    virtual float maxLoad(LibraryTerm* term);
    virtual Net*  net(const char* name) const;
    virtual LibraryTerm*              libraryPin(const char* cell_name,
                                                 const char* pin_name) const;
    virtual LibraryTerm*              libraryPin(LibraryCell* cell,
                                                 const char*  pin_name) const;
    virtual std::vector<LibraryTerm*> libraryPins(Instance* inst) const;
    virtual std::vector<LibraryTerm*> libraryPins(LibraryCell* cell) const;
    virtual std::vector<LibraryTerm*> libraryInputPins(LibraryCell* cell) const;
    virtual std::vector<LibraryTerm*>
                                      libraryOutputPins(LibraryCell* cell) const;
    virtual std::vector<LibraryCell*> tiehiCells() const;
    virtual std::vector<LibraryCell*> tieloCells() const;
    virtual std::vector<LibraryCell*> inverterCells() const;
    virtual std::vector<LibraryCell*> bufferCells() const;
    virtual LibraryCell*              smallestInverterCell() const;
    virtual LibraryCell*              smallestBufferCell() const;
    virtual bool                      isClocked(InstanceTerm* term) const;
    virtual bool                      isPrimary(Net* net) const;
    virtual bool                      isInput(InstanceTerm* term) const;
    virtual bool                      isOutput(InstanceTerm* term) const;
    virtual bool                      isAnyInput(InstanceTerm* term) const;
    virtual bool                      isAnyOutput(InstanceTerm* term) const;
    virtual bool                      isBiDriect(InstanceTerm* term) const;
    virtual bool                      isTriState(InstanceTerm* term) const;
    virtual bool                      isInput(LibraryTerm* term) const;
    virtual bool                      isOutput(LibraryTerm* term) const;
    virtual bool                      isAnyInput(LibraryTerm* term) const;
    virtual bool                      isAnyOutput(LibraryTerm* term) const;
    virtual bool                      isBiDriect(LibraryTerm* term) const;
    virtual bool                      isTriState(LibraryTerm* term) const;
    virtual bool                      isCombinational(Instance* inst) const;
    virtual bool                      isCombinational(LibraryCell* cell) const;
    virtual bool      isSingleOutputCombinational(Instance* inst) const;
    virtual bool      isSingleOutputCombinational(LibraryCell* cell) const;
    virtual bool      violatesMaximumCapacitance(InstanceTerm* term,
                                                 float         load_cap) const;
    virtual bool      violatesMaximumCapacitance(InstanceTerm* term) const;
    virtual bool      violatesMaximumTransition(InstanceTerm* term) const;
    virtual bool      isLoad(InstanceTerm* term) const;
    virtual Instance* createInstance(const char* inst_name, LibraryCell* cell);
    virtual void      createClock(const char*             clock_name,
                                  std::vector<BlockTerm*> ports, float period);
    virtual void      createClock(const char*              clock_name,
                                  std::vector<std::string> ports, float period);
    virtual Net*      createNet(const char* net_name);
    virtual void      connect(Net* net, InstanceTerm* term) const;
    virtual InstanceTerm* connect(Net* net, Instance* inst,
                                  LibraryTerm* port) const;
    virtual void          disconnect(InstanceTerm* term) const;
    virtual int           disconnectAll(Net* net) const;
    virtual void          swapPins(InstanceTerm* first, InstanceTerm* second);
    virtual void          del(Net* net) const;
    virtual void          del(Instance* inst) const;
    virtual void          clear();
    virtual unsigned int  fanoutCount(Net* net,
                                      bool include_top_level = false) const;
    virtual std::vector<PathPoint> criticalPath(int path_count = 1) const;
    virtual std::vector<std::vector<PathPoint>>
                                   bestPath(int path_count = 1) const;
    virtual std::vector<PathPoint> worstSlackPath(InstanceTerm* term) const;
    virtual std::vector<PathPoint> worstArrivalPath(InstanceTerm* term) const;
    virtual std::vector<PathPoint> bestSlackPath(InstanceTerm* term) const;
    virtual std::vector<PathPoint> bestArrivalPath(InstanceTerm* term) const;
    virtual float slack(InstanceTerm* term, bool is_rise, bool worst) const;
    virtual float slack(InstanceTerm* term, bool worst = true) const;
    virtual float arrival(InstanceTerm* term, int ap_index,
                          bool is_rise = true) const;
    virtual float slew(InstanceTerm* term, bool is_rise = true) const;
    virtual float required(InstanceTerm* term, bool worst = true) const;
    virtual bool isCommutative(InstanceTerm* first, InstanceTerm* second) const;
    virtual bool dontUse(LibraryCell* cell) const;
    virtual bool dontTouch(Instance* cell) const;
    virtual void resetDelays();
    virtual void resetDelays(InstanceTerm* term);
    virtual std::vector<Net*>      nets() const;
    virtual std::vector<Instance*> instances() const;
    virtual Block*                 top() const;
    virtual Library*               library() const;
    virtual LibraryTechnology*     technology() const;
    virtual std::string            name(Block* object) const;
    virtual std::string            name(Net* object) const;
    virtual std::string            name(Instance* object) const;
    virtual std::string            name(BlockTerm* object) const;
    virtual std::string            name(Library* object) const;
    virtual std::string            name(LibraryCell* object) const;
    virtual std::string            name(LibraryTerm* object) const;
    virtual std::string            topName() const;
    virtual HandlerType            handlerType() const;

    DatabaseStaNetwork* network() const;
    DatabaseSta*        sta() const;
    virtual ~OpenStaHandler();

    virtual int evaluateFunctionExpression(
        InstanceTerm*                          term,
        std::unordered_map<LibraryTerm*, int>& inputs) const;
    virtual int evaluateFunctionExpression(
        LibraryTerm* term, std::unordered_map<LibraryTerm*, int>& inputs) const;
    virtual int evaluateFunctionExpression(
        sta::FuncExpr*                         func,
        std::unordered_map<LibraryTerm*, int>& inputs) const;
    void resetCache(); // Reset equivalent cells and target loads
private:
    void                   makeEquivalentCells();
    sta::LibertyLibrarySeq allLibs() const;
    DatabaseSta*           sta_;
    Database*              db_;

    const sta::MinMax* min_max_;
    bool               has_equiv_cells_;
    bool               has_target_loads_;

    std::unordered_map<LibraryCell*, float> target_load_map_;

    void findTargetLoads();

    sta::Vertex* vertex(InstanceTerm* term) const;

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

    /* The following code is borrowed from James Cherry's Resizer Code */
    const sta::Corner*              corner_;
    const sta::DcalcAnalysisPt*     dcalc_ap_;
    const sta::Pvt*                 pvt_;
    const sta::ParasiticAnalysisPt* parasitics_ap_;
    sta::Slew                       target_slews_[sta::RiseFall::index_count];

    void      findTargetLoads(sta::LibertyLibrarySeq* resize_libs);
    void      findTargetLoads(Liberty* library, sta::Slew slews[]);
    void      findTargetLoad(LibraryCell* cell, sta::Slew slews[]);
    float     findTargetLoad(LibraryCell* cell, sta::TimingArc* arc,
                             sta::Slew in_slew, sta::Slew out_slew);
    sta::Slew targetSlew(const sta::RiseFall* rf);
    void      findBufferTargetSlews(sta::LibertyLibrarySeq* resize_libs);
    void      findBufferTargetSlews(Liberty* library, sta::Slew slews[],
                                    int counts[]);
    void      slewLimit(InstanceTerm* pin, sta::MinMax* min_max, float& limit,
                        bool& exists) const;
};

typedef OpenStaHandler DatabaseHandler;
}; // namespace psn
#endif