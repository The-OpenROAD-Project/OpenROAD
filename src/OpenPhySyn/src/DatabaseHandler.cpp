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

// Temproary fix for OpenSTA import ordering
#define THROW_DCL throw()

#include <OpenPhySyn/DatabaseHandler.hpp>
#include <OpenPhySyn/PsnLogger.hpp>
#include <OpenSTA/dcalc/ArcDelayCalc.hh>
#include <OpenSTA/dcalc/DcalcAnalysisPt.hh>
#include <OpenSTA/dcalc/GraphDelayCalc.hh>
#include <OpenSTA/graph/Graph.hh>
#include <OpenSTA/liberty/FuncExpr.hh>
#include <OpenSTA/liberty/TableModel.hh>
#include <OpenSTA/liberty/TimingArc.hh>
#include <OpenSTA/liberty/TimingModel.hh>
#include <OpenSTA/liberty/TimingRole.hh>
#include <OpenSTA/liberty/Transition.hh>
#include <OpenSTA/network/NetworkCmp.hh>
#include <OpenSTA/network/PortDirection.hh>
#include <OpenSTA/sdc/Sdc.hh>
#include <OpenSTA/search/Corner.hh>
#include <OpenSTA/search/PathEnd.hh>
#include <OpenSTA/search/PathExpanded.hh>
#include <OpenSTA/search/Power.hh>
#include <OpenSTA/search/Search.hh>
#include <OpenSTA/util/PatternMatch.hh>
#include <algorithm>
#include <cmath>
#include <set>

namespace psn
{
OpenStaHandler::OpenStaHandler(DatabaseSta* sta)
    : sta_(sta),
      db_(sta->db()),
      has_equiv_cells_(false),
      has_target_loads_(false)
{
    // Use default corner for now
    corner_        = sta_->findCorner("default");
    min_max_       = sta::MinMax::max();
    dcalc_ap_      = corner_->findDcalcAnalysisPt(min_max_);
    pvt_           = dcalc_ap_->operatingConditions();
    parasitics_ap_ = corner_->findParasiticAnalysisPt(min_max_);
    resetDelays();
}

std::vector<InstanceTerm*>
OpenStaHandler::pins(Net* net) const
{
    std::vector<InstanceTerm*> terms;
    auto                       pin_iter = network()->pinIterator(net);
    while (pin_iter->hasNext())
    {
        InstanceTerm* pin = pin_iter->next();
        terms.push_back(pin);
    }
    return terms;
}
std::vector<InstanceTerm*>
OpenStaHandler::pins(Instance* inst) const
{
    std::vector<InstanceTerm*> terms;
    auto                       pin_iter = network()->pinIterator(inst);
    while (pin_iter->hasNext())
    {
        InstanceTerm* pin = pin_iter->next();
        terms.push_back(pin);
    }
    return terms;
}
Net*
OpenStaHandler::net(InstanceTerm* term) const
{
    return network()->net(term);
}
Term*
OpenStaHandler::term(InstanceTerm* term) const
{
    return network()->term(term);
}
Net*
OpenStaHandler::net(Term* term) const
{
    return network()->net(term);
}
std::vector<InstanceTerm*>
OpenStaHandler::connectedPins(Net* net) const
{
    std::vector<InstanceTerm*> terms;
    auto                       pin_iter = network()->connectedPinIterator(net);
    while (pin_iter->hasNext())
    {
        InstanceTerm* pin = pin_iter->next();
        terms.push_back(pin);
    }
    std::sort(terms.begin(), terms.end(), sta::PinPathNameLess(network()));
    return terms;
}
std::set<BlockTerm*>
OpenStaHandler::clockPins() const
{
    std::set<BlockTerm*> clock_pins;
    auto                 clk_iter = new sta::ClockIterator(network()->sdc());
    while (clk_iter->hasNext())
    {
        auto clk  = clk_iter->next();
        auto pins = clk->pins();
        for (auto pin : pins)
        {
            clock_pins.insert(pin);
        }
    }
    delete clk_iter;
    return clock_pins;
}

std::vector<InstanceTerm*>
OpenStaHandler::inputPins(Instance* inst, bool include_top_level) const
{
    auto inst_pins = pins(inst);
    return filterPins(inst_pins, PinDirection::input(), include_top_level);
}

std::vector<InstanceTerm*>
OpenStaHandler::outputPins(Instance* inst, bool include_top_level) const
{
    auto inst_pins = pins(inst);
    return filterPins(inst_pins, PinDirection::output(), include_top_level);
}

std::vector<InstanceTerm*>
OpenStaHandler::fanoutPins(Net* pin_net, bool include_top_level) const
{
    auto inst_pins = pins(pin_net);

    auto filtered_inst_pins =
        filterPins(inst_pins, PinDirection::input(), include_top_level);

    if (include_top_level)
    {
        std::vector<InstanceTerm*> filtered_top_pins;
        auto itr = network()->connectedPinIterator(pin_net);
        while (itr->hasNext())
        {
            InstanceTerm* term = itr->next();
            if (network()->isTopLevelPort(term) &&
                network()->direction(term)->isOutput())
            {
                filtered_top_pins.push_back(term);
            }
        }
        delete itr;
        filtered_inst_pins.insert(filtered_inst_pins.end(),
                                  filtered_top_pins.begin(),
                                  filtered_top_pins.end());
    }
    return filtered_inst_pins;
}

std::vector<LibraryCell*>
OpenStaHandler::tiehiCells() const
{
    std::vector<LibraryCell*> cells;
    auto                      all_libs = allLibs();
    for (auto& lib : all_libs)
    {
        sta::LibertyCellIterator cell_iter(lib);
        while (cell_iter.hasNext())
        {
            auto cell        = cell_iter.next();
            auto output_pins = libraryOutputPins(cell);

            if (!isSingleOutputCombinational(cell))
            {
                continue;
            }
            auto           output_pin  = output_pins[0];
            sta::FuncExpr* output_func = output_pin->function();
            if (!output_func)
            {
                continue;
            }
            if (output_func->op() == sta::FuncExpr::op_one)
            {
                cells.push_back(cell);
            }
        }
    }
    return cells;
}
std::vector<LibraryCell*>
OpenStaHandler::inverterCells() const
{
    std::vector<LibraryCell*> cells;
    auto                      all_libs = allLibs();
    for (auto& lib : all_libs)
    {
        sta::LibertyCellIterator cell_iter(lib);
        while (cell_iter.hasNext())
        {
            auto cell        = cell_iter.next();
            auto output_pins = libraryOutputPins(cell);
            auto input_pins  = libraryInputPins(cell);

            if (!isSingleOutputCombinational(cell) || input_pins.size() != 1)
            {
                continue;
            }
            auto           output_pin  = output_pins[0];
            sta::FuncExpr* output_func = output_pin->function();
            if (!output_func)
            {
                continue;
            }
            if (output_func->op() == sta::FuncExpr::op_not)
            {
                cells.push_back(cell);
            }
        }
    }
    return cells;
}
LibraryCell*
OpenStaHandler::smallestInverterCell() const
{
    auto inverter_cells = inverterCells();
    if (inverter_cells.size())
    {
        auto smallest = *(inverter_cells.begin());
        for (auto& lib : inverter_cells)
        {
            if (area(lib) < area(smallest))
            {
                smallest = lib;
            }
        }
        return smallest;
    }
    return nullptr;
}
std::vector<LibraryCell*>
OpenStaHandler::bufferCells() const
{
    std::vector<LibraryCell*> cells;
    auto                      all_libs = allLibs();
    for (auto& lib : all_libs)
    {
        auto buff_libs = lib->buffers();
        cells.insert(cells.end(), buff_libs->begin(), buff_libs->end());
    }
    return cells;
}

LibraryCell*
OpenStaHandler::smallestBufferCell() const
{
    auto buff_cells = bufferCells();
    if (buff_cells.size())
    {
        auto smallest = *(buff_cells.begin());
        for (auto& lib : buff_cells)
        {
            if (area(lib) < area(smallest))
            {
                smallest = lib;
            }
        }
        return smallest;
    }
    return nullptr;
}

std::vector<LibraryCell*>
OpenStaHandler::tieloCells() const
{
    std::vector<LibraryCell*> cells;
    auto                      all_libs = allLibs();
    for (auto& lib : all_libs)
    {
        sta::LibertyCellIterator cell_iter(lib);
        while (cell_iter.hasNext())
        {
            auto cell        = cell_iter.next();
            auto output_pins = libraryOutputPins(cell);

            if (!isSingleOutputCombinational(cell))
            {
                continue;
            }
            auto           output_pin  = output_pins[0];
            sta::FuncExpr* output_func = output_pin->function();
            if (!output_func)
            {
                continue;
            }
            if (output_func->op() == sta::FuncExpr::op_zero)
            {
                cells.push_back(cell);
            }
        }
    }
    return cells;
}

std::vector<std::vector<PathPoint>>
OpenStaHandler::bestPath(int path_count) const
{
    return getPaths(false, path_count);
}
std::vector<PathPoint>
OpenStaHandler::criticalPath(int path_count) const
{
    auto paths = getPaths(true, path_count);
    if (paths.size())
    {
        return paths[0];
    }
    return std::vector<PathPoint>();
}
std::vector<PathPoint>
OpenStaHandler::worstSlackPath(InstanceTerm* term) const
{
    sta::PathRef path;
    sta_->vertexWorstSlackPath(vertex(term), sta::MinMax::min(), path);
    return expandPath(&path);
}
std::vector<PathPoint>
OpenStaHandler::worstArrivalPath(InstanceTerm* term) const
{
    sta::PathRef path;
    sta_->vertexWorstArrivalPath(vertex(term), sta::MinMax::max(), path);
    return expandPath(&path);
}

std::vector<PathPoint>
OpenStaHandler::bestSlackPath(InstanceTerm* term) const
{
    sta::PathRef path;
    sta_->vertexWorstSlackPath(vertex(term), sta::MinMax::max(), path);
    return expandPath(&path);
}
std::vector<PathPoint>
OpenStaHandler::bestArrivalPath(InstanceTerm* term) const
{
    sta::PathRef path;
    sta_->vertexWorstArrivalPath(vertex(term), sta::MinMax::min(), path);
    return expandPath(&path);
}
float
OpenStaHandler::slack(InstanceTerm* term, bool is_rise, bool worst) const
{
    return sta_->pinSlack(
        term, is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(),
        worst ? sta::MinMax::min() : sta::MinMax::max());
}
float
OpenStaHandler::slack(InstanceTerm* term, bool worst) const
{
    return sta_->pinSlack(term,
                          worst ? sta::MinMax::min() : sta::MinMax::max());
}
float
OpenStaHandler::slew(InstanceTerm* term, bool is_rise) const
{
    return sta_->vertexSlew(
        vertex(term), is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(),
        sta::MinMax::max());
}
float
OpenStaHandler::arrival(InstanceTerm* term, int ap_index, bool is_rise) const
{

    return sta_->vertexArrival(
        vertex(term), is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(),
        sta_->corners()->findPathAnalysisPt(ap_index));
}
float
OpenStaHandler::required(InstanceTerm* term, bool worst) const
{
    sta_->vertexRequired(vertex(term),
                         worst ? sta::MinMax::min() : sta::MinMax::max());
    return 0;
}
std::vector<std::vector<PathPoint>>
OpenStaHandler::getPaths(bool get_max, int path_count) const
{
    sta_->ensureGraph();
    sta_->searchPreamble();

    sta::PathEndSeq* path_ends =
        sta_->search()->findPathEnds( // from, thrus, to, unconstrained
            nullptr, nullptr, nullptr, false,
            // corner, min_max,
            corner_, get_max ? sta::MinMaxAll::max() : sta::MinMaxAll::min(),
            // group_count, endpoint_count, unique_pins
            path_count, path_count, true, -sta::INF,
            sta::INF, // slack_min, slack_max,
            true,     // sort_by_slack
            nullptr,  // group_names
            // setup, hold, recovery, removal,
            true, true, true, true,
            // clk_gating_setup, clk_gating_hold
            true, true);

    std::vector<std::vector<PathPoint>> result;
    bool                                first_path = true;
    for (auto& path_end : *path_ends)
    {
        result.push_back(expandPath(path_end, !first_path));
        first_path = false;
    }
    delete path_ends;
    return result;
}
std::vector<PathPoint>
OpenStaHandler::expandPath(sta::PathEnd* path_end, bool enumed) const
{
    return expandPath(path_end->path(), enumed);
}

std::vector<PathPoint>
OpenStaHandler::expandPath(sta::Path* path, bool enumed) const
{

    std::vector<PathPoint> points;
    sta::PathExpanded      expanded(path, sta_);
    for (size_t i = 1; i < expanded.size(); i++)
    {
        auto ref           = expanded.path(i);
        auto pin           = ref->vertex(sta_)->pin();
        auto is_rising     = ref->transition(sta_) == sta::RiseFall::rise();
        auto arrival       = ref->arrival(sta_);
        auto required      = enumed ? 0 : ref->required(sta_);
        auto path_ap_index = ref->pathAnalysisPtIndex(sta_);
        auto slack         = enumed ? 0 : ref->slack(sta_);
        points.push_back(
            PathPoint(pin, is_rising, arrival, required, slack, path_ap_index));
    }
    return points;
}

bool
OpenStaHandler::isCommutative(InstanceTerm* first, InstanceTerm* second) const
{
    auto inst = instance(first);
    if (inst != instance(second))
    {
        return false;
    }
    if (first == second)
    {
        return true;
    }
    auto cell_lib = libraryCell(inst);
    if (cell_lib->isClockGate() || cell_lib->isPad() || cell_lib->isMacro() ||
        cell_lib->hasSequentials())
    {
        return false;
    }
    auto                      first_lib   = libraryPin(first);
    auto                      second_lib  = libraryPin(second);
    auto                      output_pins = outputPins(inst, false);
    auto                      input_pins  = inputPins(inst, false);
    std::vector<LibraryTerm*> remaining_pins;
    for (auto& pin : input_pins)
    {
        if (pin != first && pin != second)
        {
            remaining_pins.push_back(libraryPin(pin));
        }
    }
    for (auto& out : output_pins)
    {
        sta::FuncExpr* func = libraryPin(out)->function();
        if (func->hasPort(first_lib) && func->hasPort(second_lib))
        {
            std::unordered_map<LibraryTerm*, int> sim_vals;
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 2; j++)
                {

                    for (int m = 0; m < std::pow(2, remaining_pins.size()); m++)
                    {
                        sim_vals[first_lib]  = i;
                        sim_vals[second_lib] = j;
                        int temp             = m;
                        for (auto& rp : remaining_pins)
                        {
                            sim_vals[rp] = temp & 1;
                            temp         = temp >> 1;
                        }
                        int first_result =
                            evaluateFunctionExpression(out, sim_vals);

                        sim_vals[first_lib]  = j;
                        sim_vals[second_lib] = i;
                        int second_result =
                            evaluateFunctionExpression(out, sim_vals);
                        if (first_result != second_result ||
                            first_result == -1 || second_result == -1)
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}
std::vector<InstanceTerm*>
OpenStaHandler::levelDriverPins() const
{
    auto                       handler_network = network();
    std::vector<InstanceTerm*> terms;
    std::vector<sta::Vertex*>  vertices;
    sta_->ensureLevelized();
    sta::VertexIterator itr(handler_network->graph());
    while (itr.hasNext())
    {
        sta::Vertex* vtx = itr.next();
        if (vtx->isDriver(handler_network))
            vertices.push_back(vtx);
    }
    std::sort(
        vertices.begin(), vertices.end(),
        [=](const sta::Vertex* v1, const sta::Vertex* v2) -> bool {
            return (v1->level() < v2->level()) ||
                   (v1->level() == v2->level() &&
                    sta::stringLess(handler_network->pathName(v1->pin()),
                                    handler_network->pathName(v2->pin())));
        });
    for (auto& v : vertices)
    {
        terms.push_back(v->pin());
    }
    return terms;
}

InstanceTerm*
OpenStaHandler::faninPin(InstanceTerm* term) const
{
    return faninPin(net(term));
}
InstanceTerm*
OpenStaHandler::faninPin(Net* net) const
{
    auto net_pins = pins(net);
    for (auto& pin : net_pins)
    {
        Instance* inst = network()->instance(pin);
        if (inst)
        {
            if (network()->direction(pin)->isOutput())
            {
                return pin;
            }
        }
    }

    return nullptr;
}

std::vector<Instance*>
OpenStaHandler::fanoutInstances(Net* net) const
{
    std::vector<Instance*>     insts;
    std::vector<InstanceTerm*> net_pins = fanoutPins(net, false);
    for (auto& term : net_pins)
    {
        insts.push_back(network()->instance(term));
    }
    return insts;
}

std::vector<Instance*>
OpenStaHandler::driverInstances() const
{
    std::set<Instance*> insts_set;
    for (auto& net : nets())
    {
        InstanceTerm* driverPin = faninPin(net);
        if (driverPin)
        {
            Instance* inst = network()->instance(driverPin);
            if (inst)
            {
                insts_set.insert(inst);
            }
        }
    }
    return std::vector<Instance*>(insts_set.begin(), insts_set.end());
}

unsigned int
OpenStaHandler::fanoutCount(Net* net, bool include_top_level) const
{
    return fanoutPins(net, include_top_level).size();
}

Point
OpenStaHandler::location(InstanceTerm* term)
{
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    network()->staToDb(term, iterm, bterm);
    if (iterm)
    {
        int x, y;
        iterm->getInst()->getOrigin(x, y);
        return Point(x, y);
    }
    if (bterm)
    {
        int x, y;
        if (bterm->getFirstPinLocation(x, y))
            return Point(x, y);
    }
    return Point(0, 0);
}

Point
OpenStaHandler::location(Instance* inst)
{
    odb::dbInst* dinst = network()->staToDb(inst);
    int          x, y;
    dinst->getOrigin(x, y);
    return Point(x, y);
}
void
OpenStaHandler::setLocation(Instance* inst, Point pt)
{
    odb::dbInst* dinst = network()->staToDb(inst);
    dinst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    dinst->setLocation(pt.getX(), pt.getY());
}
float
OpenStaHandler::area(Instance* inst) const
{
    odb::dbInst*   dinst  = network()->staToDb(inst);
    odb::dbMaster* master = dinst->getMaster();
    return dbuToMeters(master->getWidth()) * dbuToMeters(master->getHeight());
}
float
OpenStaHandler::area(LibraryCell* cell) const
{
    odb::dbMaster* master = db_->findMaster(name(cell).c_str());
    return dbuToMeters(master->getWidth()) * dbuToMeters(master->getHeight());
}

float
OpenStaHandler::area() const
{
    float total_area = 0.0;
    for (auto inst : instances())
    {
        total_area += area(inst);
    }
    return total_area;
}
float
OpenStaHandler::power(std::vector<Instance*>& insts)
{
    float            total_pwr = 0.0;
    sta::PowerResult total;
    for (auto inst : insts)
    {
        sta_->power(inst, corner_, total);
        total_pwr += total.total();
    }
    return total_pwr;
}

float
OpenStaHandler::power()
{
    sta::PowerResult total, sequential, combinational, macro, pad;
    sta_->power(corner_, total, sequential, combinational, macro, pad);
    return total.total();
}
LibraryTerm*
OpenStaHandler::libraryPin(InstanceTerm* term) const
{
    return network()->libertyPort(term);
}
bool
OpenStaHandler::isClocked(InstanceTerm* term) const
{
    return sta_->search()->isClock(vertex(term));
}
bool
OpenStaHandler::isPrimary(Net* net) const
{
    for (auto& pin : pins(net))
    {
        if (network()->isTopLevelPort(pin))
        {
            return true;
        }
    }
    return false;
}

LibraryCell*
OpenStaHandler::libraryCell(InstanceTerm* term) const
{
    auto inst = network()->instance(term);
    if (inst)
    {
        return network()->libertyCell(inst);
    }
    return nullptr;
}

LibraryCell*
OpenStaHandler::libraryCell(Instance* inst) const
{
    return network()->libertyCell(inst);
}

LibraryCell*
OpenStaHandler::libraryCell(const char* name) const
{
    return network()->findLibertyCell(name);
}
LibraryCell*
OpenStaHandler::largestLibraryCell(LibraryCell* cell)
{
    if (!has_equiv_cells_)
    {
        makeEquivalentCells();
    }
    auto  equiv_cells = sta_->equivCells(cell);
    auto  largest     = cell;
    float current_max = maxLoad(cell);
    if (equiv_cells)
    {
        for (auto e_cell : *equiv_cells)
        {
            auto cell_load = maxLoad(e_cell);
            if (cell_load > current_max)
            {
                current_max = cell_load;
                largest     = e_cell;
            }
        }
    }
    return largest;
}
double
OpenStaHandler::dbuToMeters(uint dist) const
{
    return dist * 1E-9;
}
double
OpenStaHandler::dbuToMicrons(uint dist) const
{
    return (1.0 * dist) / db_->getTech()->getLefUnits();
}
bool
OpenStaHandler::isPlaced(InstanceTerm* term) const
{
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    network()->staToDb(term, iterm, bterm);
    odb::dbPlacementStatus status = odb::dbPlacementStatus::UNPLACED;
    if (iterm)
    {
        odb::dbInst* inst = iterm->getInst();
        status            = inst->getPlacementStatus();
    }
    if (bterm)
        status = bterm->getFirstPinPlacementStatus();
    return status == odb::dbPlacementStatus::PLACED ||
           status == odb::dbPlacementStatus::LOCKED ||
           status == odb::dbPlacementStatus::FIRM ||
           status == odb::dbPlacementStatus::COVER;
}
bool
OpenStaHandler::isPlaced(Instance* inst) const
{
    odb::dbInst*           dinst  = network()->staToDb(inst);
    odb::dbPlacementStatus status = dinst->getPlacementStatus();
    return status == odb::dbPlacementStatus::PLACED ||
           status == odb::dbPlacementStatus::LOCKED ||
           status == odb::dbPlacementStatus::FIRM ||
           status == odb::dbPlacementStatus::COVER;
}
bool
OpenStaHandler::isDriver(InstanceTerm* term) const
{
    return network()->isDriver(term);
}

float
OpenStaHandler::pinCapacitance(InstanceTerm* term) const
{
    auto port = network()->libertyPort(term);
    if (port)
    {
        return pinCapacitance(port);
    }
    return 0.0;
}
float
OpenStaHandler::pinCapacitance(LibraryTerm* term) const
{
    float cap1 = term->capacitance(sta::RiseFall::rise(), min_max_);
    float cap2 = term->capacitance(sta::RiseFall::fall(), min_max_);
    return std::max(cap1, cap2);
}

float
OpenStaHandler::pinAverageRise(LibraryTerm* from, LibraryTerm* to) const
{
    return pinTableAverage(from, to, true, true);
}
float
OpenStaHandler::pinAverageFall(LibraryTerm* from, LibraryTerm* to) const
{
    return pinTableAverage(from, to, true, false);
}
float
OpenStaHandler::pinAverageRiseTransition(LibraryTerm* from,
                                         LibraryTerm* to) const
{
    return pinTableAverage(from, to, false, true);
}
float
OpenStaHandler::pinAverageFallTransition(LibraryTerm* from,
                                         LibraryTerm* to) const
{
    return pinTableAverage(from, to, false, false);
}
float
OpenStaHandler::pinTableLookup(LibraryTerm* from, LibraryTerm* to, float slew,
                               float cap, bool is_delay, bool is_rise) const
{
    auto                  lib_cell        = from->libertyCell();
    sta::TimingArcSetSeq* timing_arc_sets = lib_cell->timingArcSets(from, to);
    if (!timing_arc_sets)
    {
        return sta::INF;
    }
    for (auto& arc_set : *timing_arc_sets)
    {
        sta::TimingArcSetArcIterator itr(arc_set);
        while (itr.hasNext())
        {
            sta::TimingArc* arc = itr.next();
            if ((is_rise &&
                 arc->toTrans()->asRiseFall() != sta::RiseFall::rise()) ||
                (!is_rise &&
                 arc->toTrans()->asRiseFall() != sta::RiseFall::fall()))
            {
                continue;
            }
            sta::GateTableModel* model =
                dynamic_cast<sta::GateTableModel*>(arc->model());
            auto delay_slew_model =
                is_delay ? model->delayModel() : model->slewModel();
            if (model)
            {
                return delay_slew_model->findValue(
                    lib_cell->libertyLibrary(), lib_cell, pvt_, slew, cap, 0);
            }
        }
    }

    return sta::INF;
}
float
OpenStaHandler::pinTableAverage(LibraryTerm* from, LibraryTerm* to,
                                bool is_delay, bool is_rise) const
{
    auto                  lib_cell        = from->libertyCell();
    sta::TimingArcSetSeq* timing_arc_sets = lib_cell->timingArcSets(from, to);
    if (!timing_arc_sets)
    {
        return sta::INF;
    }
    float sum   = 0;
    int   count = 0;
    for (auto& arc_set : *timing_arc_sets)
    {
        sta::TimingArcSetArcIterator itr(arc_set);
        while (itr.hasNext())
        {
            sta::TimingArc* arc = itr.next();
            if ((is_rise &&
                 arc->toTrans()->asRiseFall() != sta::RiseFall::rise()) ||
                (!is_rise &&
                 arc->toTrans()->asRiseFall() != sta::RiseFall::fall()))
            {
                continue;
            }
            sta::GateTableModel* model =
                dynamic_cast<sta::GateTableModel*>(arc->model());
            auto delay_slew_model =
                is_delay ? model->delayModel() : model->slewModel();
            if (model)
            {
                auto axis1 = delay_slew_model->axis1();
                auto axis2 = delay_slew_model->axis2();
                for (size_t i = 0; i < axis1->size(); i++)
                {
                    for (size_t j = 0; j < axis2->size(); j++)
                    {
                        sum += delay_slew_model->findValue(
                            lib_cell->libertyLibrary(), lib_cell, pvt_,
                            axis1->axisValue(i), axis2->axisValue(j), 0);
                        count++;
                    }
                }
            }
        }
    }
    if (sum == 0)
    {
        return sta::INF;
    }
    return sum / count;
}
float
OpenStaHandler::loadCapacitance(InstanceTerm* term) const
{
    return network()->graphDelayCalc()->loadCap(term, dcalc_ap_);
}
Instance*
OpenStaHandler::instance(const char* name) const
{
    return network()->findInstance(name);
}
BlockTerm*
OpenStaHandler::port(const char* name) const
{
    return network()->findPin(network()->topInstance(), name);
}

Instance*
OpenStaHandler::instance(InstanceTerm* term) const
{
    return network()->instance(term);
}
Net*
OpenStaHandler::net(const char* name) const
{
    return network()->findNet(name);
}

LibraryTerm*
OpenStaHandler::libraryPin(const char* cell_name, const char* pin_name) const
{
    LibraryCell* cell = libraryCell(cell_name);
    if (!cell)
    {
        return nullptr;
    }
    return libraryPin(cell, pin_name);
}
LibraryTerm*
OpenStaHandler::libraryPin(LibraryCell* cell, const char* pin_name) const
{
    return cell->findLibertyPort(pin_name);
}

std::vector<LibraryTerm*>
OpenStaHandler::libraryPins(Instance* inst) const
{
    return libraryPins(libraryCell(inst));
}
std::vector<LibraryTerm*>
OpenStaHandler::libraryPins(LibraryCell* cell) const
{
    std::vector<LibraryTerm*>    pins;
    sta::LibertyCellPortIterator itr(cell);
    while (itr.hasNext())
    {
        auto port = itr.next();
        pins.push_back(port);
    }
    return pins;
}
std::vector<LibraryTerm*>
OpenStaHandler::libraryInputPins(LibraryCell* cell) const
{
    auto pins = libraryPins(cell);
    for (auto it = pins.begin(); it != pins.end(); it++)
    {
        if (!((*it)->direction()->isAnyInput()))
        {
            it = pins.erase(it);
            it--;
        }
    }
    return pins;
}
std::vector<LibraryTerm*>
OpenStaHandler::libraryOutputPins(LibraryCell* cell) const
{
    auto pins = libraryPins(cell);
    for (auto it = pins.begin(); it != pins.end(); it++)
    {
        if (!((*it)->direction()->isAnyOutput()))
        {
            it = pins.erase(it);
            it--;
        }
    }
    return pins;
}

std::vector<InstanceTerm*>
OpenStaHandler::filterPins(std::vector<InstanceTerm*>& terms,
                           PinDirection*               direction,
                           bool                        include_top_level) const
{
    std::vector<InstanceTerm*> inst_terms;

    for (auto& term : terms)
    {
        Instance* inst = network()->instance(term);
        if (inst)
        {
            if (term && network()->direction(term) == direction)
            {
                inst_terms.push_back(term);
            }
        }
        else if (include_top_level)
        {
        }
    }
    return inst_terms;
}

bool
OpenStaHandler::isTopLevel(InstanceTerm* term) const
{
    return network()->isTopLevelPort(term);
}
void
OpenStaHandler::del(Net* net) const
{
    network()->deleteNet(net);
}
void
OpenStaHandler::del(Instance* inst) const
{
    network()->deleteInstance(inst);
}
int
OpenStaHandler::disconnectAll(Net* net) const
{
    int count = 0;
    for (auto& pin : pins(net))
    {
        network()->disconnectPin(pin);
        count++;
    }

    return count;
}

void
OpenStaHandler::connect(Net* net, InstanceTerm* term) const
{
    auto inst      = network()->instance(term);
    auto term_port = network()->port(term);
    network()->connect(inst, term_port, net);
}

void
OpenStaHandler::disconnect(InstanceTerm* term) const
{
    network()->disconnectPin(term);
}

void
OpenStaHandler::swapPins(InstanceTerm* first, InstanceTerm* second)
{
    auto first_net  = net(first);
    auto second_net = net(second);
    disconnect(first);
    disconnect(second);
    connect(first_net, second);
    connect(second_net, first);
    resetDelays(first);
    resetDelays(second);
}
Instance*
OpenStaHandler::createInstance(const char* inst_name, LibraryCell* cell)
{
    return network()->makeInstance(cell, inst_name, network()->topInstance());
}
// namespace psn
void
OpenStaHandler::createClock(const char*             clock_name,
                            std::vector<BlockTerm*> ports, float period)
{
    sta::PinSet* pin_set = new sta::PinSet;
    for (auto& p : ports)
    {
        pin_set->insert(p);
    }

    sta::FloatSeq* waveform = new sta::FloatSeq;
    waveform->push_back(0);
    waveform->push_back(period / 2.0f);
    const char* comment = "";
    sta_->makeClock(clock_name, pin_set, false, period, waveform,
                    const_cast<char*>(comment));
}
void
OpenStaHandler::createClock(const char*              clock_name,
                            std::vector<std::string> port_names, float period)
{
    std::vector<BlockTerm*> ports;
    for (auto& p : port_names)
    {
        ports.push_back(port(p.c_str()));
    }
    createClock(clock_name, ports, period);
}

Net*
OpenStaHandler::createNet(const char* net_name)
{
    return network()->makeNet(net_name, network()->topInstance());
}

InstanceTerm*
OpenStaHandler::connect(Net* net, Instance* inst, LibraryTerm* port) const
{
    return network()->connect(inst, port, net);
}

std::vector<Net*>
OpenStaHandler::nets() const
{
    sta::NetSeq       all_nets;
    sta::PatternMatch pattern("*");
    network()->findNetsMatching(network()->topInstance(), &pattern, &all_nets);
    return static_cast<std::vector<Net*>>(all_nets);
}
std::vector<Instance*>
OpenStaHandler::instances() const
{
    sta::InstanceSeq  all_insts;
    sta::PatternMatch pattern("*");
    network()->findInstancesMatching(network()->topInstance(), &pattern,
                                     &all_insts);
    return static_cast<std::vector<Instance*>>(all_insts);
}

std::string
OpenStaHandler::topName() const
{
    return std::string(network()->name(network()->topInstance()));
}
std::string
OpenStaHandler::name(Block* object) const
{
    return std::string(object->getConstName());
}
std::string
OpenStaHandler::name(Net* object) const
{
    return std::string(network()->name(object));
}
std::string
OpenStaHandler::name(Instance* object) const
{
    return std::string(network()->name(object));
}
std::string
OpenStaHandler::name(BlockTerm* object) const
{
    return std::string(network()->name(object));
}
std::string
OpenStaHandler::name(Library* object) const
{
    return std::string(object->getConstName());
}
std::string
OpenStaHandler::name(LibraryCell* object) const
{
    return std::string(network()->name(network()->cell(object)));
}
std::string
OpenStaHandler::name(LibraryTerm* object) const
{
    return object->name();
}

Library*
OpenStaHandler::library() const
{
    LibrarySet libs = db_->getLibs();

    if (!libs.size())
    {
        return nullptr;
    }
    Library* lib = *(libs.begin());
    return lib;
}

LibraryTechnology*
OpenStaHandler::technology() const
{
    Library* lib = library();
    if (!lib)
    {
        return nullptr;
    }
    LibraryTechnology* tech = lib->getTech();
    return tech;
}

Block*
OpenStaHandler::top() const
{
    Chip* chip = db_->getChip();
    if (!chip)
    {
        return nullptr;
    }

    Block* block = chip->getBlock();
    return block;
}

OpenStaHandler::~OpenStaHandler()
{
}
void
OpenStaHandler::clear()
{
    sta_->clear();
    db_->clear();
}
DatabaseStaNetwork*
OpenStaHandler::network() const
{
    return sta_->getDbNetwork();
}
DatabaseSta*
OpenStaHandler::sta() const
{
    return sta_;
}
float
OpenStaHandler::maxLoad(LibraryCell* cell)
{
    sta::LibertyCellPortIterator itr(cell);
    while (itr.hasNext())
    {
        auto port = itr.next();
        if (port->direction() == PinDirection::output())
        {
            float limit;
            bool  exists;
            port->capacitanceLimit(min_max_, limit, exists);
            if (exists)
            {
                return limit;
            }
        }
    }
    return 0;
}
float
OpenStaHandler::maxLoad(LibraryTerm* term)
{
    float limit;
    bool  exists;
    term->capacitanceLimit(min_max_, limit, exists);
    if (exists)
    {
        return limit;
    }
    return 0;
}
bool
OpenStaHandler::isInput(InstanceTerm* term) const
{
    return network()->direction(term)->isInput();
}
bool
OpenStaHandler::isOutput(InstanceTerm* term) const
{
    return network()->direction(term)->isOutput();
}
bool
OpenStaHandler::isAnyInput(InstanceTerm* term) const
{
    return network()->direction(term)->isAnyInput();
}
bool
OpenStaHandler::isAnyOutput(InstanceTerm* term) const
{
    return network()->direction(term)->isAnyOutput();
}
bool
OpenStaHandler::isBiDriect(InstanceTerm* term) const
{
    return network()->direction(term)->isBidirect();
}
bool
OpenStaHandler::isTriState(InstanceTerm* term) const
{
    return network()->direction(term)->isTristate();
}
bool
OpenStaHandler::isSingleOutputCombinational(Instance* inst) const
{
    if (inst == network()->topInstance())
    {
        return false;
    }
    return isSingleOutputCombinational(libraryCell(inst));
}
bool
OpenStaHandler::isSingleOutputCombinational(LibraryCell* cell) const
{
    if (!cell)
    {
        return false;
    }
    auto output_pins = libraryOutputPins(cell);

    return (output_pins.size() == 1 && isCombinational(cell));
}
bool
OpenStaHandler::isCombinational(Instance* inst) const
{
    if (inst == network()->topInstance())
    {
        return false;
    }
    return isCombinational(libraryCell(inst));
}
bool
OpenStaHandler::isCombinational(LibraryCell* cell) const
{
    if (!cell)
    {
        return false;
    }
    return (!cell->isClockGate() && !cell->isPad() && !cell->isMacro() &&
            !cell->hasSequentials());
}

bool
OpenStaHandler::isInput(LibraryTerm* term) const
{
    return term->direction()->isInput();
}
bool
OpenStaHandler::isOutput(LibraryTerm* term) const
{
    return term->direction()->isOutput();
}
bool
OpenStaHandler::isAnyInput(LibraryTerm* term) const
{
    return term->direction()->isAnyInput();
}
bool
OpenStaHandler::isAnyOutput(LibraryTerm* term) const
{
    return term->direction()->isAnyOutput();
}
bool
OpenStaHandler::isBiDriect(LibraryTerm* term) const
{
    return term->direction()->isBidirect();
}
bool
OpenStaHandler::isTriState(LibraryTerm* term) const
{
    return term->direction()->isTristate();
}
bool
OpenStaHandler::violatesMaximumCapacitance(InstanceTerm* term) const
{
    float load_cap = loadCapacitance(term);
    return violatesMaximumCapacitance(term, load_cap);
}
bool
OpenStaHandler::violatesMaximumCapacitance(InstanceTerm* term,
                                           float         load_cap) const
{
    LibraryTerm* port = network()->libertyPort(term);
    if (port)
    {
        float cap_limit;
        bool  exists;
        port->capacitanceLimit(sta::MinMax::max(), cap_limit, exists);
        return exists && load_cap > cap_limit;
    }
    return false;
}

bool
OpenStaHandler::violatesMaximumTransition(InstanceTerm* term) const
{
    auto  vert = network()->graph()->pinDrvrVertex(term);
    float limit;
    bool  exists;
    slewLimit(term, sta::MinMax::max(), limit, exists);
    for (auto rf : sta::RiseFall::range())
    {
        auto slew = network()->graph()->slew(vert, rf, dcalc_ap_->index());
        if (slew > limit)
            return true;
    }
    return false;
}
bool
OpenStaHandler::isLoad(InstanceTerm* term) const
{
    return network()->isLoad(term);
}
void
OpenStaHandler::makeEquivalentCells()
{
    sta::LibertyLibrarySeq       map_libs;
    sta::LibertyLibraryIterator* lib_iter = network()->libertyLibraryIterator();
    while (lib_iter->hasNext())
    {
        auto lib = lib_iter->next();
        map_libs.push_back(lib);
    }
    delete lib_iter;
    auto all_libs = allLibs();
    sta_->makeEquivCells(&all_libs, &map_libs);
    has_equiv_cells_ = true;
}
sta::LibertyLibrarySeq
OpenStaHandler::allLibs() const
{
    sta::LibertyLibrarySeq       seq;
    sta::LibertyLibraryIterator* iter = network()->libertyLibraryIterator();
    while (iter->hasNext())
    {
        sta::LibertyLibrary* lib = iter->next();
        seq.push_back(lib);
    }
    delete iter;
    return seq;
}
void
OpenStaHandler::resetCache()
{
    has_equiv_cells_  = false;
    has_target_loads_ = false;
    target_load_map_.clear();
}
void
OpenStaHandler::findTargetLoads()
{
    auto all_libs = allLibs();
    findTargetLoads(&all_libs);
    has_target_loads_ = true;
}

sta::Vertex*
OpenStaHandler::vertex(InstanceTerm* term) const
{
    sta::Vertex *vertex, *bidirect_drvr_vertex;
    sta_->graph()->pinVertices(term, vertex, bidirect_drvr_vertex);
    return vertex;
}

int
OpenStaHandler::evaluateFunctionExpression(
    InstanceTerm* term, std::unordered_map<LibraryTerm*, int>& inputs) const
{
    return evaluateFunctionExpression(libraryPin(term), inputs);
}
int
OpenStaHandler::evaluateFunctionExpression(
    LibraryTerm* term, std::unordered_map<LibraryTerm*, int>& inputs) const
{
    return evaluateFunctionExpression(term->function(), inputs);
}
int
OpenStaHandler::evaluateFunctionExpression(
    sta::FuncExpr* func, std::unordered_map<LibraryTerm*, int>& inputs) const
{
    int left;
    int right;
    switch (func->op())
    {
    case sta::FuncExpr::op_port:
        if (inputs.count(func->port()))
        {
            return inputs[func->port()];
        }
        else
        {
            return -1;
        }
    case sta::FuncExpr::op_not:
        left = evaluateFunctionExpression(func->left(), inputs);
        if (left == -1)
        {
            return -1;
        }
        return !left;
    case sta::FuncExpr::op_or:
        left = evaluateFunctionExpression(func->left(), inputs);
        if (left == -1)
        {
            return -1;
        }
        right = evaluateFunctionExpression(func->right(), inputs);
        if (right == -1)
        {
            return -1;
        }
        return left | right;
    case sta::FuncExpr::op_and:
        left = evaluateFunctionExpression(func->left(), inputs);
        if (left == -1)
        {
            return -1;
        }
        right = evaluateFunctionExpression(func->right(), inputs);
        if (right == -1)
        {
            return -1;
        }
        return left & right;
    case sta::FuncExpr::op_xor:
        left = evaluateFunctionExpression(func->left(), inputs);
        if (left == -1)
        {
            return -1;
        }
        right = evaluateFunctionExpression(func->right(), inputs);
        if (right == -1)
        {
            return -1;
        }
        return left ^ right;
    case sta::FuncExpr::op_one:
        return 1;
    case sta::FuncExpr::op_zero:
        return 0;
    default:
        return -1;
    }
} // namespace psn

/* The following is borrowed from James Cherry's Resizer Code */

// Find a target slew for the libraries and then
// a target load for each cell that gives the target slew.
void
OpenStaHandler::findTargetLoads(sta::LibertyLibrarySeq* resize_libs)
{
    // Find target slew across all buffers in the libraries.
    findBufferTargetSlews(resize_libs);
    for (auto lib : *resize_libs)
        findTargetLoads(lib, target_slews_);
}

float
OpenStaHandler::targetLoad(LibraryCell* cell)
{
    if (!has_target_loads_)
    {
        findTargetLoads();
    }
    if (target_load_map_.count(cell))
    {
        return target_load_map_[cell];
    }
    return 0.0;
}

float
OpenStaHandler::gateDelay(Instance* inst, InstanceTerm* to, float in_slew,
                          LibraryTerm* from, float* drvr_slew, int rise_fall)
{
    sta::ArcDelay                        max      = -sta::INF;
    auto                                 lib_cell = libraryCell(inst);
    sta::LibertyCellTimingArcSetIterator itr(lib_cell);
    while (itr.hasNext())
    {
        sta::TimingArcSet* arc_set = itr.next();
        if (arc_set->to() == libraryPin(to))
        {
            sta::TimingArcSetArcIterator arc_it(arc_set);
            while (arc_it.hasNext())
            {
                sta::TimingArc* arc = arc_it.next();
                if (from && arc->from() != from)
                {
                    continue;
                }
                if (rise_fall != -1)
                {
                    if (rise_fall)
                    { //  1 is rising edge
                        if (arc->toTrans()->asRiseFall() !=
                            sta::RiseFall::rise())
                        {
                            continue;
                        }
                    }
                    else
                    { // 0 is falling edge

                        if (arc->toTrans()->asRiseFall() !=
                            sta::RiseFall::fall())
                        {
                            continue;
                        }
                    }
                }

                float         load_cap = loadCapacitance(to);
                sta::ArcDelay gate_delay;
                sta::Slew     tmp_slew;
                sta::Slew*    slew = drvr_slew ? drvr_slew : &tmp_slew;
                sta_->arcDelayCalc()->gateDelay(lib_cell, arc, in_slew,
                                                load_cap, nullptr, 0.0, pvt_,
                                                dcalc_ap_, gate_delay, *slew);
                max = std::max(max, gate_delay);
            }
        }
    }
    return max;
}
void
OpenStaHandler::findTargetLoads(Liberty* library, sta::Slew slews[])
{
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext())
    {
        auto cell = cell_iter.next();
        findTargetLoad(cell, slews);
    }
}

void
OpenStaHandler::findTargetLoad(LibraryCell* cell, sta::Slew slews[])
{
    sta::LibertyCellTimingArcSetIterator arc_set_iter(cell);
    float                                target_load_sum = 0.0;
    int                                  arc_count       = 0;
    while (arc_set_iter.hasNext())
    {
        auto arc_set = arc_set_iter.next();
        auto role    = arc_set->role();
        if (!role->isTimingCheck() &&
            role != sta::TimingRole::tristateDisable() &&
            role != sta::TimingRole::tristateEnable())
        {
            sta::TimingArcSetArcIterator arc_iter(arc_set);
            while (arc_iter.hasNext())
            {
                sta::TimingArc* arc    = arc_iter.next();
                sta::RiseFall*  in_rf  = arc->fromTrans()->asRiseFall();
                sta::RiseFall*  out_rf = arc->toTrans()->asRiseFall();
                float           arc_target_load = findTargetLoad(
                    cell, arc, slews[in_rf->index()], slews[out_rf->index()]);
                target_load_sum += arc_target_load;
                arc_count++;
            }
        }
    }
    float target_load = (arc_count > 0) ? target_load_sum / arc_count : 0.0;
    target_load_map_[cell] = target_load;
}

// Find the load capacitance that will cause the output slew
// to be equal to out_slew.
float
OpenStaHandler::findTargetLoad(LibraryCell* cell, sta::TimingArc* arc,
                               sta::Slew in_slew, sta::Slew out_slew)
{
    sta::GateTimingModel* model =
        dynamic_cast<sta::GateTimingModel*>(arc->model());
    if (model)
    {
        float cap_init = 1.0e-12;         // 1pF
        float cap_tol  = cap_init * .001; // .1%
        float load_cap = cap_init;
        float cap_step = cap_init;
        while (cap_step > cap_tol)
        {
            sta::ArcDelay arc_delay;
            sta::Slew     arc_slew;
            model->gateDelay(cell, pvt_, in_slew, load_cap, 0.0, false,
                             arc_delay, arc_slew);
            if (arc_slew > out_slew)
            {
                load_cap -= cap_step;
                cap_step /= 2.0;
            }
            load_cap += cap_step;
        }
        return load_cap;
    }
    return 0.0;
}
void
OpenStaHandler::slewLimit(InstanceTerm* pin, sta::MinMax* min_max,
                          // Return values.
                          float& limit, bool& exists) const

{
    exists         = false;
    auto  top_cell = network()->cell(network()->topInstance());
    float top_limit;
    bool  top_limit_exists;
    network()->sdc()->slewLimit(top_cell, min_max, top_limit, top_limit_exists);

    // Default to top ("design") limit.
    exists = top_limit_exists;
    limit  = top_limit;
    if (network()->isTopLevelPort(pin))
    {
        auto  port = network()->port(pin);
        float port_limit;
        bool  port_limit_exists;
        network()->sdc()->slewLimit(port, min_max, port_limit,
                                    port_limit_exists);
        // Use the tightest limit.
        if (port_limit_exists &&
            (!exists || min_max->compare(limit, port_limit)))
        {
            limit  = port_limit;
            exists = true;
        }
    }
    else
    {
        float pin_limit;
        bool  pin_limit_exists;
        network()->sdc()->slewLimit(pin, min_max, pin_limit, pin_limit_exists);
        // Use the tightest limit.
        if (pin_limit_exists && (!exists || min_max->compare(limit, pin_limit)))
        {
            limit  = pin_limit;
            exists = true;
        }

        float port_limit;
        bool  port_limit_exists;
        auto  port = network()->libertyPort(pin);
        if (port)
        {
            port->slewLimit(min_max, port_limit, port_limit_exists);
            // Use the tightest limit.
            if (port_limit_exists &&
                (!exists || min_max->compare(limit, port_limit)))
            {
                limit  = port_limit;
                exists = true;
            }
        }
    }
}

float
OpenStaHandler::gateDelay(LibraryTerm* out_port, float load_cap)
{
    auto cell = out_port->libertyCell();
    // Max rise/fall delays.
    sta::ArcDelay                        max_delay = -sta::INF;
    sta::LibertyCellTimingArcSetIterator set_iter(cell);
    while (set_iter.hasNext())
    {
        sta::TimingArcSet* arc_set = set_iter.next();
        if (arc_set->to() == out_port)
        {
            sta::TimingArcSetArcIterator arc_iter(arc_set);
            while (arc_iter.hasNext())
            {
                sta::TimingArc* arc     = arc_iter.next();
                sta::RiseFall*  in_rf   = arc->fromTrans()->asRiseFall();
                float           in_slew = target_slews_[in_rf->index()];
                sta::ArcDelay   gate_delay;
                sta::Slew       drvr_slew;
                sta_->arcDelayCalc()->gateDelay(cell, arc, in_slew, load_cap,
                                                nullptr, 0.0, pvt_, dcalc_ap_,
                                                gate_delay, drvr_slew);
                max_delay = std::max(max_delay, gate_delay);
            }
        }
    }
    return max_delay;
}

float
OpenStaHandler::bufferDelay(psn::LibraryCell* buffer_cell, float load_cap)
{
    psn::LibraryTerm *input, *output;
    buffer_cell->bufferPorts(input, output);
    return gateDelay(output, load_cap);
}

float
OpenStaHandler::portCapacitance(const LibraryTerm* port, bool isMax)
{
    float cap1 = port->capacitance(
        sta::RiseFall::rise(), isMax ? sta::MinMax::max() : sta::MinMax::min());
    float cap2 = port->capacitance(
        sta::RiseFall::fall(), isMax ? sta::MinMax::max() : sta::MinMax::min());
    return std::max(cap1, cap2);
}
float
OpenStaHandler::bufferInputCapacitance(LibraryCell* buffer_cell)
{
    LibraryTerm *input, *output;
    buffer_cell->bufferPorts(input, output);
    return portCapacitance(input);
}
float
OpenStaHandler::bufferOutputCapacitance(LibraryCell* buffer_cell)
{
    LibraryTerm *input, *output;
    buffer_cell->bufferPorts(input, output);
    return portCapacitance(input);
}
////////////////////////////////////////////////////////////////

sta::Slew
OpenStaHandler::targetSlew(const sta::RiseFall* rf)
{
    return target_slews_[rf->index()];
}

// Find target slew across all buffers in the libraries.
void
OpenStaHandler::findBufferTargetSlews(sta::LibertyLibrarySeq* resize_libs)
{
    target_slews_[sta::RiseFall::riseIndex()] = 0.0;
    target_slews_[sta::RiseFall::fallIndex()] = 0.0;
    int tgt_counts[sta::RiseFall::index_count]{0};

    for (auto lib : *resize_libs)
    {
        sta::Slew slews[sta::RiseFall::index_count]{0.0};
        int       counts[sta::RiseFall::index_count]{0};

        findBufferTargetSlews(lib, slews, counts);
        for (auto rf : sta::RiseFall::rangeIndex())
        {
            target_slews_[rf] += slews[rf];
            tgt_counts[rf] += counts[rf];
            slews[rf] /= counts[rf];
        }
    }

    for (auto rf : sta::RiseFall::rangeIndex())
        target_slews_[rf] /= tgt_counts[rf];
}
bool
OpenStaHandler::dontUse(LibraryCell* cell) const
{
    return cell->dontUse();
}
bool
OpenStaHandler::dontTouch(Instance*) const
{
    return false;
}
void
OpenStaHandler::resetDelays()
{
    sta_->graphDelayCalc()->delaysInvalid();
    sta_->search()->arrivalsInvalid();
    sta_->search()->requiredsInvalid();
    sta_->search()->endpointsInvalid();
    if (top())
    {
        sta_->findDelays();
    }
}
void
OpenStaHandler::resetDelays(InstanceTerm* term)
{
    sta_->delaysInvalidFrom(term);
    sta_->delaysInvalidFromFanin(term);
}
void
OpenStaHandler::findBufferTargetSlews(Liberty* library,
                                      // Return values.
                                      sta::Slew slews[], int counts[])
{
    for (auto buffer : *library->buffers())
    {
        if (!dontUse(buffer))
        {
            sta::LibertyPort *input, *output;
            buffer->bufferPorts(input, output);
            auto arc_sets = buffer->timingArcSets(input, output);
            if (arc_sets)
            {
                for (auto arc_set : *arc_sets)
                {
                    sta::TimingArcSetArcIterator arc_iter(arc_set);
                    while (arc_iter.hasNext())
                    {
                        sta::TimingArc*       arc = arc_iter.next();
                        sta::GateTimingModel* model =
                            dynamic_cast<sta::GateTimingModel*>(arc->model());
                        sta::RiseFall* in_rf  = arc->fromTrans()->asRiseFall();
                        sta::RiseFall* out_rf = arc->toTrans()->asRiseFall();
                        float in_cap   = input->capacitance(in_rf, min_max_);
                        float load_cap = in_cap * 10.0; // "factor debatable"
                        sta::ArcDelay arc_delay;
                        sta::Slew     arc_slew;
                        model->gateDelay(buffer, pvt_, 0.0, load_cap, 0.0,
                                         false, arc_delay, arc_slew);
                        model->gateDelay(buffer, pvt_, arc_slew, load_cap, 0.0,
                                         false, arc_delay, arc_slew);
                        slews[out_rf->index()] += arc_slew;
                        counts[out_rf->index()]++;
                    }
                }
            }
        }
    }
}
HandlerType
OpenStaHandler::handlerType() const
{
    return HandlerType::OPENSTA;
}
} // namespace psn
