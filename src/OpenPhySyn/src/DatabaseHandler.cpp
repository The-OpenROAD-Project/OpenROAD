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

#include "OpenPhySyn/DatabaseHandler.hpp"

#include "sta/Graph.hh"
#include "sta/Parasitics.hh"

#include <algorithm>
#include <set>
#include "OpenPhySyn/PsnGlobal.hpp"
#include "OpenPhySyn/PsnLogger.hpp"
#include "OpenPhySyn/SteinerTree.hpp"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "opendb/db.h"
#include "opendb/dbTypes.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/Bfs.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Power.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"

namespace psn
{
OpenStaHandler::OpenStaHandler(Psn* psn_inst, DatabaseSta* sta)
    : sta_(sta),
      db_(sta->db()),
      has_equiv_cells_(false),
      has_buffer_inverter_seq_(false),
      has_target_loads_(false),
      psn_(psn_inst),
      has_wire_rc_(false),
      maximum_area_valid_(false)
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
Net*
OpenStaHandler::bufferNet(Net* b_net, LibraryCell* buffer,
                          std::string buffer_name, std::string net_name,
                          Point location)
{
    auto buf_inst      = createInstance(buffer_name.c_str(), buffer);
    auto buf_net       = createNet(net_name.c_str());
    auto buff_in_port  = bufferInputPin(buffer);
    auto buff_out_port = bufferOutputPin(buffer);
    connect(b_net, buf_inst, buff_in_port);
    connect(buf_net, buf_inst, buff_out_port);
    setLocation(buf_inst, location);
    if (b_net && hasWireRC())
    {
        calculateParasitics(b_net);
    }
    if (buf_net && hasWireRC())
    {
        calculateParasitics(buf_net);
    }
    auto db_net = network()->staToDb(b_net);
    if (db_net)
    {
        db_net->setBuffered(true);
    }
    return buf_net;
}
std::set<InstanceTerm*>
OpenStaHandler::clockPins() const
{
    std::set<InstanceTerm*> clock_pins;
    auto                    clk_iter = new sta::ClockIterator(network()->sdc());
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

            if (isSingleOutputCombinational(cell))
            {
                auto           output_pin  = output_pins[0];
                sta::FuncExpr* output_func = output_pin->function();
                if (output_func && output_func->op() == sta::FuncExpr::op_one)
                {
                    cells.push_back(cell);
                }
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

            if (isSingleOutputCombinational(cell) && input_pins.size() == 1)
            {
                auto           output_pin  = output_pins[0];
                sta::FuncExpr* output_func = output_pin->function();
                if (output_func && output_func->op() == sta::FuncExpr::op_not)
                {
                    cells.push_back(cell);
                }
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

// cluster_threshold:
// 1   : Single buffer cell
// 3/4 : Small set
// 1/4 : Medium set
// 1/12: Large set
// 0   : All buffer cells
std::pair<std::vector<LibraryCell*>, std::vector<LibraryCell*>>
OpenStaHandler::bufferClusters(float cluster_threshold, bool find_superior,
                               bool include_inverting)
{
    std::vector<LibraryCell*>        buffer_cells, inverter_cells;
    std::unordered_set<LibraryCell*> superior_buffer_cells,
        superior_inverter_cells;
    for (auto& buf : bufferCells())
    {
        if (!dontUse(buf))
        {
            buffer_cells.push_back(buf);
        }
    }
    if (include_inverting)
    {
        for (auto& inv : inverterCells())
        {
            if (!dontUse(inv))
            {
                inverter_cells.push_back(inv);
            }
        }
    }
    std::sort(buffer_cells.begin(), buffer_cells.end(),
              [=](LibraryCell* b1, LibraryCell* b2) -> bool {
                  return bufferInputCapacitance(b1) <
                         bufferInputCapacitance(b2);
              });
    std::sort(inverter_cells.begin(), inverter_cells.end(),
              [=](LibraryCell* b1, LibraryCell* b2) -> bool {
                  return bufferInputCapacitance(b1) <
                         bufferInputCapacitance(b2);
              });

    // Get the smallest capacitance/resistance.
    float min_buff_cap = bufferInputCapacitance(buffer_cells[0]);
    float min_buff_resistance =
        bufferOutputPin(buffer_cells[buffer_cells.size() - 1])
            ->driveResistance();
    float min_buff_slew      = 20E-12;
    float min_inv_cap        = 0.0;
    float min_inv_resistance = 0.0;
    float min_inv_slew       = 20E-12;
    if (inverter_cells.size())
    {
        min_inv_cap = bufferInputCapacitance(inverter_cells[0]);
        float min_inv_resistance =
            bufferOutputPin(inverter_cells[inverter_cells.size() - 1])
                ->driveResistance();
    }

    // Find superior buffers/inverters.
    if (!find_superior)
    {
        superior_buffer_cells.insert(buffer_cells.begin(), buffer_cells.end());
        superior_inverter_cells.insert(inverter_cells.begin(),
                                       inverter_cells.end());
    }
    else
    {
        for (int i = 1; i <= 100; i++)
        {
            float buf_cap = min_buff_cap * i;
            float inv_cap = min_inv_cap * i;
            for (int j = 1; j <= 25; j++)
            {
                float buf_res = min_buff_resistance * j;
                float inv_res = min_inv_resistance * j;
                for (int m = 1; m <= 1; m++)
                {
                    float        buf_slew   = m * min_buff_slew;
                    float        inv_slew   = m * min_inv_slew;
                    LibraryCell *chosen_buf = nullptr, *chosen_inv = nullptr;
                    float        min_delay = sta::INF;
                    for (auto& buf : buffer_cells)
                    {
                        float delay =
                            buf_res * bufferInputCapacitance(buf) +
                            gateDelay(bufferOutputPin(buf), buf_cap, &buf_slew);
                        if (delay < min_delay &&
                            buf_cap <= maxLoad(bufferOutputPin(buf)))
                        {
                            min_delay  = delay;
                            chosen_buf = buf;
                        }
                    }
                    min_delay = sta::INF;
                    for (auto& inv : inverter_cells)
                    {
                        float delay =
                            inv_res * bufferInputCapacitance(inv) +
                            gateDelay(bufferOutputPin(inv), inv_cap, &buf_slew);
                        if (delay < min_delay &&
                            buf_cap <= maxLoad(bufferOutputPin(inv)))
                        {
                            min_delay  = delay;
                            chosen_inv = inv;
                        }
                    }
                    if (chosen_buf)
                    {
                        superior_buffer_cells.insert(chosen_buf);
                    }
                    if (chosen_inv)
                    {
                        superior_inverter_cells.insert(chosen_inv);
                    }
                }
            }
        }
    }
    std::unordered_map<LibraryCell*, float> input_capacitances;
    std::unordered_map<LibraryCell*, float> drive_capacitances;
    std::unordered_map<LibraryCell*, float> intrinsic_delays;
    std::unordered_map<LibraryCell*, float> driver_conductance;
    float max_buff_input_capacitances = -sta::INF;
    float max_buff_drive_capacitance  = -sta::INF;
    float max_buff_intrinsic_delays   = -sta::INF;
    float max_buff_driver_conductance = -sta::INF;
    float max_inv_input_capacitances  = -sta::INF;
    float max_inv_drive_capacitance   = -sta::INF;
    float max_inv_intrinsic_delays    = -sta::INF;
    float max_inv_driver_conductance  = -sta::INF;
    for (auto& buf : superior_buffer_cells)
    {
        auto output_pin         = bufferOutputPin(buf);
        input_capacitances[buf] = bufferInputCapacitance(buf);
        drive_capacitances[buf] = maxLoad(buf);
        intrinsic_delays[buf]   = gateDelay(output_pin, min_buff_cap) -
                                output_pin->driveResistance() * min_buff_cap;
        driver_conductance[buf] = 1.0 / output_pin->driveResistance();
        max_buff_input_capacitances =
            std::max(max_buff_input_capacitances, input_capacitances[buf]);
        max_buff_drive_capacitance =
            std::max(max_buff_drive_capacitance, drive_capacitances[buf]);
        max_buff_intrinsic_delays =
            std::max(max_buff_intrinsic_delays, intrinsic_delays[buf]);
        max_buff_driver_conductance =
            std::max(max_buff_driver_conductance, driver_conductance[buf]);
    }
    for (auto& inv : superior_inverter_cells)
    {
        auto output_pin         = bufferOutputPin(inv);
        input_capacitances[inv] = bufferInputCapacitance(inv);
        drive_capacitances[inv] = maxLoad(inv);
        intrinsic_delays[inv]   = gateDelay(output_pin, min_buff_cap) -
                                output_pin->driveResistance() * min_buff_cap;
        driver_conductance[inv] = 1.0 / output_pin->driveResistance();
        max_inv_input_capacitances =
            std::max(max_inv_input_capacitances, input_capacitances[inv]);
        max_inv_drive_capacitance =
            std::max(max_inv_drive_capacitance, drive_capacitances[inv]);
        max_inv_intrinsic_delays =
            std::max(max_inv_intrinsic_delays, intrinsic_delays[inv]);
        max_inv_driver_conductance =
            std::max(max_inv_driver_conductance, driver_conductance[inv]);
    }

    auto buff_distances = [&](LibraryCell* first,
                              LibraryCell* second) -> float {
        return std::sqrt(
            std::pow((input_capacitances[first] - input_capacitances[second]) /
                         max_buff_input_capacitances,
                     2) +
            std::pow((drive_capacitances[first] - drive_capacitances[second]) /
                         max_buff_drive_capacitance,
                     2) +
            std::pow((intrinsic_delays[first] - intrinsic_delays[second]) /
                         max_buff_intrinsic_delays,
                     2) +
            std::pow((driver_conductance[first] - driver_conductance[second]) /
                         max_buff_driver_conductance,
                     2)

        );
    };
    auto inv_distances = [&](LibraryCell* first, LibraryCell* second) -> float {
        return std::sqrt(
            std::pow((input_capacitances[first] - input_capacitances[second]) /
                         max_inv_input_capacitances,
                     2) +
            std::pow((drive_capacitances[first] - drive_capacitances[second]) /
                         max_inv_drive_capacitance,
                     2) +
            std::pow((intrinsic_delays[first] - intrinsic_delays[second]) /
                         max_inv_intrinsic_delays,
                     2) +
            std::pow((driver_conductance[first] - driver_conductance[second]) /
                         max_inv_driver_conductance,
                     2)

        );
    };

    auto buff_vector = std::vector<LibraryCell*>(superior_buffer_cells.begin(),
                                                 superior_buffer_cells.end());
    auto inv_vector = std::vector<LibraryCell*>(superior_inverter_cells.begin(),
                                                superior_inverter_cells.end());

    // TO-DO: Add K-Center clustering here

    return std::pair<std::vector<LibraryCell*>, std::vector<LibraryCell*>>(
        std::vector<LibraryCell*>(), std::vector<LibraryCell*>());
}
std::vector<LibraryCell*>
OpenStaHandler::equivalentCells(LibraryCell* cell)
{
    if (!libraryInputPins(cell).size())
    {
        return std::vector<LibraryCell*>({cell});
    }
    if (!has_equiv_cells_)
    {
        makeEquivalentCells();
    }
    auto equiv_cells = sta_->equivCells(cell);
    if (!equiv_cells)
    {
        return std::vector<LibraryCell*>({cell});
    }
    std::vector<LibraryCell*> filtered_cells;
    for (auto& c : *equiv_cells)
    {
        if (!dontUse(cell) || c == cell)
        {
            filtered_cells.push_back(c);
        }
    }
    return filtered_cells;
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

            if (isSingleOutputCombinational(cell))
            {
                auto           output_pin  = output_pins[0];
                sta::FuncExpr* output_func = output_pin->function();
                if (output_func && output_func->op() == sta::FuncExpr::op_zero)
                {
                    cells.push_back(cell);
                }
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
OpenStaHandler::slew(InstanceTerm* term) const
{
    return std::max(slew(term, true), slew(term, false));
}
float
OpenStaHandler::slew(InstanceTerm* term, bool is_rise) const
{
    if (network()->direction(term)->isInput())
    {
        return sta_->vertexSlew(vertex(term),
                                is_rise ? sta::RiseFall::rise()
                                        : sta::RiseFall::fall(),
                                sta::MinMax::max());
    }

    auto pin_net = net(term);

    auto net_pins = pins(pin_net);

    for (auto connected_pin : net_pins)
    {
        if (connected_pin != term)
        {
            return sta_->vertexSlew(vertex(connected_pin),
                                    is_rise ? sta::RiseFall::rise()
                                            : sta::RiseFall::fall(),
                                    sta::MinMax::max());
        }
    }
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
    auto vert = network()->graph()->pinLoadVertex(term);
    auto req  = sta_->vertexRequired(vert, min_max_);
    //  worst ? sta::MinMax::min() : sta::MinMax::max());
    if (sta::fuzzyInf(req))
    {
        return 0;
    }
    return req;
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
    sta_->ensureGraph();
    sta_->ensureLevelized();

    auto handler_network = network();

    std::vector<InstanceTerm*> terms;
    std::vector<sta::Vertex*>  vertices;
    sta::VertexIterator        itr(handler_network->graph());
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

Port*
OpenStaHandler::topPort(InstanceTerm* term) const
{
    return network()->port(term);
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
OpenStaHandler::dbuToMeters(int dist) const
{
    return dist * 1E-9;
}
double
OpenStaHandler::dbuToMicrons(int dist) const
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
    float cap1 = term->capacitance(sta::RiseFall::rise(), sta::MinMax::max());
    float cap2 = term->capacitance(sta::RiseFall::fall(), sta::MinMax::max());
    return std::max(cap1, cap2);
}
float
OpenStaHandler::pinSlewLimit(InstanceTerm* term, bool* exists) const
{
    float limit;
    bool  limit_exists;
    slewLimit(term, sta::MinMax::max(), limit, limit_exists);
    if (exists)
    {
        *exists = limit_exists;
    }
    return limit;
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
                 arc->toTrans()->asRiseFall() == sta::RiseFall::rise()) ||
                (!is_rise &&
                 arc->toTrans()->asRiseFall() == sta::RiseFall::fall()))
            {
                sta::GateTableModel* model =
                    dynamic_cast<sta::GateTableModel*>(arc->model());
                auto delay_slew_model =
                    is_delay ? model->delayModel() : model->slewModel();
                if (model)
                {
                    return delay_slew_model->findValue(
                        lib_cell->libertyLibrary(), lib_cell, pvt_, slew, cap,
                        0);
                }
            }
        }
    }

    return sta::INF;
}

// This function assumes the input slew to be the target input slew
float
OpenStaHandler::slew(LibraryTerm* term, float load_cap, float* tr_slew)
{
    if (!has_target_loads_)
    {
        findTargetLoads();
    }

    auto cell = term->libertyCell();
    // Max rise/fall delays.
    sta::Slew                            max_slew = -sta::INF;
    sta::LibertyCellTimingArcSetIterator set_iter(cell);
    while (set_iter.hasNext())
    {
        sta::TimingArcSet* arc_set = set_iter.next();
        if (arc_set->to() == term)
        {
            sta::TimingArcSetArcIterator arc_iter(arc_set);
            while (arc_iter.hasNext())
            {
                sta::TimingArc* arc   = arc_iter.next();
                sta::RiseFall*  in_rf = arc->fromTrans()->asRiseFall();
                float           in_slew =
                    tr_slew ? *tr_slew : target_slews_[in_rf->index()];
                sta::ArcDelay gate_delay;
                sta::Slew     drvr_slew;
                sta_->arcDelayCalc()->gateDelay(cell, arc, in_slew, load_cap,
                                                nullptr, 0.0, pvt_, dcalc_ap_,
                                                gate_delay, drvr_slew);
                max_slew = std::max(max_slew, drvr_slew);
            }
        }
    }
    return max_slew;
}
float
OpenStaHandler::bufferFixedInputSlew(LibraryCell* buffer_cell, float cap)
{
    auto output_pin      = bufferOutputPin(buffer_cell);
    auto min_buff_cap    = bufferInputCapacitance(smallestBufferCell());
    auto intrinsic_delay = gateDelay(output_pin, min_buff_cap) -
                           output_pin->driveResistance() * min_buff_cap;
    auto res = output_pin->driveResistance();
    return res * cap + intrinsic_delay;
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
                 arc->toTrans()->asRiseFall() == sta::RiseFall::rise()) ||
                (!is_rise &&
                 arc->toTrans()->asRiseFall() == sta::RiseFall::fall()))
            {
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

void
OpenStaHandler::setLegalizer(std::function<bool(float)> legalizer)
{
    legalizer_ = legalizer;
}
bool
OpenStaHandler::legalize(float max_displacement)
{
    if (legalizer_)
    {
        return legalizer_(max_displacement);
    }
    return false;
}
bool
OpenStaHandler::isTopLevel(InstanceTerm* term) const
{
    return network()->isTopLevelPort(term);
}
void
OpenStaHandler::del(Net* net) const
{
    sta_->deleteNet(net);
}
void
OpenStaHandler::del(Instance* inst) const
{
    sta_->deleteInstance(inst);
}
int
OpenStaHandler::disconnectAll(Net* net) const
{
    int count = 0;
    for (auto& pin : pins(net))
    {
        sta_->disconnectPin(pin);
        count++;
    }

    return count;
}

void
OpenStaHandler::connect(Net* net, InstanceTerm* term) const
{
    auto inst      = network()->instance(term);
    auto term_port = network()->port(term);
    sta_->connectPin(inst, term_port, net);
}

void
OpenStaHandler::disconnect(InstanceTerm* term) const
{
    sta_->disconnectPin(term);
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
    if (hasWireRC())
    {
        calculateParasitics(first_net);
        calculateParasitics(second_net);
    }
    resetDelays(first);
    resetDelays(second);
}
Instance*
OpenStaHandler::createInstance(const char* inst_name, LibraryCell* cell)
{
    return network()->makeInstance(cell, inst_name, network()->topInstance());
}

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
    auto net = network()->makeNet(net_name, network()->topInstance());
    if (net && hasWireRC())
    {
        calculateParasitics(net);
    }
    return net;
}
float
OpenStaHandler::resistance(LibraryTerm* term) const
{
    return term->driveResistance();
}
float
OpenStaHandler::resistancePerMicron() const
{
    if (!has_wire_rc_)
    {
        PSN_LOG_WARN("Wire RC is not set or invalid");
    }
    return res_per_micron_;
}
float
OpenStaHandler::capacitancePerMicron() const
{
    if (!has_wire_rc_)
    {
        PSN_LOG_WARN("Wire RC is not set or invalid");
    }
    return cap_per_micron_;
}

void
OpenStaHandler::connect(Net* net, Instance* inst, LibraryTerm* port) const
{
    sta_->connectPin(inst, port, net);
}
void
OpenStaHandler::connect(Net* net, Instance* inst, Port* port) const
{
    sta_->connectPin(inst, port, net);
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
void
OpenStaHandler::setDontUse(std::vector<std::string>& cell_names)
{
    for (auto& name : cell_names)
    {
        auto cell = libraryCell(name.c_str());
        if (!cell)
        {
            PSN_LOG_WARN("Cannot find cell with the name", name);
        }
        else
        {
            dont_use_.insert(cell);
        }
    }
}

std::string
OpenStaHandler::topName() const
{
    return std::string(network()->name(network()->topInstance()));
}
std::string
OpenStaHandler::generateNetName(int& start_index)
{
    std::string name;
    do
        name = std::string("net_") + std::to_string(start_index++);
    while (net(name.c_str()));
    return name;
}
std::string
OpenStaHandler::generateInstanceName(const std::string& prefix,
                                     int&               start_index)
{
    std::string name;
    do
        name = prefix + std::to_string(start_index++);
    while (instance(name.c_str()));
    return name;
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
    auto libs = db_->getLibs();

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

bool
OpenStaHandler::hasLiberty() const
{
    sta::LibertyLibraryIterator* iter = network()->libertyLibraryIterator();
    return iter->hasNext();
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
OpenStaHandler::isBiDirect(InstanceTerm* term) const
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
void
OpenStaHandler::replaceInstance(Instance* inst, LibraryCell* cell)
{
    auto current_name = name(cell);
    auto db_lib_cell  = db_->findMaster(current_name.c_str());
    if (db_lib_cell)
    {
        auto db_inst     = network()->staToDb(inst);
        auto db_inst_lib = db_inst->getMaster();
        auto sta_cell    = network()->dbToSta(db_lib_cell);
        sta_->replaceCell(inst, sta_cell);
    }
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

void
OpenStaHandler::setWireRC(float res_per_micon, float cap_per_micron)
{
    sta_->ensureGraph();
    sta_->ensureLevelized();
    sta_->graphDelayCalc()->delaysInvalid();
    sta_->search()->arrivalsInvalid();

    res_per_micron_ = res_per_micon;
    cap_per_micron_ = cap_per_micron;
    has_wire_rc_    = true;
    calculateParasitics();
    sta_->findDelays();
}

bool
OpenStaHandler::hasWireRC()
{
    return has_wire_rc_;
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
OpenStaHandler::isBiDirect(LibraryTerm* term) const
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
    sta::Vertex *vert, *bi;
    sta_->graph()->pinVertices(term, vert, bi);
    float limit;
    bool  exists;
    slewLimit(term, sta::MinMax::max(), limit, exists);
    bool vio = false;
    for (auto rf : sta::RiseFall::range())
    {
        auto pin_slew = sta_->graph()->slew(vert, rf, dcalc_ap_->index());
        if (pin_slew > limit)
        {
            return true;
        }

        if (bi)
        {
            pin_slew = sta_->graph()->slew(bi, rf, dcalc_ap_->index());
            if (pin_slew > limit)
            {
                return true;
            }
        }
    }
    return vio;
}

std::vector<InstanceTerm*>
OpenStaHandler::maximumTransitionViolations() const
{
    if (sta_->sdc()->haveClkSlewLimits())
    {
        sta_->updateTiming(false);
    }
    else
    {
        sta_->findDelays();
    }
    std::unordered_set<InstanceTerm*> vio_pins;
    auto                              clock_nets = clockNets();
    for (auto& pin : levelDriverPins())
    {
        auto pin_net = net(pin);
        if (pin_net && !clock_nets.count(pin_net))
        {

            auto net_pins = pins(pin_net);
            for (auto connected_pin : net_pins)
            {
                if (violatesMaximumTransition(connected_pin))
                {
                    vio_pins.insert(pin);
                    break;
                }
            }
        }
    }
    return std::vector<InstanceTerm*>(vio_pins.begin(), vio_pins.end());
}
std::vector<InstanceTerm*>
OpenStaHandler::maximumCapacitanceViolations() const
{
    if (sta_->sdc()->haveClkSlewLimits())
    {
        sta_->updateTiming(false);
    }
    else
    {
        sta_->findDelays();
    }
    std::unordered_set<InstanceTerm*> vio_pins;
    auto                              clock_nets = clockNets();
    for (auto& pin : levelDriverPins())
    {
        auto pin_net = net(pin);
        if (pin_net && !clock_nets.count(pin_net))
        {

            auto net_pins = pins(pin_net);
            for (auto connected_pin : net_pins)
            {
                if (violatesMaximumCapacitance(connected_pin))
                {
                    vio_pins.insert(pin);
                    break;
                }
            }
        }
    }
    return std::vector<InstanceTerm*>(vio_pins.begin(), vio_pins.end());
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
    auto                   all_libs = allLibs();
    sta::LibertyLibrarySeq lib_seq(all_libs.size());
    for (size_t i = 0; i < all_libs.size(); i++)
    {
        lib_seq[i] = all_libs[i];
    }
    sta_->makeEquivCells(&lib_seq, &map_libs);
    has_equiv_cells_ = true;
}

std::vector<Liberty*>
OpenStaHandler::allLibs() const
{
    std::vector<Liberty*>        seq;
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
    has_equiv_cells_         = false;
    has_buffer_inverter_seq_ = false;
    has_target_loads_        = false;
    maximum_area_valid_      = false;
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
}

float
OpenStaHandler::bufferChainDelayPenalty(float load_cap)
{
    if (!has_buffer_inverter_seq_)
    {
        computeBuffersDelayPenalty();
    }

    if (!buffer_inverter_seq_.size())
    {
        return 0.0;
    }

    if (!penalty_cache_.count(load_cap))
    {
        auto smallest_buff = buffer_inverter_seq_[0];
        if (bufferInputCapacitance(smallest_buff) >= load_cap)
        {
            penalty_cache_[load_cap] = 0.0;
            return 0.0;
        }
        else
        {
            float        min_penalty = sta::INF;
            LibraryCell* c;
            for (auto& buf : buffer_inverter_seq_)
            {
                bool  is_inverting = inverting_buffer_.count(buf) > 0;
                float d_penalty    = is_inverting
                                      ? inverting_buffer_penalty_map_[buf]
                                      : buffer_penalty_map_[buf];
                auto  out_pin = bufferOutputPin(buf);
                float delay   = gateDelay(out_pin, load_cap);
                float penalty = delay + d_penalty;
                if (penalty < min_penalty)
                {
                    min_penalty = penalty;
                    c           = buf;
                }
            }
            penalty_cache_[load_cap] = min_penalty;
            return min_penalty;
        }
    }
    return penalty_cache_[load_cap];
}

void
OpenStaHandler::computeBuffersDelayPenalty(bool include_inverting)
{
    // TODO Support include buffer slews
    auto all_libs = allLibs();
    buffer_inverter_seq_.clear();
    inverting_buffer_.clear();
    non_inverting_buffer_.clear();

    for (auto& lib : all_libs)
    {
        auto buff_types = *lib->buffers();
        for (auto& b :
             std::vector<LibraryCell*>(buff_types.begin(), buff_types.end()))
        {
            if (!dontUse(b))
            {
                non_inverting_buffer_.insert(b);
                buffer_inverter_seq_.push_back(b);
            }
        }
    }
    if (include_inverting)
    {
        auto all_inverters = inverterCells();
        for (auto& inv : all_inverters)
        {
            if (!dontUse(inv))
            {
                inverting_buffer_.insert(inv);
                buffer_inverter_seq_.push_back(inv);
            }
        }
    }

    std::sort(buffer_inverter_seq_.begin(), buffer_inverter_seq_.end(),
              [=](LibraryCell* b1, LibraryCell* b2) -> bool {
                  return bufferInputCapacitance(b1) <
                         bufferInputCapacitance(b2);
              });
    has_buffer_inverter_seq_ = true;
    buffer_penalty_map_.clear();
    inverting_buffer_penalty_map_.clear();
    penalty_cache_.clear();
    if (!buffer_inverter_seq_.size())
    {
        return;
    }
    auto first_cell = buffer_inverter_seq_[0];
    buffer_penalty_map_[first_cell] =
        inverting_buffer_.count(first_cell) ? sta::INF : 0;
    inverting_buffer_penalty_map_[first_cell] =
        inverting_buffer_.count(first_cell) ? 0 : sta::INF;

    for (size_t i = 1; i < buffer_inverter_seq_.size(); i++)
    {
        float        min_penalty = sta::INF;
        LibraryCell* best_chain  = nullptr;
        bool         is_sink_inverting =
            inverting_buffer_.count(buffer_inverter_seq_[i]) > 0;
        for (size_t j = 0; j < i; j++)
        {
            bool is_inverting =
                inverting_buffer_.count(buffer_inverter_seq_[j]) > 0;
            auto  out_pin = bufferOutputPin(buffer_inverter_seq_[j]);
            float delay   = gateDelay(
                out_pin, bufferInputCapacitance(buffer_inverter_seq_[i]));

            float d_penalty =
                is_inverting
                    ? inverting_buffer_penalty_map_[buffer_inverter_seq_[j]]
                    : buffer_penalty_map_[buffer_inverter_seq_[j]];

            float penalty = delay + d_penalty;
            if (penalty < min_penalty)
            {
                min_penalty = penalty;
                best_chain  = buffer_inverter_seq_[j];
            }
        }
        if (is_sink_inverting)
        {
            inverting_buffer_penalty_map_[buffer_inverter_seq_[i]] =
                min_penalty;
        }
        else
        {
            buffer_penalty_map_[buffer_inverter_seq_[i]] = min_penalty;
        }
    }
}
InstanceTerm*
OpenStaHandler::largestLoadCapacitancePin(Instance* cell)
{
    float         max_cap = -sta::INF;
    InstanceTerm* max_pin = nullptr;
    for (auto& pin : inputPins(cell))
    {
        auto cap = loadCapacitance(pin);
        if (cap > max_cap)
        {
            max_cap = cap;
            max_pin = pin;
        }
    }
    return max_pin;
}
LibraryTerm*
OpenStaHandler::largestInputCapacitanceLibraryPin(Instance* cell)
{
    float        max_cap = -sta::INF;
    LibraryTerm* max_pin = nullptr;
    for (auto& pin : inputPins(cell))
    {
        auto cap = loadCapacitance(pin);
        if (cap > max_cap)
        {
            max_cap = cap;
            max_pin = libraryPin(pin);
        }
    }
    return max_pin;
}
float
OpenStaHandler::largestInputCapacitance(Instance* cell)
{
    return largestInputCapacitance(libraryCell(cell));
}
float
OpenStaHandler::largestInputCapacitance(LibraryCell* cell)
{
    float max_cap    = -sta::INF;
    auto  input_pins = libraryInputPins(cell);
    if (!input_pins.size())
    {
        return 0.0;
    }
    for (auto& pin : input_pins)
    {
        float cap = pinCapacitance(pin);
        if (cap > max_cap)
        {
            max_cap = cap;
        }
    }
    return max_cap;
}

/* The following is borrowed from James Cherry's Resizer Code */

// Find a target slew for the libraries and then
// a target load for each cell that gives the target slew.
void
OpenStaHandler::findTargetLoads(std::vector<Liberty*>* resize_libs)
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
OpenStaHandler::coreArea() const
{
    auto block = top();
    Rect core;
    block->getCoreArea(core);
    return dbuToMeters(core.dx()) * dbuToMeters(core.dy());
}

bool
OpenStaHandler::maximumUtilizationViolation() const
{
    return sta::fuzzyGreaterEqual(area(), maximumArea());
}
void
OpenStaHandler::setMaximumArea(float area)
{
    maximum_area_       = area;
    maximum_area_valid_ = true;
}
bool
OpenStaHandler::hasMaximumArea() const
{
    return maximum_area_valid_;
}
float
OpenStaHandler::maximumArea() const
{
    if (!maximum_area_valid_)
    {
        PSN_LOG_WARN("Maximum area is not set or invalid");
    }
    return maximum_area_;
}

float
OpenStaHandler::gateDelay(Instance* inst, InstanceTerm* to, float in_slew,
                          LibraryTerm* from, float* drvr_slew, int rise_fall)
{
    return gateDelay(libraryCell(inst), to, in_slew, from, drvr_slew,
                     rise_fall);
}

float
OpenStaHandler::gateDelay(LibraryCell* lib_cell, InstanceTerm* to,
                          float in_slew, LibraryTerm* from, float* drvr_slew,
                          int rise_fall)
{
    sta::ArcDelay                        max = -sta::INF;
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
                if (!from || (from && arc->from() == from))
                {
                    if (rise_fall != -1 ||
                        (rise_fall == 1 && arc->toTrans()->asRiseFall() ==
                                               sta::RiseFall::rise()) ||
                        (rise_fall == 0 &&
                         arc->toTrans()->asRiseFall() == sta::RiseFall::fall()))
                    {
                        // 1 is rising edge
                        // 0 is falling edge
                        float         load_cap = loadCapacitance(to);
                        sta::ArcDelay gate_delay;
                        sta::Slew     tmp_slew;
                        sta::Slew*    slew = drvr_slew ? drvr_slew : &tmp_slew;
                        sta_->arcDelayCalc()->gateDelay(
                            lib_cell, arc, in_slew, load_cap, nullptr, 0.0,
                            pvt_, dcalc_ap_, gate_delay, *slew);
                        max = std::max(max, gate_delay);
                    }
                }
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
std::set<Net*>
OpenStaHandler::clockNets() const
{
    std::set<Net*>            nets;
    sta::ClkArrivalSearchPred srch_pred(sta_);
    sta::BfsFwdIterator       bfs(sta::BfsIndex::other, &srch_pred, sta_);
    sta::PinSet               clk_pins;
    sta_->search()->findClkVertexPins(clk_pins);
    for (auto pin : clk_pins)
    {
        sta::Vertex *vert, *bi_vert;
        network()->graph()->pinVertices(pin, vert, bi_vert);
        bfs.enqueue(vert);
        if (bi_vert)
            bfs.enqueue(bi_vert);
    }
    while (bfs.hasNext())
    {
        auto vertex = bfs.next();
        auto pin    = vertex->pin();
        Net* net    = network()->net(pin);
        nets.insert(net);
        bfs.enqueueAdjacentVertices(vertex);
    }
    return nets;
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
    sta_->sdc()->slewLimit(top_cell, min_max, top_limit, top_limit_exists);

    // Default to top ("design") limit.
    exists = top_limit_exists;
    limit  = top_limit;
    if (network()->isTopLevelPort(pin))
    {
        auto  port = network()->port(pin);
        float port_limit;
        bool  port_limit_exists;
        sta_->sdc()->slewLimit(port, min_max, port_limit, port_limit_exists);
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
OpenStaHandler::gateDelay(InstanceTerm* pin, float load_cap, float* tr_slew)
{
    return gateDelay(libraryPin(pin), load_cap, tr_slew);
}

float
OpenStaHandler::gateDelay(LibraryTerm* out_port, float load_cap, float* tr_slew)
{
    if (!has_target_loads_)
    {
        findTargetLoads();
    }
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
                sta::TimingArc* arc   = arc_iter.next();
                sta::RiseFall*  in_rf = arc->fromTrans()->asRiseFall();
                float           in_slew =
                    tr_slew ? *tr_slew : target_slews_[in_rf->index()];
                sta::ArcDelay gate_delay;
                sta::Slew     drvr_slew;
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
OpenStaHandler::portCapacitance(const LibraryTerm* port, bool isMax) const
{
    float cap1 = port->capacitance(
        sta::RiseFall::rise(), isMax ? sta::MinMax::max() : sta::MinMax::min());
    float cap2 = port->capacitance(
        sta::RiseFall::fall(), isMax ? sta::MinMax::max() : sta::MinMax::min());
    return std::max(cap1, cap2);
}

LibraryTerm*
OpenStaHandler::bufferInputPin(LibraryCell* buffer_cell) const
{
    LibraryTerm *input, *output;
    buffer_cell->bufferPorts(input, output);
    return input;
}
LibraryTerm*
OpenStaHandler::bufferOutputPin(LibraryCell* buffer_cell) const
{
    LibraryTerm *input, *output;
    buffer_cell->bufferPorts(input, output);
    return output;
}

float
OpenStaHandler::inverterInputCapacitance(LibraryCell* inv_cell)
{
    LibraryTerm *input, *output;
    inv_cell->bufferPorts(input, output);
    return portCapacitance(input);
}
float
OpenStaHandler::bufferInputCapacitance(LibraryCell* buffer_cell) const
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
OpenStaHandler::findBufferTargetSlews(std::vector<Liberty*>* resize_libs)
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
    {
        target_slews_[rf] /= tgt_counts[rf];
    }
}
bool
OpenStaHandler::dontUse(LibraryCell* cell) const
{
    return cell->dontUse() || dont_use_.count(cell);
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
void
OpenStaHandler::calculateParasitics()
{
    for (auto& net : nets())
    {
        if (!isClock(net))
        {
            calculateParasitics(net);
        }
    }
}
bool
OpenStaHandler::isClock(Net* net) const
{

    auto net_pin = faninPin(net);
    if (net_pin)
    {
        auto vert = vertex(net_pin);
        if (vert)
        {

            if (network()->search()->isClock(vert))
            {
                return true;
            }
        }
    }

    return false;
}

void
OpenStaHandler::calculateParasitics(Net* net)
{
    auto tree = SteinerTree::create(net, psn_);
    if (tree && tree->isPlaced())
    {
        sta::Parasitic* parasitic = sta_->parasitics()->makeParasiticNetwork(
            net, false, parasitics_ap_);
        int branch_count = tree->branchCount();
        for (int i = 0; i < branch_count; i++)
        {
            auto                branch = tree->branch(i);
            sta::ParasiticNode* n1 =
                findParasiticNode(tree, parasitic, net, branch.firstPin(),
                                  branch.firstSteinerPoint());
            sta::ParasiticNode* n2 =
                findParasiticNode(tree, parasitic, net, branch.secondPin(),
                                  branch.secondSteinerPoint());
            if (n1 != n2)
            {
                if (branch.wireLength() == 0)
                {
                    sta_->parasitics()->makeResistor(nullptr, n1, n2, 1.0e-3,
                                                     parasitics_ap_);
                }
                else
                {
                    float wire_length = dbuToMeters(branch.wireLength());
                    float wire_cap    = wire_length * cap_per_micron_;
                    float wire_res    = wire_length * res_per_micron_;
                    sta_->parasitics()->incrCap(n1, wire_cap / 2.0,
                                                parasitics_ap_);
                    sta_->parasitics()->makeResistor(nullptr, n1, n2, wire_res,
                                                     parasitics_ap_);
                    sta_->parasitics()->incrCap(n2, wire_cap / 2.0,
                                                parasitics_ap_);
                }
            }
        }
    }
}
sta::ParasiticNode*
OpenStaHandler::findParasiticNode(std::unique_ptr<SteinerTree>& tree,
                                  sta::Parasitic* parasitic, const Net* net,
                                  const InstanceTerm* pin, SteinerPoint pt)
{
    if (pin == nullptr)
    {
        pin = tree->alias(pt);
    }
    if (pin)
    {
        return sta_->parasitics()->ensureParasiticNode(parasitic, pin);
    }
    else
    {
        return sta_->parasitics()->ensureParasiticNode(parasitic, net, pt);
    }
}
HandlerType
OpenStaHandler::handlerType() const
{
    return HandlerType::OPENSTA;
}
} // namespace psn
