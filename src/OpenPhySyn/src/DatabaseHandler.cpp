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
#include "OpenPhySyn/DatabaseHandler.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include "OpenPhySyn/ClusteringUtils.hpp"
#include "OpenPhySyn/LibraryMapping.hpp"
#include "OpenPhySyn/PsnGlobal.hpp"
#include "OpenPhySyn/PsnLogger.hpp"
#include "OpenPhySyn/SteinerTree.hpp"
#include "OpenPhySyn/Types.hpp"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "opendb/geom.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/Bfs.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/EquivCells.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
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
#include "sta/Units.hh"

namespace psn
{
DatabaseHandler::DatabaseHandler(Psn* psn_inst, DatabaseSta* sta)
    : sta_(sta),
      db_(sta->db()),
      has_equiv_cells_(false),
      has_buffer_inverter_seq_(false),
      has_target_loads_(false),
      psn_(psn_inst),
      has_wire_rc_(false),
      maximum_area_valid_(false),
      has_library_cell_mappings_(false)
{
    // Use default corner for now
    corner_                      = sta_->findCorner("default");
    min_max_                     = sta::MinMax::max();
    dcalc_ap_                    = corner_->findDcalcAnalysisPt(min_max_);
    pvt_                         = dcalc_ap_->operatingConditions();
    parasitics_ap_               = corner_->findParasiticAnalysisPt(min_max_);
    legalizer_                   = nullptr;
    res_per_micron_callback_     = nullptr;
    cap_per_micron_callback_     = nullptr;
    dont_use_callback_           = nullptr;
    compute_parasitics_callback_ = nullptr;
    maximum_area_callback_       = nullptr;
    update_design_area_callback_ = nullptr;
    resetDelays();
}

std::vector<InstanceTerm*>
DatabaseHandler::pins(Net* net) const
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
DatabaseHandler::pins(Instance* inst) const
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
DatabaseHandler::net(InstanceTerm* term) const
{
    return network()->net(term);
}
Term*
DatabaseHandler::term(InstanceTerm* term) const
{
    return network()->term(term);
}
Net*
DatabaseHandler::net(Term* term) const
{
    return network()->net(term);
}
std::vector<InstanceTerm*>
DatabaseHandler::connectedPins(Net* net) const
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
DatabaseHandler::bufferNet(Net* b_net, LibraryCell* buffer,
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
DatabaseHandler::clockPins() const
{
    std::set<InstanceTerm*> clock_pins;
    auto                    clk_iter = new sta::ClockIterator(sta_->sdc());
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
DatabaseHandler::inputPins(Instance* inst, bool include_top_level) const
{
    auto inst_pins = pins(inst);
    return filterPins(inst_pins, PinDirection::input(), include_top_level);
}

std::vector<InstanceTerm*>
DatabaseHandler::outputPins(Instance* inst, bool include_top_level) const
{
    auto inst_pins = pins(inst);
    return filterPins(inst_pins, PinDirection::output(), include_top_level);
}

std::vector<InstanceTerm*>
DatabaseHandler::fanoutPins(Net* pin_net, bool include_top_level) const
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

bool
DatabaseHandler::isTieHi(Instance* inst) const
{
    return isTieHi(libraryCell(inst));
}
bool
DatabaseHandler::isTieHi(LibraryCell* cell) const
{
    if (isSingleOutputCombinational(cell))
    {
        auto           output_pins = libraryOutputPins(cell);
        auto           output_pin  = output_pins[0];
        sta::FuncExpr* output_func = output_pin->function();
        if (output_func && output_func->op() == sta::FuncExpr::op_one)
        {
            return true;
        }
    }
    return false;
}
bool
DatabaseHandler::isTieLo(Instance* inst) const
{
    return isTieHiLo(libraryCell(inst));
}
bool
DatabaseHandler::isTieLo(LibraryCell* cell) const
{
    if (isSingleOutputCombinational(cell))
    {
        auto           output_pins = libraryOutputPins(cell);
        auto           output_pin  = output_pins[0];
        sta::FuncExpr* output_func = output_pin->function();
        if (output_func && output_func->op() == sta::FuncExpr::op_zero)
        {
            return true;
        }
    }
    return false;
}
bool
DatabaseHandler::isTieHiLo(Instance* inst) const
{
    return isTieHiLo(libraryCell(inst));
}
bool
DatabaseHandler::isTieHiLo(LibraryCell* cell) const
{
    return isTieHi(cell) || isTieLo(cell);
}
bool
DatabaseHandler::isTieCell(Instance* inst) const
{
    return isTieCell(libraryCell(inst));
}
bool
DatabaseHandler::isTieCell(LibraryCell* cell) const
{
    return isTieCell(cell);
}

std::vector<LibraryCell*>
DatabaseHandler::tiehiCells() const
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
DatabaseHandler::inverterCells() const
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

            if (!dontUse(cell) && isSingleOutputCombinational(cell) &&
                input_pins.size() == 1)
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
DatabaseHandler::smallestInverterCell() const
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
DatabaseHandler::bufferCells() const
{
    std::vector<LibraryCell*> cells;
    auto                      all_libs = allLibs();
    for (auto& lib : all_libs)
    {
        auto buff_libs = lib->buffers();
        for (auto& cell : *buff_libs)
        {
            if (!dontUse(cell))
            {
                cells.push_back(cell);
            }
        }
    }
    return cells;
}

void
DatabaseHandler::populatePrimitiveCellCache()
{

    std::unordered_map<int, std::string> and_truth;
    std::unordered_map<int, std::string> nand_truth;
    std::unordered_map<int, std::string> or_truth;
    std::unordered_map<int, std::string> nor_truth;
    std::unordered_map<int, std::string> xor_truth;
    std::unordered_map<int, std::string> xnor_truth;

    for (int i = 2; i <= 16; i++)
    {
        std::string and_fn;
        std::string nand_fn;
        std::string or_fn;
        std::string nor_fn;
        std::string xor_fn;
        std::string xnor_fn;
        for (int j = 0; j < std::pow(2, i); j++)
        {
            std::bitset<64> inp(j);
            bool            and_result = inp.test(0);
            bool            or_result  = inp.test(0);
            bool            xor_result = inp.test(0);
            for (int m = 1; m < i; m++)
            {
                and_result &= inp.test(m);
                or_result |= inp.test(m);
                xor_result ^= inp.test(m);
            }
            bool nor_result  = !or_result;
            bool nand_result = !and_result;
            bool xnor_result = !xor_result;
            and_fn           = std::to_string((int)and_result) + and_fn;
            or_fn            = std::to_string((int)or_result) + or_fn;
            nand_fn          = std::to_string((int)nand_result) + nand_fn;
            nor_fn           = std::to_string((int)nor_result) + nor_fn;
            xor_fn           = std::to_string((int)xor_result) + xor_fn;
            xnor_fn          = std::to_string((int)xnor_result) + xnor_fn;
        }
        for (int j = and_fn.size(); j < 64; j++)
        {
            and_fn  = "0" + and_fn;
            or_fn   = "0" + or_fn;
            nand_fn = "0" + nand_fn;
            nor_fn  = "0" + nor_fn;
            xor_fn  = "0" + xor_fn;
            xnor_fn = "0" + xnor_fn;
        }
        and_truth[i]  = and_fn;
        nand_truth[i] = nand_fn;
        or_truth[i]   = or_fn;
        nor_truth[i]  = nor_fn;
        xor_truth[i]  = xor_fn;
        xnor_truth[i] = xnor_fn;
    }

    for (auto& lib : allLibs())
    {
        sta::LibertyCellIterator cell_iter(lib);
        while (cell_iter.hasNext())
        {
            auto lib_cell = cell_iter.next();
            if (nand_cells_.count(lib_cell->libertyCell()) ||
                and_cells_.count(lib_cell->libertyCell()) ||
                nor_cells_.count(lib_cell->libertyCell()) ||
                or_cells_.count(lib_cell->libertyCell()) ||
                xor_cells_.count(lib_cell->libertyCell()))
            {
                continue;
            }

            auto input_pins = libraryInputPins(lib_cell);
            if (isSingleOutputCombinational(lib_cell) && input_pins.size() < 32)
            {
                auto           output_pins = libraryOutputPins(lib_cell);
                auto           input_pins  = libraryInputPins(lib_cell);
                auto           output_pin  = output_pins[0];
                sta::FuncExpr* output_func = output_pin->function();
                if (output_func)
                {
                    auto table = std::bitset<64>(computeTruthTable(lib_cell));
                    if (input_pins.size() >= 2 && input_pins.size() <= 16)
                    {
                        if (nand_truth[input_pins.size()] == table.to_string())
                        {
                            nand_cells_.insert(lib_cell);
                            auto equiv_cells = equivalentCells(lib_cell);
                            for (auto& eq : equiv_cells)
                            {
                                nand_cells_.insert(eq);
                            }
                        }
                        else if (and_truth[input_pins.size()] ==
                                 table.to_string())
                        {
                            and_cells_.insert(lib_cell);
                            auto equiv_cells = equivalentCells(lib_cell);
                            for (auto& eq : equiv_cells)
                            {
                                and_cells_.insert(eq);
                            }
                        }
                        else if (or_truth[input_pins.size()] ==
                                 table.to_string())
                        {
                            or_cells_.insert(lib_cell);
                            auto equiv_cells = equivalentCells(lib_cell);
                            for (auto& eq : equiv_cells)
                            {
                                or_cells_.insert(eq);
                            }
                        }
                        else if (nor_truth[input_pins.size()] ==
                                 table.to_string())
                        {
                            nor_cells_.insert(lib_cell);
                            auto equiv_cells = equivalentCells(lib_cell);
                            for (auto& eq : equiv_cells)
                            {
                                nor_cells_.insert(eq);
                            }
                        }
                        else if (xor_truth[input_pins.size()] ==
                                 table.to_string())
                        {
                            xor_cells_.insert(lib_cell);
                            auto equiv_cells = equivalentCells(lib_cell);
                            for (auto& eq : equiv_cells)
                            {
                                xor_cells_.insert(eq);
                            }
                        }
                        else if (xnor_truth[input_pins.size()] ==
                                 table.to_string())
                        {
                            xnor_cells_.insert(lib_cell);
                            auto equiv_cells = equivalentCells(lib_cell);
                            for (auto& eq : equiv_cells)
                            {
                                xnor_cells_.insert(eq);
                            }
                        }
                    }
                }
            }
        }
    }
}

std::vector<LibraryCell*>
DatabaseHandler::nandCells(int in_size)
{
    if (!nand_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (!in_size)
    {
        return std::vector<LibraryCell*>(nand_cells_.begin(),
                                         nand_cells_.end());
    }
    else
    {
        std::vector<LibraryCell*> cells;
        for (auto& cell : nand_cells_)
        {
            if (libraryInputPins(cell).size() == in_size)
            {
                cells.push_back(cell);
            }
        }
        return cells;
    }
}
std::vector<LibraryCell*>
DatabaseHandler::andCells(int in_size)
{
    if (!and_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (!in_size)
    {
        return std::vector<LibraryCell*>(and_cells_.begin(), and_cells_.end());
    }
    else
    {
        std::vector<LibraryCell*> cells;
        for (auto& cell : and_cells_)
        {
            if (libraryInputPins(cell).size() == in_size)
            {
                cells.push_back(cell);
            }
        }
        return cells;
    }
}
std::vector<LibraryCell*>
DatabaseHandler::orCells(int in_size)
{
    if (!or_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (!in_size)
    {
        return std::vector<LibraryCell*>(or_cells_.begin(), or_cells_.end());
    }
    else
    {
        std::vector<LibraryCell*> cells;
        for (auto& cell : or_cells_)
        {
            if (libraryInputPins(cell).size() == in_size)
            {
                cells.push_back(cell);
            }
        }
        return cells;
    }
}
std::vector<LibraryCell*>
DatabaseHandler::norCells(int in_size)
{
    if (!nor_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (!in_size)
    {
        return std::vector<LibraryCell*>(nor_cells_.begin(), nor_cells_.end());
    }
    else
    {
        std::vector<LibraryCell*> cells;
        for (auto& cell : nor_cells_)
        {
            if (libraryInputPins(cell).size() == in_size)
            {
                cells.push_back(cell);
            }
        }
        return cells;
    }
}
std::vector<LibraryCell*>
DatabaseHandler::xorCells(int in_size)
{
    if (!xor_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (!in_size)
    {
        return std::vector<LibraryCell*>(xor_cells_.begin(), xor_cells_.end());
    }
    else
    {
        std::vector<LibraryCell*> cells;
        for (auto& cell : xor_cells_)
        {
            if (libraryInputPins(cell).size() == in_size)
            {
                cells.push_back(cell);
            }
        }
        return cells;
    }
}
std::vector<LibraryCell*>
DatabaseHandler::xnorCells(int in_size)
{
    if (!xnor_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (!in_size)
    {
        return std::vector<LibraryCell*>(xnor_cells_.begin(),
                                         xnor_cells_.end());
    }
    else
    {
        std::vector<LibraryCell*> cells;
        for (auto& cell : xnor_cells_)
        {
            if (libraryInputPins(cell).size() == in_size)
            {
                cells.push_back(cell);
            }
        }
        return cells;
    }
}
int
DatabaseHandler::isAND(LibraryCell* cell)
{
    if (!and_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (and_cells_.count(cell))
    {
        return libraryInputPins(cell).size();
    }
    return 0;
}
int
DatabaseHandler::isNAND(LibraryCell* cell)
{
    if (!nand_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (nand_cells_.count(cell))
    {
        return libraryInputPins(cell).size();
    }
    return 0;
}
int
DatabaseHandler::isOR(LibraryCell* cell)
{
    if (!or_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (or_cells_.count(cell))
    {
        return libraryInputPins(cell).size();
    }
    return 0;
}
int
DatabaseHandler::isNOR(LibraryCell* cell)
{
    if (!nor_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (nor_cells_.count(cell))
    {
        return libraryInputPins(cell).size();
    }
    return 0;
}
int
DatabaseHandler::isXOR(LibraryCell* cell)
{
    if (!xor_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (xor_cells_.count(cell))
    {
        return libraryInputPins(cell).size();
    }
    return 0;
}
int
DatabaseHandler::isXNOR(LibraryCell* cell)
{
    if (!xnor_cells_.size())
    {
        populatePrimitiveCellCache();
    }
    if (xnor_cells_.count(cell))
    {
        return libraryInputPins(cell).size();
    }
    return 0;
}
bool
DatabaseHandler::isXORXNOR(LibraryCell* cell)
{
    return isXOR(cell) || isXNOR(cell);
}
bool
DatabaseHandler::isANDOR(LibraryCell* cell)
{
    return isAND(cell) || isOR(cell);
}
bool
DatabaseHandler::isNANDNOR(LibraryCell* cell)
{
    return isNAND(cell) || isNOR(cell);
}
bool
DatabaseHandler::isAnyANDOR(LibraryCell* cell)
{
    return isANDOR(cell) || isNANDNOR(cell);
}

LibraryCell*
DatabaseHandler::closestDriver(LibraryCell*              cell,
                               std::vector<LibraryCell*> candidates,
                               float                     scale)
{
    if (!candidates.size() || !isSingleOutputCombinational(cell))
    {
        return nullptr;
    }
    auto         output_pin    = libraryOutputPins(cell)[0];
    auto         current_limit = scale * maxLoad(output_pin);
    LibraryCell* closest       = nullptr;
    auto         diff          = sta::INF;
    for (auto& cand : candidates)
    {
        auto limit = maxLoad(libraryOutputPins(cand)[0]);
        if (limit == current_limit)
        {
            return cand;
        }

        auto new_diff = std::fabs(limit - current_limit);
        if (new_diff < diff)
        {
            diff    = new_diff;
            closest = cand;
        }
    }
    return closest;
}
LibraryCell*
DatabaseHandler::halfDrivingPowerCell(Instance* inst)
{
    return halfDrivingPowerCell(libraryCell(inst));
}
LibraryCell*
DatabaseHandler::halfDrivingPowerCell(LibraryCell* cell)
{
    return closestDriver(cell, equivalentCells(cell), 0.5);
}
std::vector<LibraryCell*>
DatabaseHandler::inverseCells(LibraryCell* cell)
{
    if (isAND(cell))
    {
        return nandCells(libraryInputPins(cell).size());
    }
    else if (isOR(cell))
    {
        return norCells(libraryInputPins(cell).size());
    }
    else if (isNOR(cell))
    {
        return orCells(libraryInputPins(cell).size());
    }
    else if (isNAND(cell))
    {
        return andCells(libraryInputPins(cell).size());
    }
    else if (isBuffer(cell))
    {
        return inverterCells();
    }
    else if (isInverter(cell))
    {
        return bufferCells();
    }
    return std::vector<LibraryCell*>();
}
std::vector<LibraryCell*>
DatabaseHandler::cellSuperset(LibraryCell* cell, int in_size)
{
    if (isAND(cell))
    {
        return andCells(in_size);
    }
    else if (isOR(cell))
    {
        return orCells(in_size);
    }
    else if (isNOR(cell))
    {
        return norCells(in_size);
    }
    else if (isNAND(cell))
    {
        return nandCells(in_size);
    }
    else if (isBuffer(cell))
    {
        return bufferCells();
    }
    else if (isInverter(cell))
    {
        return inverterCells();
    }
    return std::vector<LibraryCell*>();
}
std::vector<LibraryCell*>
DatabaseHandler::invertedEquivalent(LibraryCell* cell)
{
    if (isAND(cell))
    {
        return norCells(libraryInputPins(cell).size());
    }
    else if (isOR(cell))
    {
        return nandCells(libraryInputPins(cell).size());
    }
    else if (isNOR(cell))
    {
        return andCells(libraryInputPins(cell).size());
    }
    else if (isNAND(cell))
    {
        return norCells(libraryInputPins(cell).size());
    }
    return std::vector<LibraryCell*>();
}

LibraryCell*
DatabaseHandler::minimumDrivingInverter(LibraryCell* cell, float extra_cap)
{
    auto invs = inverterCells();
    std::sort(invs.begin(), invs.end(),
              [&](LibraryCell* b1, LibraryCell* b2) -> bool {
                  return area(b1) < area(b2);
              });
    LibraryCell* smallest = nullptr;
    for (auto& inv : invs)
    {
        bool can_drive = true;
        auto out_pin   = bufferOutputPin(inv);
        auto limit     = maxLoad(out_pin);
        if (out_pin)
        {
            for (auto& in_pin : libraryInputPins(cell))
            {
                if (portCapacitance(in_pin) + extra_cap > limit)
                {
                    can_drive = false;
                    break;
                }
            }
            if (can_drive)
            {
                smallest = inv;
                break;
            }
        }
    }
    return smallest;
}
// cluster_threshold:
// 1   : Single buffer cell
// 3/4 : Small set
// 1/4 : Medium set
// 1/12: Large set
// 0   : All buffer cells
std::pair<std::vector<LibraryCell*>, std::vector<LibraryCell*>>
DatabaseHandler::bufferClusters(float cluster_threshold, bool find_superior,
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

    auto buffer_cluster = KCenterClustering::cluster<LibraryCell*>(
        buff_vector, buff_distances, cluster_threshold, 0);
    auto inverter_cluster = KCenterClustering::cluster<LibraryCell*>(
        inv_vector, inv_distances, cluster_threshold, 0);
    return std::pair<std::vector<LibraryCell*>, std::vector<LibraryCell*>>(
        buffer_cluster, inverter_cluster);
}
std::unordered_set<InstanceTerm*>
DatabaseHandler::commutativePins(InstanceTerm* term)
{
    std::unordered_set<LibraryTerm*> comm_lib_pins;
    LibraryTerm*                     lib_pin = libraryPin(term);
    if (!commutative_pins_cache_.count(lib_pin))
    {
        comm_lib_pins.insert(lib_pin);
        for (auto inp : libraryInputPins(lib_pin->libertyCell()))
        {
            if (inp != lib_pin && isCommutative(lib_pin, inp))
            {
                comm_lib_pins.insert(inp);
            }
        }
        commutative_pins_cache_[lib_pin] = comm_lib_pins;
    }
    comm_lib_pins = commutative_pins_cache_[lib_pin];
    std::unordered_set<InstanceTerm*> comm;

    for (auto& pn : inputPins(instance(term)))
    {
        if (pn != term && comm_lib_pins.count(libraryPin(pn)))
        {
            comm.insert(pn);
        }
    }

    return comm;
}
std::vector<LibraryCell*>
DatabaseHandler::equivalentCells(LibraryCell* cell)
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
DatabaseHandler::smallestBufferCell() const
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
DatabaseHandler::tieloCells() const
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
DatabaseHandler::bestPath(int path_count) const
{
    return getPaths(false, path_count);
}
std::vector<PathPoint>
DatabaseHandler::criticalPath(int path_count) const
{
    auto paths = getPaths(true, path_count);
    if (paths.size())
    {
        return paths[0];
    }
    return std::vector<PathPoint>();
}
std::vector<std::vector<PathPoint>>
DatabaseHandler::criticalPaths(int path_count) const
{
    auto paths = getPaths(true, path_count);
    if (path_count < paths.size())
    {
        paths.resize(path_count + 1);
    }
    return paths;
}
std::vector<PathPoint>
DatabaseHandler::worstSlackPath(InstanceTerm* term, bool trim) const
{
    sta::PathRef path;
    // sta_->search()->endpointsInvalid();
    sta_->vertexWorstSlackPath(vertex(term), sta::MinMax::max(), path);
    auto expanded = expandPath(&path);
    if (trim && !path.isNull())
    {
        for (int i = expanded.size() - 1; i >= 0; i--)
        {
            if (expanded[i].pin() == term)
            {
                expanded.resize(i + 1);
                break;
            }
        }
    }
    return expanded;
}
std::vector<PathPoint>
DatabaseHandler::worstArrivalPath(InstanceTerm* term) const
{
    sta::PathRef path;
    sta_->vertexWorstArrivalPath(vertex(term), sta::MinMax::max(), path);
    return expandPath(&path);
}

std::vector<PathPoint>
DatabaseHandler::bestSlackPath(InstanceTerm* term) const
{
    sta::PathRef path;
    sta_->vertexWorstSlackPath(vertex(term), sta::MinMax::min(), path);
    return expandPath(&path);
}
std::vector<PathPoint>
DatabaseHandler::bestArrivalPath(InstanceTerm* term) const
{
    sta::PathRef path;
    sta_->vertexWorstArrivalPath(vertex(term), sta::MinMax::min(), path);
    return expandPath(&path);
}
float
DatabaseHandler::pinSlack(InstanceTerm* term, bool is_rise, bool worst) const
{
    return sta_->pinSlack(
        term, is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(),
        worst ? sta::MinMax::min() : sta::MinMax::max());
}
float
DatabaseHandler::pinSlack(InstanceTerm* term, bool worst) const
{
    return sta_->pinSlack(term,
                          worst ? sta::MinMax::min() : sta::MinMax::max());
}
float
DatabaseHandler::slack(InstanceTerm* term)
{
    return gateDelay(term, loadCapacitance(term)) - required(term);
}
float
DatabaseHandler::slew(InstanceTerm* term) const
{
    return std::max(slew(term, true), slew(term, false));
}
float
DatabaseHandler::slew(InstanceTerm* term, bool is_rise) const
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
DatabaseHandler::arrival(InstanceTerm* term, int ap_index, bool is_rise) const
{

    return sta_->vertexArrival(
        vertex(term), is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(),
        sta_->corners()->findPathAnalysisPt(ap_index));
}
float
DatabaseHandler::required(InstanceTerm* term) const
{
    auto vert = network()->graph()->pinLoadVertex(term);
    auto req  = sta_->vertexRequired(vert, min_max_);
    if (sta::delayInf(req))
    {
        return 0;
    }
    return req;
}
float
DatabaseHandler::required(InstanceTerm* term, bool is_rise,
                          PathAnalysisPoint* path_ap) const
{
    auto vert = network()->graph()->pinLoadVertex(term);
    auto req  = sta_->vertexRequired(
        vert, is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(), path_ap);
    if (sta::delayInf(req))
    {
        return 0;
    }
    return req;
}

std::vector<std::vector<PathPoint>>
DatabaseHandler::getPaths(bool get_max, int path_count) const
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

    bool first_path = true;
    for (auto& path_end : *path_ends)
    {
        result.push_back(expandPath(path_end, !first_path));
        first_path = false;
    }
    delete path_ends;
    return result;
}
InstanceTerm*
DatabaseHandler::worstSlackPin() const
{
    float   ws;
    Vertex* vert;
    sta_->findRequireds();

    sta_->worstSlack(min_max_, ws, vert);
    return vert->pin();
}

float
DatabaseHandler::worstSlack(InstanceTerm* term) const
{
    float ws;
    // sta_->findRequireds();
    auto vert = vertex(term);
    sta_->vertexRequired(vert, sta::MinMax::min());
    sta::PathRef ref;

    sta_->vertexWorstSlackPath(vert, sta::MinMax::max(), ref);
    if (!ref.tag(sta_))
    {
        return sta::INF;
    }
    return ref.slack(sta_);
}
float
DatabaseHandler::worstSlack() const
{
    float   ws;
    Vertex* vert;
    sta_->findRequireds();

    sta_->worstSlack(min_max_, ws, vert);
    return ws;
}
std::vector<std::vector<PathPoint>>
DatabaseHandler::getNegativeSlackPaths() const
{
    std::vector<std::vector<PathPoint>> result;
    sta_->ensureLevelized();
    sta_->search()->findAllArrivals();
    sta_->findRequireds();

    for (auto& vert : *sta_->search()->endpoints())
    {
        sta::PathRef ref;
        sta_->vertexWorstSlackPath(vert, sta::MinMax::max(), ref);
        if (ref.tag(sta_))
        {
            auto vertSlack = ref.slack(sta_);
            if (vertSlack < 0.0)
            {
                result.push_back(expandPath(&ref));
            }
        }
    }
    std::sort(result.begin(), result.end(),
              [&](const std::vector<PathPoint>& p1,
                  const std::vector<PathPoint>& p2) -> bool {
                  return p1[p1.size() - 1].slack() < p2[p2.size() - 1].slack();
              });
    // Remove clock pin
    for (auto& pth : result)
    {
        pth.erase(pth.begin());
    }

    return result;
}
std::vector<PathPoint>
DatabaseHandler::expandPath(sta::PathEnd* path_end, bool enumed) const
{
    return expandPath(path_end->path(), enumed);
}

std::vector<PathPoint>
DatabaseHandler::expandPath(sta::Path* path, bool enumed) const
{
    std::vector<PathPoint> points;
    sta::PathExpanded      expanded(path, sta_);
    for (size_t i = 1; i < expanded.size(); i++)
    {
        auto ref           = expanded.path(i);
        auto pin           = ref->vertex(sta_)->pin();
        auto is_rising     = ref->transition(sta_) == sta::RiseFall::rise();
        auto arrival       = ref->arrival(sta_);
        auto path_ap       = ref->pathAnalysisPt(sta_);
        auto path_required = enumed ? 0 : ref->required(sta_);
        if (!path_required || sta::delayInf(path_required))
        {
            path_required = required(pin, is_rising, path_ap);
        }
        auto slack = enumed ? path_required - arrival : ref->slack(sta_);
        points.push_back(
            PathPoint(pin, is_rising, arrival, path_required, slack, path_ap));
    }
    return points;
}

bool
DatabaseHandler::isCommutative(InstanceTerm* first, InstanceTerm* second) const
{
    return isCommutative(libraryPin(first), libraryPin(second));
}
bool
DatabaseHandler::isCommutative(LibraryTerm* first, LibraryTerm* second) const
{

    if (first == second)
    {
        return true;
    }
    auto cell_lib = first->libertyCell();
    if (cell_lib->isClockGate() || cell_lib->isPad() || cell_lib->isMacro() ||
        cell_lib->hasSequentials())
    {
        return false;
    }
    auto                      output_pins = libraryOutputPins(cell_lib);
    auto                      input_pins  = libraryInputPins(cell_lib);
    std::vector<LibraryTerm*> remaining_pins;
    for (auto& pin : input_pins)
    {
        if (pin != first && pin != second)
        {
            remaining_pins.push_back(pin);
        }
    }
    for (auto& out : output_pins)
    {
        sta::FuncExpr* func = out->function();
        if (!func)
        {
            return false;
        }
        if (func->hasPort(first) && func->hasPort(second))
        {
            std::unordered_map<LibraryTerm*, int> sim_vals;
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 2; j++)
                {

                    for (int m = 0; m < std::pow(2, remaining_pins.size()); m++)
                    {
                        sim_vals[first]  = i;
                        sim_vals[second] = j;
                        int temp         = m;
                        for (auto& rp : remaining_pins)
                        {
                            sim_vals[rp] = temp & 1;
                            temp         = temp >> 1;
                        }
                        int first_result =
                            evaluateFunctionExpression(out, sim_vals);

                        sim_vals[first]  = j;
                        sim_vals[second] = i;
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
DatabaseHandler::levelDriverPins(
    bool reverse, std::unordered_set<InstanceTerm*> filter_pins) const
{
    sta_->ensureGraph();
    sta_->ensureLevelized();

    auto handler_network = network();

    std::vector<InstanceTerm*> terms;
    std::vector<Vertex*>       vertices;
    sta::VertexIterator        itr(handler_network->graph());
    while (itr.hasNext())
    {
        Vertex* vtx = itr.next();
        if (vtx->isDriver(handler_network))
            vertices.push_back(vtx);
    }
    std::sort(
        vertices.begin(), vertices.end(),
        [=](const Vertex* v1, const Vertex* v2) -> bool {
            return (v1->level() < v2->level()) ||
                   (v1->level() == v2->level() &&
                    sta::stringLess(handler_network->pathName(v1->pin()),
                                    handler_network->pathName(v2->pin())));
        });
    for (auto& v : vertices)
    {
        auto pn = v->pin();
        if (!filter_pins.size() || filter_pins.count(pn))
        {
            terms.push_back(v->pin());
        }
    }
    if (reverse)
    {
        std::reverse(terms.begin(), terms.end());
    }
    return terms;
}

InstanceTerm*
DatabaseHandler::faninPin(InstanceTerm* term) const
{
    return faninPin(net(term));
}
InstanceTerm*
DatabaseHandler::faninPin(Net* net) const
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
DatabaseHandler::fanoutInstances(Net* net) const
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
DatabaseHandler::driverInstances() const
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
DatabaseHandler::fanoutCount(Net* net, bool include_top_level) const
{
    return fanoutPins(net, include_top_level).size();
}

Point
DatabaseHandler::location(InstanceTerm* term)
{
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    network()->staToDb(term, iterm, bterm);
    if (iterm)
    {
        int x, y;
        if (iterm->getAvgXY(&x, &y))
        {
            return Point(x, y);
        }
        else
        {
            iterm->getInst()->getOrigin(x, y);
            return Point(x, y);
        }
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
DatabaseHandler::location(Instance* inst)
{
    odb::dbInst* dinst = network()->staToDb(inst);
    int          x, y;
    dinst->getOrigin(x, y);
    return Point(x, y);
}
void
DatabaseHandler::setLocation(Instance* inst, Point pt)
{
    odb::dbInst* dinst = network()->staToDb(inst);
    dinst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    dinst->setLocation(pt.getX(), pt.getY());
}

float
DatabaseHandler::area(Instance* inst) const
{
    odb::dbInst*   dinst  = network()->staToDb(inst);
    odb::dbMaster* master = dinst->getMaster();
    if (master->isCoreAutoPlaceable())
    {
        return dbuToMeters(master->getWidth()) *
               dbuToMeters(master->getHeight());
    }
    return 0.0;
}
float
DatabaseHandler::area(LibraryCell* cell) const
{
    odb::dbMaster* master = db_->findMaster(name(cell).c_str());
    if (master->isCoreAutoPlaceable())
    {
        return dbuToMeters(master->getWidth()) *
               dbuToMeters(master->getHeight());
    }
    return 0.0;
}

float
DatabaseHandler::area() const
{
    float total_area = 0.0;
    for (auto inst : top()->getInsts())
    {
        auto master = inst->getMaster();
        if (master->isCoreAutoPlaceable())
        {
            total_area += dbuToMeters(master->getWidth()) *
                          dbuToMeters(master->getHeight());
        }
    }
    return total_area;
}

std::string
DatabaseHandler::unitScaledArea(float ar) const
{
    auto unit = network()->units()->distanceUnit();
    return std::string(unit->asString(ar / unit->scale(), 0));
}
float
DatabaseHandler::power(std::vector<Instance*>& insts)
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
DatabaseHandler::power()
{
    sta::PowerResult total, sequential, combinational, macro, pad;
    sta_->power(corner_, total, sequential, combinational, macro, pad);
    return total.total();
}
LibraryTerm*
DatabaseHandler::libraryPin(InstanceTerm* term) const
{
    return network()->libertyPort(term);
}

Port*
DatabaseHandler::topPort(InstanceTerm* term) const
{
    return network()->port(term);
}
bool
DatabaseHandler::isClocked(InstanceTerm* term) const
{
    return sta_->search()->isClock(vertex(term));
}
bool
DatabaseHandler::isPrimary(Net* net) const
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
DatabaseHandler::libraryCell(InstanceTerm* term) const
{
    auto inst = network()->instance(term);
    if (inst)
    {
        return network()->libertyCell(inst);
    }
    return nullptr;
}

LibraryCell*
DatabaseHandler::libraryCell(Instance* inst) const
{
    return network()->libertyCell(inst);
}

LibraryCell*
DatabaseHandler::libraryCell(const char* name) const
{
    return network()->findLibertyCell(name);
}
LibraryCell*
DatabaseHandler::largestLibraryCell(LibraryCell* cell)
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
DatabaseHandler::dbuToMeters(int dist) const
{
    return static_cast<double>(dist) /
           (db_->getTech()->getDbUnitsPerMicron() * 1E6);
}
double
DatabaseHandler::dbuToMicrons(int dist) const
{
    return (1.0 * dist) / db_->getTech()->getLefUnits();
}
bool
DatabaseHandler::isPlaced(InstanceTerm* term) const
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
DatabaseHandler::isPlaced(Instance* inst) const
{
    odb::dbInst*           dinst  = network()->staToDb(inst);
    odb::dbPlacementStatus status = dinst->getPlacementStatus();
    return status == odb::dbPlacementStatus::PLACED ||
           status == odb::dbPlacementStatus::LOCKED ||
           status == odb::dbPlacementStatus::FIRM ||
           status == odb::dbPlacementStatus::COVER;
}
bool
DatabaseHandler::isDriver(InstanceTerm* term) const
{
    return network()->isDriver(term);
}

float
DatabaseHandler::pinCapacitance(InstanceTerm* term) const
{
    auto port = network()->libertyPort(term);
    if (port)
    {
        return pinCapacitance(port);
    }
    return 0.0;
}
void
DatabaseHandler::ripupBuffers(std::unordered_set<Instance*> buffers)
{
    for (auto& cell : buffers)
    {
        ripupBuffer(cell);
    }
}
void
DatabaseHandler::ripupBuffer(Instance* buffer)
{
    auto input_pins  = inputPins(buffer);
    auto output_pins = outputPins(buffer);
    if (input_pins.size() == 1 && output_pins.size() == 1)
    {
        auto output_pin = output_pins[0];
        auto input_pin  = input_pins[0];
        auto source_net = net(input_pin);
        auto sink_net   = net(output_pin);
        if (!source_net)
        {
            PSN_LOG_ERROR("Cannot find buffer driving net");
            return;
        }
        if (!sink_net)
        {
            PSN_LOG_ERROR("Cannot find buffer driven net");
            return;
        }
        auto driven_pins = fanoutPins(sink_net, true);
        disconnectAll(sink_net);
        for (auto& p : driven_pins)
        {
            if (p != output_pin)
            {
                connect(source_net, p);
            }
        }
        disconnect(input_pin);
        del(buffer);
        del(sink_net);
        if (hasWireRC())
        {
            calculateParasitics(source_net);
        }
    }
    else
    {
        PSN_LOG_ERROR("ripupBuffer() requires buffer cells");
    }
}

float
DatabaseHandler::pinCapacitance(LibraryTerm* term) const
{
    float cap1 = term->capacitance(sta::RiseFall::rise(), sta::MinMax::max());
    float cap2 = term->capacitance(sta::RiseFall::fall(), sta::MinMax::max());
    return std::max(cap1, cap2);
}
float
DatabaseHandler::pinSlewLimit(InstanceTerm* term, bool* exists) const
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
DatabaseHandler::pinAverageRise(LibraryTerm* from, LibraryTerm* to) const
{
    return pinTableAverage(from, to, true, true);
}
float
DatabaseHandler::pinAverageFall(LibraryTerm* from, LibraryTerm* to) const
{
    return pinTableAverage(from, to, true, false);
}
float
DatabaseHandler::pinAverageRiseTransition(LibraryTerm* from,
                                          LibraryTerm* to) const
{
    return pinTableAverage(from, to, false, true);
}
float
DatabaseHandler::pinAverageFallTransition(LibraryTerm* from,
                                          LibraryTerm* to) const
{
    return pinTableAverage(from, to, false, false);
}

float
DatabaseHandler::pinTableLookup(LibraryTerm* from, LibraryTerm* to, float slew,
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
DatabaseHandler::slew(LibraryTerm* term, float load_cap, float* tr_slew)
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
DatabaseHandler::bufferFixedInputSlew(LibraryCell* buffer_cell, float cap)
{
    auto output_pin      = bufferOutputPin(buffer_cell);
    auto min_buff_cap    = bufferInputCapacitance(smallestBufferCell());
    auto intrinsic_delay = gateDelay(output_pin, min_buff_cap) -
                           output_pin->driveResistance() * min_buff_cap;
    auto res = output_pin->driveResistance();
    return res * cap + intrinsic_delay;
}

float
DatabaseHandler::pinTableAverage(LibraryTerm* from, LibraryTerm* to,
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
DatabaseHandler::loadCapacitance(InstanceTerm* term) const
{
    return network()->graphDelayCalc()->loadCap(term, dcalc_ap_);
}
Instance*
DatabaseHandler::instance(const char* name) const
{
    return network()->findInstance(name);
}
BlockTerm*
DatabaseHandler::port(const char* name) const
{
    return network()->findPin(network()->topInstance(), name);
}

InstanceTerm*
DatabaseHandler::pin(const char* name) const
{
    return network()->findPin(name);
}

Instance*
DatabaseHandler::instance(InstanceTerm* term) const
{
    return network()->instance(term);
}
Net*
DatabaseHandler::net(const char* name) const
{
    return network()->findNet(name);
}

LibraryTerm*
DatabaseHandler::libraryPin(const char* cell_name, const char* pin_name) const
{
    LibraryCell* cell = libraryCell(cell_name);
    if (!cell)
    {
        return nullptr;
    }
    return libraryPin(cell, pin_name);
}
LibraryTerm*
DatabaseHandler::libraryPin(LibraryCell* cell, const char* pin_name) const
{
    return cell->findLibertyPort(pin_name);
}

std::vector<LibraryTerm*>
DatabaseHandler::libraryPins(Instance* inst) const
{
    return libraryPins(libraryCell(inst));
}
std::vector<LibraryTerm*>
DatabaseHandler::libraryPins(LibraryCell* cell) const
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
DatabaseHandler::libraryInputPins(LibraryCell* cell) const
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
DatabaseHandler::libraryOutputPins(LibraryCell* cell) const
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
DatabaseHandler::filterPins(std::vector<InstanceTerm*>& terms,
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
DatabaseHandler::setLegalizer(Legalizer legalizer)
{
    legalizer_ = legalizer;
}
bool
DatabaseHandler::legalize(int max_displacement)
{
    if (legalizer_)
    {
        return legalizer_(max_displacement);
    }
    return false;
}
bool
DatabaseHandler::isTopLevel(InstanceTerm* term) const
{
    return network()->isTopLevelPort(term);
}
void
DatabaseHandler::del(Net* net) const
{
    sta_->deleteNet(net);
}
void
DatabaseHandler::del(Instance* inst) const
{
    sta_->deleteInstance(inst);
}
int
DatabaseHandler::disconnectAll(Net* net) const
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
DatabaseHandler::connect(Net* net, InstanceTerm* term) const
{
    auto inst      = network()->instance(term);
    auto term_port = network()->port(term);
    sta_->connectPin(inst, term_port, net);
}

void
DatabaseHandler::disconnect(InstanceTerm* term) const
{
    sta_->disconnectPin(term);
}

void
DatabaseHandler::swapPins(InstanceTerm* first, InstanceTerm* second)
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
DatabaseHandler::createInstance(const char* inst_name, LibraryCell* cell)
{
    return sta_->makeInstance(inst_name, cell, network()->topInstance());
}

void
DatabaseHandler::createClock(const char*             clock_name,
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
DatabaseHandler::createClock(const char*              clock_name,
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
DatabaseHandler::createNet(const char* net_name)
{
    auto net = sta_->makeNet(net_name, network()->topInstance());
    return net;
}
float
DatabaseHandler::resistance(LibraryTerm* term) const
{
    return term->driveResistance();
}
float
DatabaseHandler::resistancePerMicron() const
{
    if (!has_wire_rc_)
    {
        PSN_LOG_WARN("Wire RC is not set or invalid");
    }
    if (res_per_micron_callback_)
    {
        return res_per_micron_callback_();
    }
    return res_per_micron_;
}
float
DatabaseHandler::capacitancePerMicron() const
{
    if (!has_wire_rc_)
    {
        PSN_LOG_WARN("Wire RC is not set or invalid");
    }
    if (cap_per_micron_callback_)
    {
        return cap_per_micron_callback_();
    }
    return cap_per_micron_;
}

void
DatabaseHandler::connect(Net* net, Instance* inst, LibraryTerm* port) const
{
    sta_->connectPin(inst, port, net);
}
void
DatabaseHandler::connect(Net* net, Instance* inst, Port* port) const
{
    sta_->connectPin(inst, port, net);
}

std::vector<Net*>
DatabaseHandler::nets() const
{
    std::vector<Net*> all_nets;
    sta::NetIterator* net_iter =
        network()->netIterator(network()->topInstance());
    while (net_iter->hasNext())
    {
        all_nets.push_back(net_iter->next());
    }
    delete net_iter;
    return all_nets;
}
std::vector<Instance*>
DatabaseHandler::instances() const
{
    sta::InstanceSeq  all_insts;
    sta::PatternMatch pattern("*");
    network()->findInstancesMatching(network()->topInstance(), &pattern,
                                     &all_insts);
    return static_cast<std::vector<Instance*>>(all_insts);
}
void
DatabaseHandler::setDontUse(std::vector<std::string>& cell_names)
{
    for (auto& name : cell_names)
    {
        auto cell = libraryCell(name.c_str());
        if (!cell)
        {
            PSN_LOG_WARN("Cannot find cell with the name {}", name);
        }
        else
        {
            dont_use_.insert(cell);
        }
    }
}

std::string
DatabaseHandler::topName() const
{
    return std::string(network()->name(network()->topInstance()));
}
std::string
DatabaseHandler::generateNetName(int& start_index)
{
    std::string name;
    do
        name = std::string("psn_net_") + std::to_string(start_index++);
    while (net(name.c_str()));
    return name;
}
std::string
DatabaseHandler::generateInstanceName(const std::string& prefix,
                                      int&               start_index)
{
    std::string name;
    do
        name =
            std::string("psn_inst_") + prefix + std::to_string(start_index++);
    while (instance(name.c_str()));
    return name;
}

std::string
DatabaseHandler::name(Block* object) const
{
    return std::string(object->getConstName());
}
std::string
DatabaseHandler::name(Net* object) const
{
    return std::string(network()->name(object));
}
std::string
DatabaseHandler::name(Instance* object) const
{
    return std::string(network()->name(object));
}
std::string
DatabaseHandler::name(BlockTerm* object) const
{
    return std::string(network()->name(object));
}
std::string
DatabaseHandler::name(Library* object) const
{
    return std::string(object->getConstName());
}
std::string
DatabaseHandler::name(LibraryCell* object) const
{
    return std::string(network()->name(network()->cell(object)));
}
std::string
DatabaseHandler::name(LibraryTerm* object) const
{
    return object->name();
}

Library*
DatabaseHandler::library() const
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
DatabaseHandler::technology() const
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
DatabaseHandler::hasLiberty() const
{
    sta::LibertyLibraryIterator* iter = network()->libertyLibraryIterator();
    return iter->hasNext();
}

Block*
DatabaseHandler::top() const
{
    Chip* chip = db_->getChip();
    if (!chip)
    {
        return nullptr;
    }

    Block* block = chip->getBlock();
    return block;
}

DatabaseHandler::~DatabaseHandler()
{
}
void
DatabaseHandler::clear()
{
    sta_->clear();
    db_->clear();
}
DatabaseStaNetwork*
DatabaseHandler::network() const
{
    return sta_->getDbNetwork();
}
DatabaseSta*
DatabaseHandler::sta() const
{
    return sta_;
}
float
DatabaseHandler::maxLoad(LibraryCell* cell)
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
DatabaseHandler::maxLoad(LibraryTerm* term)
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
DatabaseHandler::isInput(InstanceTerm* term) const
{
    return network()->direction(term)->isInput();
}
bool
DatabaseHandler::isOutput(InstanceTerm* term) const
{
    return network()->direction(term)->isOutput();
}
bool
DatabaseHandler::isAnyInput(InstanceTerm* term) const
{
    return network()->direction(term)->isAnyInput();
}
bool
DatabaseHandler::isAnyOutput(InstanceTerm* term) const
{
    return network()->direction(term)->isAnyOutput();
}
bool
DatabaseHandler::isBiDirect(InstanceTerm* term) const
{
    return network()->direction(term)->isBidirect();
}
bool
DatabaseHandler::isTriState(InstanceTerm* term) const
{
    return network()->direction(term)->isTristate();
}
bool
DatabaseHandler::isSingleOutputCombinational(Instance* inst) const
{
    if (inst == network()->topInstance())
    {
        return false;
    }
    return isSingleOutputCombinational(libraryCell(inst));
}
bool
DatabaseHandler::isBuffer(LibraryCell* cell) const
{
    return cell->isBuffer();
}
bool
DatabaseHandler::isInverter(LibraryCell* cell) const
{
    auto out_pins = libraryOutputPins(cell);
    return isSingleOutputCombinational(cell) && out_pins[0]->function() &&
           out_pins[0]->function()->op() == sta::FuncExpr::op_not &&
           libraryInputPins(cell).size() == 1;
}
void
DatabaseHandler::replaceInstance(Instance* inst, LibraryCell* cell)
{
    bool both_inv  = isInverter(libraryCell(inst)) && isInverter(cell);
    bool both_buff = isBuffer(libraryCell(inst)) && isBuffer(cell);

    if (!both_inv && !both_buff && isSingleOutputCombinational(inst) &&
        isSingleOutputCombinational(cell) && inputPins(inst).size() == 1 &&
        libraryInputPins(cell).size() == 1)
    {
        // Manually replace inverters/buffers
        auto in_pin     = inputPins(inst)[0];
        auto out_pin    = outputPins(inst)[0];
        auto input_net  = net(in_pin);
        auto output_net = net(out_pin);
        disconnect(in_pin);
        disconnect(out_pin);
        auto current_loc  = location(inst);
        auto current_name = name(inst);
        auto db_inst      = network()->staToDb(inst);
        auto current_rot  = db_inst->getOrient();
        del(inst);
        auto new_inst    = createInstance(current_name.c_str(), cell);
        auto new_db_inst = network()->staToDb(new_inst);
        new_db_inst->setOrient(current_rot);
        setLocation(new_inst, current_loc);
        connect(input_net, inputPins(new_inst)[0]);
        connect(output_net, outputPins(new_inst)[0]);
    }
    else
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
}

bool
DatabaseHandler::isSingleOutputCombinational(LibraryCell* cell) const
{
    if (!cell)
    {
        return false;
    }
    auto output_pins = libraryOutputPins(cell);

    return (output_pins.size() == 1 && isCombinational(cell));
}
bool
DatabaseHandler::isCombinational(Instance* inst) const
{
    if (inst == network()->topInstance())
    {
        return false;
    }
    return isCombinational(libraryCell(inst));
}
bool
DatabaseHandler::isCombinational(LibraryCell* cell) const
{
    if (!cell)
    {
        return false;
    }
    return (!cell->isClockGate() && !cell->isPad() && !cell->isMacro() &&
            !cell->hasSequentials());
}

bool
DatabaseHandler::isSpecial(Net* net) const
{
    odb::dbNet* db_net = network()->staToDb(net);
    return db_net && db_net->isSpecial();
}

void
DatabaseHandler::setWireRC(float res_per_micron, float cap_per_micron,
                           bool reset_delays)
{
    if (reset_delays)
    {
        sta_->ensureGraph();
        sta_->ensureLevelized();
        sta_->graphDelayCalc()->delaysInvalid();
        sta_->search()->arrivalsInvalid();
    }
    res_per_micron_ = res_per_micron;
    cap_per_micron_ = cap_per_micron;
    has_wire_rc_    = true;
    calculateParasitics();
    sta_->findDelays();
}
void
DatabaseHandler::setWireRC(ParasticsCallback res_per_micron,
                           ParasticsCallback cap_per_micron)
{
    has_wire_rc_             = true;
    res_per_micron_callback_ = res_per_micron;
    cap_per_micron_callback_ = cap_per_micron;
}

void
DatabaseHandler::setDontUseCallback(DontUseCallback dont_use_callback)
{
    dont_use_callback_ = dont_use_callback;
}
void
DatabaseHandler::setComputeParasiticsCallback(
    ComputeParasiticsCallback compute_parasitics_callback)
{
    compute_parasitics_callback_ = compute_parasitics_callback;
}

bool
DatabaseHandler::hasWireRC()
{
    return has_wire_rc_;
}

bool
DatabaseHandler::isInput(LibraryTerm* term) const
{
    return term->direction()->isInput();
}
bool
DatabaseHandler::isOutput(LibraryTerm* term) const
{
    return term->direction()->isOutput();
}
bool
DatabaseHandler::isAnyInput(LibraryTerm* term) const
{
    return term->direction()->isAnyInput();
}
bool
DatabaseHandler::isAnyOutput(LibraryTerm* term) const
{
    return term->direction()->isAnyOutput();
}
bool
DatabaseHandler::isBiDirect(LibraryTerm* term) const
{
    return term->direction()->isBidirect();
}
bool
DatabaseHandler::isTriState(LibraryTerm* term) const
{
    return term->direction()->isTristate();
}
float
DatabaseHandler::capacitanceLimit(InstanceTerm* term) const
{
    const sta::Corner*   corner;
    const sta::RiseFall* rf;
    float                cap, limit, diff;
    sta_->checkCapacitance(term, nullptr, min_max_, corner, rf, cap, limit,
                           diff);
    return limit;
}

bool
DatabaseHandler::violatesMaximumCapacitance(InstanceTerm* term,
                                            float limit_scale_factor) const
{
    float load_cap = loadCapacitance(term);
    return violatesMaximumCapacitance(term, load_cap, limit_scale_factor);
}

// Assumes sta checkCapacitanceLimitPreamble is called()
bool
DatabaseHandler::violatesMaximumCapacitance(InstanceTerm* term, float load_cap,
                                            float limit_scale_factor) const
{
    const sta::Corner*   corner;
    const sta::RiseFall* rf;
    float                cap, limit, ignore;
    sta_->checkCapacitance(term, nullptr, sta::MinMax::max(), corner, rf, cap,
                           limit, ignore);
    float diff = (limit_scale_factor * limit) - cap;
    return diff < 0.0 && limit > 0.0;
}

// Assumes sta checkSlewLimitPreamble is called()
bool
DatabaseHandler::violatesMaximumTransition(InstanceTerm* term,
                                           float         limit_scale_factor)
{
    const sta::Corner*   corner;
    const sta::RiseFall* rf;
    float                slew, limit, ignore;

    sta_->checkSlew(term, nullptr, sta::MinMax::max(), false, corner, rf, slew,
                    limit, ignore);
    float diff = (limit_scale_factor * limit) - slew;
    return diff < 0.0 && limit > 0.0;
}

ElectircalViolation
DatabaseHandler::hasElectricalViolation(InstanceTerm* pin,
                                        float         cap_scale_factor,
                                        float         trans_sacle_factor)
{
    auto pin_net   = net(pin);
    auto net_pins  = pins(pin_net);
    bool vio_trans = false;
    bool vio_cap   = false;
    for (auto connected_pin : net_pins)
    {
        if (violatesMaximumTransition(connected_pin, trans_sacle_factor))
        {
            vio_trans = true;
            if (vio_cap)
            {
                break;
            }
        }
        else if (violatesMaximumCapacitance(connected_pin, cap_scale_factor))
        {
            vio_cap = true;
            if (vio_trans)
            {
                break;
            }
        }
    }
    if (vio_cap && vio_trans)
    {
        return ElectircalViolation::CapacitanceAndTransition;
    }
    else if (vio_trans)
    {
        return ElectircalViolation::Transition;
    }
    else if (vio_cap)
    {
        return ElectircalViolation::Capacitance;
    }
    else
    {
        return ElectircalViolation::None;
    }
}

std::vector<InstanceTerm*>
DatabaseHandler::maximumTransitionViolations(float limit_scale_factor) const
{
    auto vio_pins = sta_->pinSlewLimitViolations(corner_, sta::MinMax::max());
    return std::vector<InstanceTerm*>(vio_pins->begin(), vio_pins->end());
}
std::vector<InstanceTerm*>
DatabaseHandler::maximumCapacitanceViolations(float limit_scale_factor) const
{
    sta_->findDelays();
    auto vio_pins =
        sta_->pinCapacitanceLimitViolations(corner_, sta::MinMax::max());
    return std::vector<InstanceTerm*>(vio_pins->begin(), vio_pins->end());
}

bool
DatabaseHandler::isLoad(InstanceTerm* term) const
{
    return network()->isLoad(term);
}
void
DatabaseHandler::makeEquivalentCells()
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
DatabaseHandler::allLibs() const
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
DatabaseHandler::resetCache()
{
    has_equiv_cells_           = false;
    has_buffer_inverter_seq_   = false;
    has_target_loads_          = false;
    has_library_cell_mappings_ = false;
    maximum_area_valid_        = false;
    target_load_map_.clear();
    resetLibraryMapping();
}
void
DatabaseHandler::findTargetLoads()
{
    auto all_libs = allLibs();
    findTargetLoads(&all_libs);
    has_target_loads_ = true;
}

Vertex*
DatabaseHandler::vertex(InstanceTerm* term) const
{
    Vertex *vertex, *bidirect_drvr_vertex;
    sta_->graph()->pinVertices(term, vertex, bidirect_drvr_vertex);
    return vertex;
}

int
DatabaseHandler::evaluateFunctionExpression(
    InstanceTerm* term, std::unordered_map<LibraryTerm*, int>& inputs) const
{
    return evaluateFunctionExpression(libraryPin(term), inputs);
}
int
DatabaseHandler::evaluateFunctionExpression(
    LibraryTerm* term, std::unordered_map<LibraryTerm*, int>& inputs) const
{
    return evaluateFunctionExpression(term->function(), inputs);
}
int
DatabaseHandler::evaluateFunctionExpression(
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
DatabaseHandler::bufferChainDelayPenalty(float load_cap)
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
DatabaseHandler::computeBuffersDelayPenalty(bool include_inverting)
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
DatabaseHandler::largestLoadCapacitancePin(Instance* cell)
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
DatabaseHandler::largestInputCapacitanceLibraryPin(Instance* cell)
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
DatabaseHandler::largestInputCapacitance(Instance* cell)
{
    return largestInputCapacitance(libraryCell(cell));
}
float
DatabaseHandler::largestInputCapacitance(LibraryCell* cell)
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

std::shared_ptr<LibraryCellMapping>
DatabaseHandler::getLibraryCellMapping(LibraryCell* cell)
{
    if (!cell || !truth_tables_.count(cell) ||
        !library_cell_mappings_.count(truth_tables_[cell]))
    {
        return nullptr;
    }
    auto mapping = library_cell_mappings_[truth_tables_[cell]];
    return mapping;
}
std::shared_ptr<LibraryCellMapping>
DatabaseHandler::getLibraryCellMapping(Instance* inst)
{
    return getLibraryCellMapping(libraryCell(inst));
}
std::unordered_set<LibraryCell*>
DatabaseHandler::truthTableToCells(std::string table_id)
{
    if (!function_to_cell_.count(table_id))
    {
        return std::unordered_set<LibraryCell*>();
    }
    return function_to_cell_[table_id];
}

std::string
DatabaseHandler::cellToTruthTable(LibraryCell* cell)
{
    if (!cell || !truth_tables_.count(cell) ||
        !library_cell_mappings_.count(truth_tables_[cell]))
    {
        return "";
    }
    return truth_tables_[cell];
}
void
DatabaseHandler::buildLibraryMappings(int max_length)
{
    auto buffer_lib   = bufferCells();
    auto inverter_lib = inverterCells();
    buildLibraryMappings(max_length, buffer_lib, inverter_lib);
}
void
DatabaseHandler::buildLibraryMappings(int                        max_length,
                                      std::vector<LibraryCell*>& buffer_lib,
                                      std::vector<LibraryCell*>& inverter_lib)
{
    resetLibraryMapping();
    std::unordered_map<LibraryCell*, std::bitset<64>> truth_tables_sim;

    std::unordered_map<sta::FuncExpr*, std::bitset<64>> function_cache;

    std::vector<LibraryCell*> all_cells;
    std::unordered_set<std::string>
         all_tables; // Table represented as <input size>_<simulation values>
    auto all_libs = allLibs();
    std::unordered_map<int,
                       std::vector<std::vector<std::string>>>
                                          chain_length_map; // Key is the chain length
    std::vector<std::vector<std::string>> all_chains;
    std::unordered_map<std::vector<std::string>*, int>
        chain_max_length; // Max length for a given chain before it outputs
                          // constants.

    // Calculate truth tables
    for (auto& lib : all_libs)
    {
        sta::LibertyCellIterator cell_iter(lib);
        while (cell_iter.hasNext())
        {
            auto lib_cell   = cell_iter.next();
            auto input_pins = libraryInputPins(lib_cell);
            if (!dontUse(lib_cell) && isSingleOutputCombinational(lib_cell) &&
                input_pins.size() < 32)
            {
                auto           output_pins = libraryOutputPins(lib_cell);
                auto           output_pin  = output_pins[0];
                sta::FuncExpr* output_func = output_pin->function();
                if (output_func)
                {
                    std::bitset<64> table = 0;
                    if (function_cache.count(output_func))
                    {
                        table = function_cache[output_func];
                    }
                    else
                    {
                        table = std::bitset<64>(computeTruthTable(lib_cell));
                        function_cache[output_func] = table;
                    }
                    all_cells.push_back(lib_cell);
                    std::string table_id = std::to_string(input_pins.size()) +
                                           "_" + table.to_string();
                    truth_tables_sim[lib_cell] = table;
                    truth_tables_[lib_cell]    = table_id;
                    function_to_cell_[table_id].insert(lib_cell);
                    if (!all_tables.count(table_id))
                    {
                        chain_length_map[1].push_back(std::vector<std::string>(
                            {table_id})); // Base chain, consisting of
                                          // the
                        // single cell.

                        all_tables.insert(table_id);
                    }
                }
                else
                {
                    truth_tables_[lib_cell] = "";
                }
            }
            else
            {
                truth_tables_[lib_cell] = "";
            }
        }
    }

    for (int chain_length = 2; chain_length <= max_length;
         chain_length++) // Chains of length 1 are already added above
    {
        for (auto& chain : chain_length_map[chain_length - 1])
        {
            if (!chain_max_length.count(&chain) ||
                chain_max_length[&chain] >= chain_length)
            {
                auto starting_table_id = chain[0];
                auto random_cell =
                    *(function_to_cell_[starting_table_id].begin());
                auto starting_input_pins  = libraryInputPins(random_cell);
                auto starting_output_pins = libraryOutputPins(random_cell);
                int  table                = 0;
                sta::FuncExpr* starting_output_func =
                    starting_output_pins[0]->function();
                int prev_output;
                for (int i = 0; i < std::pow(2, starting_input_pins.size());
                     ++i)
                {
                    std::unordered_map<LibraryTerm*, int> sim_vals;
                    std::bitset<64>                       input_bits(i);
                    for (size_t j = 0; j < starting_input_pins.size(); j++)
                    {
                        sim_vals[starting_input_pins[j]] =
                            input_bits.test(starting_input_pins.size() - j - 1);
                    }
                    prev_output = evaluateFunctionExpression(
                        starting_output_func, sim_vals);
                    for (size_t m = 1; m < chain.size(); m++)
                    {
                        sim_vals.clear();
                        auto node_table_id = chain[m];
                        auto node_random_cell =
                            *(function_to_cell_[node_table_id].begin());

                        auto node_input_pins =
                            libraryInputPins(node_random_cell);
                        auto node_output_pins =
                            libraryOutputPins(node_random_cell);
                        sta::FuncExpr* node_output_func =
                            node_output_pins[0]->function();
                        for (auto& in_pin : node_input_pins)
                        {
                            sim_vals[in_pin] = prev_output;
                        }
                        prev_output = evaluateFunctionExpression(
                            node_output_func, sim_vals);
                    }
                    table |= prev_output << i;
                }
                if (table && ~table) // Not a constant
                {
                    for (auto& new_table : all_tables)
                    {
                        // Only accept buffer/inverter for now
                        if (new_table.substr(0, 2) == "1_")
                        {
                            std::bitset<64> table_bits(table);

                            auto new_table_random_cell =
                                *(function_to_cell_[new_table].begin());
                            auto new_table_input_pins =
                                libraryInputPins(new_table_random_cell);
                            auto new_table_output_pins =
                                libraryOutputPins(new_table_random_cell);
                            int            table = 0;
                            sta::FuncExpr* new_table_output_func =
                                new_table_output_pins[0]->function();
                            std::unordered_map<LibraryTerm*, int> sim_vals;

                            int new_chain_table = 0;
                            for (int i = 0;
                                 i < std::pow(2, starting_input_pins.size());
                                 ++i)
                            {
                                for (auto& in_pin : new_table_input_pins)
                                {
                                    sim_vals[in_pin] = table_bits.test(i);
                                }
                                new_chain_table |=
                                    evaluateFunctionExpression(
                                        new_table_output_func, sim_vals)
                                    << i;
                            }
                            if (new_chain_table &&
                                ~new_chain_table) // Not a constant
                            {
                                auto new_chain = chain;
                                new_chain.push_back(new_table);
                                chain_length_map[chain_length].push_back(
                                    new_chain);

                                std::string chain_id =
                                    std::to_string(starting_input_pins.size()) +
                                    "_" +
                                    std::bitset<64>(new_chain_table)
                                        .to_string();
                                if (function_to_cell_.count(
                                        chain_id)) // We found an equivalent
                                                   // cell to this chain
                                {
                                    if (!library_cell_mappings_.count(chain_id))
                                    {
                                        library_cell_mappings_[chain_id] =
                                            std::make_shared<LibraryCellMapping>(
                                                chain_id);
                                    }
                                    auto& mapping =
                                        library_cell_mappings_[chain_id];
                                    if (!mapping->mappings()->count(
                                            new_chain[0]))
                                    {
                                        auto root_cell = std::make_shared<
                                            LibraryCellMappingNode>(
                                            name(*(
                                                function_to_cell_[new_chain[0]]
                                                    .begin())),
                                            new_chain[0], nullptr,
                                            new_chain[0] == chain_id, false,
                                            isBuffer(*(
                                                function_to_cell_[new_chain[0]]
                                                    .begin())),
                                            isInverter(*(
                                                function_to_cell_[new_chain[0]]
                                                    .begin())),
                                            0);
                                        root_cell->setSelf(root_cell);
                                        mapping->mappings()->insert(
                                            {new_chain[0], root_cell});
                                    }
                                    auto it =
                                        mapping->mappings()->at(new_chain[0]);
                                    for (size_t i = 1; i < new_chain.size();
                                         i++)
                                    {
                                        if (!it->children().count(new_chain[i]))
                                        {
                                            auto new_node = std::make_shared<
                                                LibraryCellMappingNode>(
                                                name(*(function_to_cell_
                                                           [new_chain[i]]
                                                               .begin())),
                                                new_chain[i], it.get(), false,
                                                false,
                                                isBuffer(*(function_to_cell_
                                                               [new_chain[i]]
                                                                   .begin())),
                                                isInverter(*(function_to_cell_
                                                                 [new_chain[i]]
                                                                     .begin())),
                                                it->level() + 1);
                                            it->children()[new_chain[i]] =
                                                new_node;
                                            new_node->setSelf(new_node);
                                        }
                                        if (new_chain[i] == new_chain[i - 1])
                                        {
                                            it->setRecurring(true);
                                        }
                                        it = it->children()[new_chain[i]];
                                        if (i == new_chain.size() - 1)
                                        {
                                            it->setTerminal(true);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                chain_max_length[&chain] = chain_length;
                            }
                        }
                    }
                }
                else
                {
                    chain_max_length[&chain] = chain_length - 1;
                }
            }
        }
    }

    has_library_cell_mappings_ = true;
}
int
DatabaseHandler::computeTruthTable(LibraryCell* lib_cell)
{
    int            table       = 0;
    auto           output_pins = libraryOutputPins(lib_cell);
    auto           input_pins  = libraryInputPins(lib_cell);
    auto           output_pin  = output_pins[0];
    sta::FuncExpr* output_func = output_pin->function();
    for (int i = 0; i < std::pow(2, input_pins.size()); ++i)
    {
        std::unordered_map<LibraryTerm*, int> sim_vals;
        std::bitset<64>                       input_bits(i);
        for (size_t j = 0; j < input_pins.size(); j++)
        {
            sim_vals[input_pins[j]] =
                input_bits.test(input_pins.size() - j - 1);
        }
        table |= evaluateFunctionExpression(output_func, sim_vals) << i;
    }
    return table;
}
void
DatabaseHandler::resetLibraryMapping()
{
    has_library_cell_mappings_ = false;
    library_cell_mappings_.clear();
    function_to_cell_.clear();
    truth_tables_.clear();
}

/* The following is borrowed from James Cherry's Resizer Code */

// Find a target slew for the libraries and then
// a target load for each cell that gives the target slew.
void
DatabaseHandler::findTargetLoads(std::vector<Liberty*>* resize_libs)
{
    // Find target slew across all buffers in the libraries.
    findBufferTargetSlews(resize_libs);
    for (auto lib : *resize_libs)
        findTargetLoads(lib, target_slews_);
}

float
DatabaseHandler::targetLoad(LibraryCell* cell)
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
DatabaseHandler::coreArea() const
{
    auto block = top();
    Rect core;
    block->getCoreArea(core);
    return dbuToMeters(core.dx()) * dbuToMeters(core.dy());
}

bool
DatabaseHandler::maximumUtilizationViolation() const
{
    return sta::fuzzyGreaterEqual(area(), maximumArea());
}
void
DatabaseHandler::setMaximumArea(float area)
{
    maximum_area_       = area;
    maximum_area_valid_ = true;
}

void
DatabaseHandler::setMaximumArea(MaxAreaCallback maximum_area_callback)
{
    maximum_area_callback_ = maximum_area_callback;
    maximum_area_valid_    = true;
}
void
DatabaseHandler::setUpdateDesignArea(
    UpdateDesignAreaCallback update_design_area_callback)
{
    update_design_area_callback_ = update_design_area_callback;
}
void
DatabaseHandler::notifyDesignAreaChanged(float new_area)
{
    if (update_design_area_callback_ != nullptr)
    {
        update_design_area_callback_(new_area);
    }
}
void
DatabaseHandler::notifyDesignAreaChanged()
{
    if (update_design_area_callback_ != nullptr)
    {
        update_design_area_callback_(area());
    }
}
bool
DatabaseHandler::hasMaximumArea() const
{
    if (maximum_area_callback_ != nullptr)
    {
        return maximum_area_callback_();
    }
    return maximum_area_valid_;
}

float
DatabaseHandler::maximumArea() const
{
    if (!maximum_area_valid_)
    {
        PSN_LOG_WARN("Maximum area is not set or invalid");
    }
    if (maximum_area_callback_)
    {
        return maximum_area_callback_();
    }
    return maximum_area_;
}

float
DatabaseHandler::gateDelay(Instance* inst, InstanceTerm* to, float in_slew,
                           LibraryTerm* from, float* drvr_slew, int rise_fall)
{
    return gateDelay(libraryCell(inst), to, in_slew, from, drvr_slew,
                     rise_fall);
}

float
DatabaseHandler::gateDelay(LibraryCell* lib_cell, InstanceTerm* to,
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
                        in_slew =
                            target_slews_[arc->toTrans()->asRiseFall()->index()];
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
DatabaseHandler::findTargetLoads(Liberty* library, sta::Slew slews[])
{
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext())
    {
        auto cell = cell_iter.next();
        findTargetLoad(cell, slews);
    }
}

void
DatabaseHandler::findTargetLoad(LibraryCell* cell, sta::Slew slews[])
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
DatabaseHandler::findTargetLoad(LibraryCell* cell, sta::TimingArc* arc,
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
DatabaseHandler::clockNets() const
{
    std::set<Net*>            nets;
    sta::ClkArrivalSearchPred srch_pred(sta_);
    sta::BfsFwdIterator       bfs(sta::BfsIndex::other, &srch_pred, sta_);
    sta::PinSet               clk_pins;
    sta_->search()->findClkVertexPins(clk_pins);
    for (auto pin : clk_pins)
    {
        Vertex *vert, *bi_vert;
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
DatabaseHandler::slewLimit(InstanceTerm* pin, sta::MinMax* min_max,
                           // Return values.
                           float& limit, bool& exists) const

{
    exists = false;
    float pin_limit;
    bool  pin_limit_exists;
    sta_->sdc()->slewLimit(network()->cell(network()->topInstance()), min_max,
                           pin_limit, pin_limit_exists);

    exists = pin_limit_exists;
    limit  = pin_limit;
    if (network()->isTopLevelPort(pin))
    {
        Port* port = network()->port(pin);
        sta_->sdc()->slewLimit(port, min_max, pin_limit, pin_limit_exists);
        if (pin_limit_exists && (!exists || min_max->compare(limit, pin_limit)))
        {
            limit  = pin_limit;
            exists = true;
        }
    }
    else
    {
        auto port = network()->libertyPort(pin);

        if (port)
        {

            port->slewLimit(min_max, pin_limit, pin_limit_exists);
            if (pin_limit_exists)
            {
                if (!exists || min_max->compare(limit, pin_limit))
                {
                    limit  = pin_limit;
                    exists = true;
                }
            }
            else if (port->direction()->isAnyOutput() &&
                     min_max == sta::MinMax::max())
            {
                port->libertyLibrary()->defaultMaxSlew(pin_limit,
                                                       pin_limit_exists);

                if (pin_limit_exists &&
                    (!exists || min_max->compare(limit, pin_limit)))
                {
                    limit  = pin_limit;
                    exists = true;
                }
            }
        }
    }
}

float
DatabaseHandler::gateDelay(InstanceTerm* pin, float load_cap, float* tr_slew)
{
    return gateDelay(libraryPin(pin), load_cap, tr_slew);
}

float
DatabaseHandler::gateDelay(LibraryTerm* out_port, float load_cap,
                           float* tr_slew)
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
DatabaseHandler::bufferDelay(psn::LibraryCell* buffer_cell, float load_cap)
{
    psn::LibraryTerm *input, *output;
    buffer_cell->bufferPorts(input, output);
    return gateDelay(output, load_cap);
}

float
DatabaseHandler::portCapacitance(const LibraryTerm* port, bool isMax) const
{
    float cap1 = port->capacitance(
        sta::RiseFall::rise(), isMax ? sta::MinMax::max() : sta::MinMax::min());
    float cap2 = port->capacitance(
        sta::RiseFall::fall(), isMax ? sta::MinMax::max() : sta::MinMax::min());
    return std::max(cap1, cap2);
}

LibraryTerm*
DatabaseHandler::bufferInputPin(LibraryCell* buffer_cell) const
{
    LibraryTerm *input, *output;
    buffer_cell->bufferPorts(input, output);
    return input;
}
LibraryTerm*
DatabaseHandler::bufferOutputPin(LibraryCell* buffer_cell) const
{
    LibraryTerm *input, *output;
    buffer_cell->bufferPorts(input, output);
    return output;
}

float
DatabaseHandler::inverterInputCapacitance(LibraryCell* inv_cell)
{
    LibraryTerm *input, *output;
    inv_cell->bufferPorts(input, output);
    return portCapacitance(input);
}
float
DatabaseHandler::bufferInputCapacitance(LibraryCell* buffer_cell) const
{
    LibraryTerm *input, *output;
    buffer_cell->bufferPorts(input, output);
    return portCapacitance(input);
}
float
DatabaseHandler::bufferOutputCapacitance(LibraryCell* buffer_cell)
{
    LibraryTerm *input, *output;
    buffer_cell->bufferPorts(input, output);
    return portCapacitance(input);
}
////////////////////////////////////////////////////////////////

sta::Slew
DatabaseHandler::targetSlew(const sta::RiseFall* rf)
{
    return target_slews_[rf->index()];
}

// Find target slew across all buffers in the libraries.
void
DatabaseHandler::findBufferTargetSlews(std::vector<Liberty*>* resize_libs)
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
DatabaseHandler::dontUse(LibraryCell* cell) const
{
    return cell->dontUse() || dont_use_.count(cell) ||
           (dont_use_callback_ != nullptr && dont_use_callback_(cell));
}
bool
DatabaseHandler::dontTouch(Instance* inst) const
{
    odb::dbInst* db_inst = network()->staToDb(inst);
    return db_inst && db_inst->isDoNotTouch();
}
bool
DatabaseHandler::dontSize(Instance* inst) const
{
    odb::dbInst* db_inst = network()->staToDb(inst);
    return db_inst && db_inst->isDoNotSize();
}
void
DatabaseHandler::resetDelays()
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
DatabaseHandler::resetDelays(InstanceTerm* term)
{
    sta_->delaysInvalidFrom(term);
    sta_->delaysInvalidFromFanin(term);
}
void
DatabaseHandler::findBufferTargetSlews(Liberty* library,
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
DatabaseHandler::calculateParasitics()
{
    for (auto& net : nets())
    {
        if (!isClock(net) && !network()->isPower(net) &&
            !network()->isGround(net))
        {
            calculateParasitics(net);
        }
    }
}
bool
DatabaseHandler::isClock(Net* net) const
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
DatabaseHandler::calculateParasitics(Net* net)
{
    if (compute_parasitics_callback_ != nullptr)
    {
        compute_parasitics_callback_(net);
    }
}

sta::ParasiticNode*
DatabaseHandler::findParasiticNode(std::unique_ptr<SteinerTree>& tree,
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
DatabaseHandler::handlerType() const
{
    return HandlerType::OPENSTA;
}
} // namespace psn
