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

#include "OpenPhySyn/BufferTree.hpp"
#include "OpenPhySyn/DatabaseHandler.hpp"
#include "OpenPhySyn/LibraryMapping.hpp"
#include "OpenPhySyn/Psn.hpp"
#include "OpenPhySyn/PsnGlobal.hpp"
#include "OpenPhySyn/PsnLogger.hpp"
#include "OpenPhySyn/SteinerTree.hpp"
#include "OpenPhySyn/Types.hpp"

#include <memory>

namespace psn
{

BufferTree::BufferTree(float cap, float req, float cost, Point location,
                       LibraryTerm* library_pin, InstanceTerm* pin,
                       LibraryCell* buffer_cell, int polarity,
                       BufferMode buffer_mode)
    : capacitance_(cap),
      required_or_slew_(req),
      wire_capacitance_(0.0),
      wire_delay_or_slew_(0.0),
      cost_(cost),
      location_(location),
      left_(nullptr),
      right_(nullptr),
      buffer_cell_(buffer_cell),
      pin_(pin),
      library_pin_(library_pin),
      upstream_buffer_cell_(buffer_cell),
      driver_cell_(nullptr),
      polarity_(polarity),
      buffer_count_(0),
      mode_(buffer_mode),
      library_mapping_node_(nullptr),
      driver_location_(0, 0)

{
}
BufferTree::BufferTree(Psn* psn_inst, std::shared_ptr<BufferTree> left,
                       std::shared_ptr<BufferTree> right, Point location)
    : capacitance_(left->totalCapacitance() + right->totalCapacitance()),
      required_or_slew_(left->mode() == Timerless
                            ? (std::max(left->totalRequiredOrSlew(),
                                        right->totalRequiredOrSlew()))
                            : std::min(left->totalRequiredOrSlew(),
                                       right->totalRequiredOrSlew())),
      wire_capacitance_(0.0),
      wire_delay_or_slew_(0.0),
      cost_(left->cost() + right->cost()),
      location_(location),
      left_(left),
      right_(right),
      buffer_cell_(nullptr),
      pin_(),
      library_pin_(left->libraryPin()),
      upstream_buffer_cell_(nullptr),
      driver_cell_(nullptr),
      polarity_(0),
      buffer_count_(left->bufferCount() + right->bufferCount()),
      mode_(left->mode()),
      library_mapping_node_(nullptr),
      driver_location_(0, 0)

{
    required_or_slew_ =
        (isTimerless() ? (std::max(left->totalRequiredOrSlew(),
                                   right->totalRequiredOrSlew()))
                       : std::min(left->totalRequiredOrSlew(),
                                  right->totalRequiredOrSlew()));
    if (left->hasUpstreamBufferCell())
    {
        if (right->hasUpstreamBufferCell())
        {
            auto left_cell  = left->upstreamBufferCell();
            auto right_cell = right->upstreamBufferCell();
            if (bufferRequired(psn_inst, left_cell) <=
                bufferRequired(psn_inst, right_cell))
            {
                upstream_buffer_cell_ = left->upstreamBufferCell();
            }
            else
            {
                upstream_buffer_cell_ = right->upstreamBufferCell();
            }
        }
        else
        {
            upstream_buffer_cell_ = left->upstreamBufferCell();
        }
    }
    else if (right->hasUpstreamBufferCell())
    {
        upstream_buffer_cell_ = right->upstreamBufferCell();
    }
}
float
BufferTree::totalCapacitance() const
{
    return capacitance() + wireCapacitance();
}
float
BufferTree::capacitance() const
{
    return capacitance_;
}
float
BufferTree::requiredOrSlew() const
{
    return required_or_slew_;
}
float
BufferTree::totalRequiredOrSlew() const
{
    return isTimerless() ? required_or_slew_ + wire_delay_or_slew_
                         : required_or_slew_ - wire_delay_or_slew_;
}
float
BufferTree::wireCapacitance() const
{
    return wire_capacitance_;
}
float
BufferTree::wireDelayOrSlew() const
{
    return wire_delay_or_slew_;
}
float
BufferTree::cost() const
{
    return cost_;
}
int
BufferTree::polarity() const
{
    return polarity_;
}
InstanceTerm*
BufferTree::pin() const
{
    return pin_;
}
bool
BufferTree::checkLimits(Psn* psn_inst, LibraryTerm* driver_pin,
                        float slew_limit, float cap_limit)
{
    if (totalCapacitance() == 0 || totalRequiredOrSlew() == 0)
    {
        return true;
    }
    DatabaseHandler& handler = *(psn_inst->handler());

    bool left_check =
        left() == nullptr ||
        left()->checkLimits(
            psn_inst,
            hasBufferCell() ? handler.bufferOutputPin(bufferCell()) : nullptr,
            slew_limit, cap_limit);
    bool right_check =
        right() == nullptr ||
        right()->checkLimits(
            psn_inst,
            hasBufferCell() ? handler.bufferOutputPin(bufferCell()) : nullptr,
            slew_limit, cap_limit);
    bool self_check =
        totalCapacitance() < cap_limit &&
        (driver_pin == nullptr ||
         handler.slew(driver_pin, totalCapacitance()) < slew_limit);
    return left_check && self_check && right_check;
}

void
BufferTree::setDriverLocation(Point loc)
{
    driver_location_ = loc;
}

Point
BufferTree::driverLocation() const
{
    return driver_location_;
}

void
BufferTree::setLibraryMappingNode(std::shared_ptr<LibraryCellMappingNode> node)
{
    library_mapping_node_ = node;
}

std::shared_ptr<LibraryCellMappingNode>
BufferTree::libraryMappingNode() const
{
    return library_mapping_node_;
}

void
BufferTree::setCapacitance(float cap)
{
    capacitance_ = cap;
}
void
BufferTree::setRequiredOrSlew(float req)
{
    required_or_slew_ = req;
}
void
BufferTree::setWireCapacitance(float cap)
{
    wire_capacitance_ = cap;
}
void
BufferTree::setWireDelayOrSlew(float delay)
{
    wire_delay_or_slew_ = delay;
}
void
BufferTree::setCost(float cost)
{
    cost_ = cost;
}
void
BufferTree::setPolarity(int polarity)
{
    polarity_ = polarity;
}
void
BufferTree::setPin(InstanceTerm* pin)
{
    pin_ = pin;
}
void
BufferTree::setLibraryPin(LibraryTerm* library_pin)
{
    library_pin_ = library_pin;
}

void
BufferTree::setMode(BufferMode buffer_mode)
{
    mode_ = buffer_mode;
}

BufferMode
BufferTree::mode() const
{
    return mode_;
}

float
BufferTree::bufferSlew(Psn* psn_inst, LibraryCell* buffer_cell, float tr_slew)
{
    return psn_inst->handler()->slew(
        psn_inst->handler()->bufferOutputPin(buffer_cell), totalCapacitance(),
        &tr_slew);
}
float
BufferTree::bufferFixedInputSlew(Psn* psn_inst, LibraryCell* buffer_cell)
{
    return psn_inst->handler()->bufferFixedInputSlew(buffer_cell,
                                                     totalCapacitance());
}

float
BufferTree::bufferRequired(Psn* psn_inst, LibraryCell* buffer_cell) const
{
    return totalRequiredOrSlew() -
           psn_inst->handler()->bufferDelay(buffer_cell, totalCapacitance());
}
float
BufferTree::bufferRequired(Psn* psn_inst) const
{
    return bufferRequired(psn_inst, buffer_cell_);
}
float
BufferTree::upstreamBufferRequired(Psn* psn_inst) const
{
    return bufferRequired(psn_inst, upstream_buffer_cell_);
}

bool
BufferTree::hasDownstreamSlewViolation(Psn* psn_inst, float slew_limit,
                                       float tr_slew)
{
    if (isBufferNode())
    {
        // if (tr_slew)
        // {
        //     tr_slew = bufferSlew(psn_inst, buffer_cell_, tr_slew);
        // }
        // else
        // {
        //     tr_slew = bufferFixedInputSlew(psn_inst, buffer_cell_);
        // }
        tr_slew = bufferSlew(psn_inst, buffer_cell_, slew_limit);
        if (tr_slew > slew_limit)
        {
            return true;
        }
    }
    tr_slew += totalRequiredOrSlew();
    if (left_ &&
        left_->hasDownstreamSlewViolation(psn_inst, slew_limit, tr_slew))
    {
        return true;
    }
    if (right_ &&
        right_->hasDownstreamSlewViolation(psn_inst, slew_limit, tr_slew))
    {
        return true;
    }

    return false;
}

LibraryTerm*
BufferTree::libraryPin() const
{
    return library_pin_;
}
std::shared_ptr<BufferTree>&
BufferTree::left()
{
    return left_;
}
std::shared_ptr<BufferTree>&
BufferTree::right()
{
    return right_;
}
void
BufferTree::setLeft(std::shared_ptr<BufferTree> left)
{
    left_ = left;
}
void
BufferTree::setRight(std::shared_ptr<BufferTree> right)
{
    right_ = right;
}
bool
BufferTree::hasUpstreamBufferCell() const
{
    return upstream_buffer_cell_ != nullptr;
}
bool
BufferTree::hasBufferCell() const
{
    return buffer_cell_ != nullptr;
}
bool
BufferTree::hasDriverCell() const
{
    return driver_cell_ != nullptr;
}

LibraryCell*
BufferTree::bufferCell() const
{
    return buffer_cell_;
}
LibraryCell*
BufferTree::upstreamBufferCell() const
{
    return upstream_buffer_cell_;
}
LibraryCell*
BufferTree::driverCell() const
{
    return driver_cell_;
}
void
BufferTree::setBufferCell(LibraryCell* buffer_cell)
{
    buffer_cell_          = buffer_cell;
    upstream_buffer_cell_ = buffer_cell;
}
void
BufferTree::setUpstreamBufferCell(LibraryCell* buffer_cell)
{
    upstream_buffer_cell_ = buffer_cell;
}
void
BufferTree::setDriverCell(LibraryCell* driver_cell)
{
    driver_cell_ = driver_cell;
}

Point
BufferTree::location() const
{
    return location_;
}

bool
BufferTree::isBufferNode() const
{
    return buffer_cell_ != nullptr;
}
bool
BufferTree::isLoadNode() const
{
    return !isBranched() && !isBufferNode();
}
bool
BufferTree::isBranched() const
{
    return left_ != nullptr && right_ != nullptr;
}
bool
BufferTree::operator<(const BufferTree& other) const
{
    return (required_or_slew_ - wire_delay_or_slew_) <
           (other.required_or_slew_ - other.wire_delay_or_slew_);
}
bool
BufferTree::operator>(const BufferTree& other) const
{
    return (required_or_slew_ - wire_delay_or_slew_) >
           (other.required_or_slew_ - other.wire_delay_or_slew_);
}
bool
BufferTree::operator<=(const BufferTree& other) const
{
    return (required_or_slew_ - wire_delay_or_slew_) <=
           (other.required_or_slew_ - other.wire_delay_or_slew_);
}
bool
BufferTree::operator>=(const BufferTree& other) const
{
    return (required_or_slew_ - wire_delay_or_slew_) >=
           (other.required_or_slew_ - other.wire_delay_or_slew_);
}
void
BufferTree::setBufferCount(int buffer_count)
{
    buffer_count_ = buffer_count;
}
int
BufferTree::bufferCount() const
{
    return buffer_count_;
}
int
BufferTree::count()
{
    buffer_count_ = (buffer_cell_ != nullptr) + (left_ ? left_->count() : 0) +
                    (right_ ? right_->count() : 0);
    return bufferCount();
}
int
BufferTree::branchCount() const
{
    return (left_ ? (1 + left_->branchCount()) : 0) +
           (right_ ? (1 + right_->branchCount()) : 0);
}
void
BufferTree::logInfo() const
{
    PSN_LOG_INFO("Buffers", bufferCount(), "Cap.", capacitance(), "(+W",
                 totalCapacitance(), ") Req./Slew", requiredOrSlew(), "(+W",
                 totalRequiredOrSlew(), ") Cost", cost(), "Left?",
                 left_ != nullptr, "Right?", right_ != nullptr, "Branches",
                 branchCount());
}
void
BufferTree::logDebug() const
{
    PSN_LOG_DEBUG("Buffers", bufferCount(), "Cap.", capacitance(), "(+W",
                  totalCapacitance(), ") Req./Slew", requiredOrSlew(), "(+W",
                  totalRequiredOrSlew(), ") Cost", cost(), "Left?",
                  left_ != nullptr, "Right?", right_ != nullptr, "Branches",
                  branchCount());
}
bool
BufferTree::isTimerless() const
{
    return mode_ == BufferMode::Timerless;
}

TimerlessBufferTree::TimerlessBufferTree(float cap, float req, float cost,
                                         Point         location,
                                         LibraryTerm*  library_pin,
                                         InstanceTerm* pin,
                                         LibraryCell* buffer_cell, int polarity)
    : BufferTree(cap, req, cost, location, library_pin, pin, buffer_cell,
                 polarity, BufferMode::Timerless)
{
}
TimerlessBufferTree::TimerlessBufferTree(Psn*                        psn_inst,
                                         std::shared_ptr<BufferTree> left,
                                         std::shared_ptr<BufferTree> right,
                                         Point                       location)
    : BufferTree(psn_inst, left, right, location)
{
    setMode(BufferMode::Timerless);
}

BufferSolution::BufferSolution(BufferMode buffer_mode) : mode_(buffer_mode){};
BufferSolution::BufferSolution(Psn*                             psn_inst,
                               std::shared_ptr<BufferSolution>& left,
                               std::shared_ptr<BufferSolution>& right,
                               Point location, LibraryCell* upstream_res_cell,
                               float      minimum_upstream_res_or_max_slew,
                               BufferMode buffer_mode)
    : mode_(buffer_mode)

{
    mergeBranches(psn_inst, left, right, location, upstream_res_cell,
                  minimum_upstream_res_or_max_slew);
}
void
BufferSolution::mergeBranches(Psn*                             psn_inst,
                              std::shared_ptr<BufferSolution>& left,
                              std::shared_ptr<BufferSolution>& right,
                              Point location, LibraryCell* upstream_res_cell,
                              float minimum_upstream_res_or_max_slew)
{
    buffer_trees_.resize(left->bufferTrees().size() *
                             right->bufferTrees().size(),
                         std::make_shared<BufferTree>());
    int index = 0;
    for (auto& left_branch : left->bufferTrees())
    {
        for (auto& right_branch : right->bufferTrees())
        {
            if (left_branch->polarity() == right_branch->polarity())
            {
                buffer_trees_[index++] =
                    isTimerless()
                        ? std::make_shared<TimerlessBufferTree>(
                              psn_inst, left_branch, right_branch, location)
                        : std::make_shared<BufferTree>(psn_inst, left_branch,
                                                       right_branch, location);
            }
        }
    }
    buffer_trees_.resize(index);
    prune(psn_inst, upstream_res_cell, minimum_upstream_res_or_max_slew);
}
void
BufferSolution::addTree(std::shared_ptr<BufferTree>& tree)
{
    buffer_trees_.push_back(tree);
}
std::vector<std::shared_ptr<BufferTree>>&
BufferSolution::bufferTrees()
{
    return buffer_trees_;
}
void
BufferSolution::addWireDelayAndCapacitance(float wire_res, float wire_cap)
{
    float wire_delay = wire_res * wire_cap;
    for (auto& tree : buffer_trees_)
    {
        tree->setWireDelayOrSlew(wire_delay);
        tree->setWireCapacitance(wire_cap);
    }
}
void
BufferSolution::addWireSlewAndCapacitance(float wire_res, float wire_cap)
{
    for (auto& tree : buffer_trees_)
    {
        float slew_degrade = // std::log(9)
            wire_res * ((wire_cap / 2.0) + tree->totalCapacitance());
        float wire_slew = std::log(9) * slew_degrade;
        tree->setWireDelayOrSlew(wire_slew);
        tree->setWireCapacitance(wire_cap);
    }
}

void
BufferSolution::addLeafTrees(Psn* psn_inst, InstanceTerm*, Point pt,
                             std::vector<LibraryCell*>& buffer_lib,
                             std::vector<LibraryCell*>& inverter_lib,
                             float                      slew_limit)
{
    if (!buffer_trees_.size())
    {
        return;
    }
    if (!isTimerless())
    {

        for (auto& buff : buffer_lib)
        {
            auto optimal_tree  = *(buffer_trees_.begin());
            auto buff_required = optimal_tree->bufferRequired(psn_inst, buff);
            for (auto& tree : buffer_trees_)
            {
                auto req = tree->bufferRequired(psn_inst, buff);
                if (req > buff_required)
                {
                    optimal_tree  = tree;
                    buff_required = req;
                }
            }
            auto buffer_cost = psn_inst->handler()->area(buff);
            auto buffer_cap = psn_inst->handler()->bufferInputCapacitance(buff);
            auto buffer_opt = std::make_shared<BufferTree>(
                buffer_cap, buff_required, optimal_tree->cost() + buffer_cost,
                pt, nullptr, nullptr, buff);
            buffer_opt->setBufferCount(optimal_tree->bufferCount() + 1);
            buffer_opt->setLeft(optimal_tree);
            buffer_trees_.push_back(buffer_opt);
        }
        for (auto& inv : inverter_lib)
        {
            auto optimal_tree  = *(buffer_trees_.begin());
            auto buff_required = optimal_tree->bufferRequired(psn_inst, inv);
            for (auto& tree : buffer_trees_)
            {
                auto req = tree->bufferRequired(psn_inst, inv);
                if (req > buff_required)
                {
                    optimal_tree  = tree;
                    buff_required = req;
                }
            }
            auto buffer_cost = psn_inst->handler()->area(inv);
            auto buffer_cap =
                psn_inst->handler()->inverterInputCapacitance(inv);
            auto buffer_opt = std::make_shared<BufferTree>(
                buffer_cap, buff_required, optimal_tree->cost() + buffer_cost,
                pt, nullptr, nullptr, inv);

            buffer_opt->setPolarity(!optimal_tree->polarity());
            buffer_opt->setBufferCount(optimal_tree->bufferCount() + 1);

            buffer_opt->setLeft(optimal_tree);
            buffer_trees_.push_back(buffer_opt);
        }
    }
    else
    {
        std::sort(buffer_trees_.begin(), buffer_trees_.end(),
                  [&](const std::shared_ptr<BufferTree>& a,
                      const std::shared_ptr<BufferTree>& b) -> bool {
                      return a->cost() < b->cost();
                  });
        // auto sol_tree = buffer_trees_[0];
        std::vector<std::shared_ptr<BufferTree>> new_trees;
        for (auto& buff : buffer_lib)
        {
            for (auto& sol_tree : buffer_trees_)
            {
                auto buffer_cost = psn_inst->handler()->area(buff);
                auto buffer_cap =
                    psn_inst->handler()->bufferInputCapacitance(buff);
                float buffer_slew =
                    sol_tree->bufferSlew(psn_inst, buff, slew_limit);
                if (buffer_slew < slew_limit)
                {
                    auto buffer_opt = std::make_shared<TimerlessBufferTree>(
                        buffer_cap, 0, sol_tree->cost() + buffer_cost, pt,
                        nullptr, nullptr, buff);
                    buffer_opt->setBufferCount(sol_tree->bufferCount() + 1);
                    buffer_opt->setLeft(sol_tree);
                    new_trees.push_back(buffer_opt);
                    break;
                }
            }
        }
        for (auto& inv : inverter_lib)
        {
            for (auto& sol_tree : buffer_trees_)
            {
                auto buffer_cost = psn_inst->handler()->area(inv);
                auto buffer_cap =
                    psn_inst->handler()->bufferInputCapacitance(inv);
                float buffer_slew = sol_tree->bufferSlew(psn_inst, inv);
                if (buffer_slew < slew_limit)
                {
                    auto buffer_opt = std::make_shared<TimerlessBufferTree>(
                        buffer_cap, 0, sol_tree->cost() + buffer_cost, pt,
                        nullptr, nullptr, inv);
                    buffer_opt->setBufferCount(sol_tree->bufferCount() + 1);
                    buffer_opt->setLeft(sol_tree);
                    buffer_opt->setPolarity(!sol_tree->polarity());
                    new_trees.push_back(buffer_opt);
                    break;
                }
            }
        }

        buffer_trees_.insert(buffer_trees_.end(), new_trees.begin(),
                             new_trees.end());
        buffer_trees_.erase(
            std::remove_if(
                buffer_trees_.begin(), buffer_trees_.end(),
                [&](const std::shared_ptr<BufferTree>& t) -> bool {
                    if ((t->isBufferNode() &&
                         std::sqrt(std::pow(t->totalRequiredOrSlew(), 2) +
                                   std::pow(psn_inst->handler()->bufferDelay(
                                                t->bufferCell(),
                                                t->totalCapacitance()),
                                            2)) > slew_limit) ||
                        (!t->isBufferNode() &&
                         t->totalRequiredOrSlew() > slew_limit))
                    {
                        return true;
                    }
                    if (t->hasDownstreamSlewViolation(psn_inst, slew_limit))
                    {
                        return true;
                    }
                    return false;
                }),
            buffer_trees_.end());
    }
}
void
BufferSolution::addLeafTreesWithResynthesis(
    Psn* psn_inst, InstanceTerm*, Point pt,
    std::vector<LibraryCell*>&                            buffer_lib,
    std::vector<LibraryCell*>&                            inverter_lib,
    std::vector<std::shared_ptr<LibraryCellMappingNode>>& mappings_terminals)
{
    if (!buffer_trees_.size())
    {
        return;
    }
    DatabaseHandler& handler = *(psn_inst->handler());

    for (auto& buff : buffer_lib)
    {
        auto optimal_tree  = *(buffer_trees_.begin());
        auto buff_required = optimal_tree->bufferRequired(psn_inst, buff);
        for (auto& tree : buffer_trees_)
        {
            auto req = tree->bufferRequired(psn_inst, buff);
            if (req > buff_required)
            {
                optimal_tree  = tree;
                buff_required = req;
            }
        }
        auto buffer_cost = psn_inst->handler()->area(buff);
        auto buffer_cap  = psn_inst->handler()->bufferInputCapacitance(buff);
        auto buffer_opt  = std::make_shared<BufferTree>(
            buffer_cap, buff_required, optimal_tree->cost() + buffer_cost, pt,
            nullptr, nullptr, buff);
        buffer_opt->setBufferCount(optimal_tree->bufferCount() + 1);
        buffer_opt->setLeft(optimal_tree);
        buffer_trees_.push_back(buffer_opt);
    }
    for (auto& inv : inverter_lib)
    {
        auto optimal_tree  = *(buffer_trees_.begin());
        auto buff_required = optimal_tree->bufferRequired(psn_inst, inv);
        for (auto& tree : buffer_trees_)
        {
            auto req = tree->bufferRequired(psn_inst, inv);
            if (req > buff_required)
            {
                optimal_tree  = tree;
                buff_required = req;
            }
        }
        if (optimal_tree &&
            (optimal_tree->polarity() == 0 || optimal_tree->polarity() == 1))
        {
            auto buffer_cost = psn_inst->handler()->area(inv);
            auto buffer_cap =
                psn_inst->handler()->inverterInputCapacitance(inv);
            auto buffer_opt = std::make_shared<BufferTree>(
                buffer_cap, buff_required, optimal_tree->cost() + buffer_cost,
                pt, nullptr, nullptr, inv);

            buffer_opt->setPolarity(!optimal_tree->polarity());
            buffer_opt->setBufferCount(optimal_tree->bufferCount() + 1);

            buffer_opt->setLeft(optimal_tree);
            buffer_trees_.push_back(buffer_opt);
        }
    }
    for (auto& term : mappings_terminals)
    {
        auto cells      = handler.truthTableToCells(term->id());
        bool is_buff    = term->isBuffer();
        bool is_inv     = term->isInverter();
        bool has_parent = term->parent() != nullptr;
        if (has_parent)
        {
            if (is_buff)
            {
                for (auto& buff : buffer_lib)
                {
                    auto optimal_tree = *(buffer_trees_.begin());
                    auto buff_required =
                        optimal_tree->bufferRequired(psn_inst, buff);
                    for (auto& tree : buffer_trees_)
                    {
                        auto req = tree->bufferRequired(psn_inst, buff);
                        if (req > buff_required)
                        {
                            optimal_tree  = tree;
                            buff_required = req;
                        }
                    }
                    auto buffer_cost = psn_inst->handler()->area(buff);
                    auto buffer_cap =
                        psn_inst->handler()->bufferInputCapacitance(buff);
                    auto buffer_opt = std::make_shared<BufferTree>(
                        buffer_cap, buff_required,
                        optimal_tree->cost() + buffer_cost, pt, nullptr,
                        nullptr, buff);
                    buffer_opt->setBufferCount(optimal_tree->bufferCount() + 1);
                    buffer_opt->setLeft(optimal_tree);
                    buffer_opt->setLibraryMappingNode(term);
                    buffer_trees_.push_back(buffer_opt);
                }
            }
            else if (is_inv)
            {
                for (auto& inv : inverter_lib)
                {
                    auto optimal_tree = *(buffer_trees_.begin());
                    auto buff_required =
                        optimal_tree->bufferRequired(psn_inst, inv);
                    for (auto& tree : buffer_trees_)
                    {
                        auto req = tree->bufferRequired(psn_inst, inv);
                        if (req > buff_required)
                        {
                            optimal_tree  = tree;
                            buff_required = req;
                        }
                    }
                    if (optimal_tree && (optimal_tree->polarity() == 0 ||
                                         optimal_tree->polarity() == 1))
                    {
                        auto buffer_cost = psn_inst->handler()->area(inv);
                        auto buffer_cap =
                            psn_inst->handler()->inverterInputCapacitance(inv);
                        auto buffer_opt = std::make_shared<BufferTree>(
                            buffer_cap, buff_required,
                            optimal_tree->cost() + buffer_cost, pt, nullptr,
                            nullptr, inv);

                        buffer_opt->setPolarity(!optimal_tree->polarity());
                        buffer_opt->setBufferCount(optimal_tree->bufferCount() +
                                                   1);

                        buffer_opt->setLeft(optimal_tree);
                        buffer_opt->setLibraryMappingNode(term);
                        buffer_trees_.push_back(buffer_opt);
                    }
                }
            }
        }
    }
}
void
BufferSolution::addUpstreamReferences(
    Psn* psn_inst, std::shared_ptr<BufferTree> base_buffer_tree)
{
    return; // Not used anymore..
    for (auto& tree : bufferTrees())
    {
        if (tree != base_buffer_tree)
        {
            if (!base_buffer_tree->hasUpstreamBufferCell())
            {
                base_buffer_tree->setUpstreamBufferCell(tree->bufferCell());
            }
            else
            {
                if (tree->bufferRequired(psn_inst) <
                    base_buffer_tree->upstreamBufferRequired(psn_inst))
                {
                    base_buffer_tree->setUpstreamBufferCell(tree->bufferCell());
                }
            }
        }
    }
}

std::shared_ptr<BufferTree>
BufferSolution::optimalDriverTreeWithResize(
    Psn* psn_inst, InstanceTerm* driver_pin,
    std::vector<LibraryCell*> driver_types, float area_penalty)
{
    if (!buffer_trees_.size())
    {
        return nullptr;
    }
    float                       max_slack;
    std::shared_ptr<BufferTree> temp_tree;
    auto                        max_tree =
        optimalDriverTree(psn_inst, driver_pin, temp_tree, &max_slack);
    auto inst         = psn_inst->handler()->instance(driver_pin);
    auto original_lib = psn_inst->handler()->libraryCell(inst);
    max_tree->setDriverCell(original_lib);
    for (auto& drv_type : driver_types)
    {
        for (size_t i = 0; i < buffer_trees_.size(); i++)
        {
            auto& tree = buffer_trees_[i];
            if (tree->polarity())
            {
                continue;
            }
            auto drvr_pin = psn_inst->handler()->libraryOutputPins(drv_type)[0];
            float max_cap =
                psn_inst->handler()->largestInputCapacitance(drv_type);

            float penalty =
                psn_inst->handler()->bufferChainDelayPenalty(max_cap) +
                area_penalty * psn_inst->handler()->area(drv_type);

            float delay = psn_inst->handler()->gateDelay(
                drvr_pin, tree->totalCapacitance());
            float slack = tree->totalRequiredOrSlew() - delay - penalty;

            if (slack > max_slack)
            {
                max_slack = slack;
                max_tree  = tree;
                max_tree->setDriverCell(drv_type);
            }
        }
    }
    return max_tree;
}
std::shared_ptr<BufferTree>
BufferSolution::optimalDriverTreeWithResynthesis(Psn*          psn_inst,
                                                 InstanceTerm* driver_pin,
                                                 float         area_penalty,
                                                 float*        tree_slack)
{
    if (!buffer_trees_.size())
    {
        return nullptr;
    }
    DatabaseHandler& handler = *(psn_inst->handler());

    auto first_tree = buffer_trees_[0];
    std::sort(buffer_trees_.begin(), buffer_trees_.end(),
              [&](const std::shared_ptr<BufferTree>& a,
                  const std::shared_ptr<BufferTree>& b) -> bool {
                  float a_delay = psn_inst->handler()->gateDelay(
                      driver_pin, a->totalCapacitance());
                  float a_slack = a->totalRequiredOrSlew() - a_delay;
                  float b_delay = psn_inst->handler()->gateDelay(
                      driver_pin, b->totalCapacitance());
                  float b_slack = b->totalRequiredOrSlew() - b_delay;
                  return a_slack > b_slack ||
                         (isEqual(a_slack, b_slack, 1E-6F) &&
                          a->cost() < b->cost());
              });

    float                       max_slack     = -1E+30F;
    float                       max_cost      = -1E+30F;
    std::shared_ptr<BufferTree> max_tree      = nullptr;
    auto                        inst          = handler.instance(driver_pin);
    auto                        original_lib  = handler.libraryCell(inst);
    auto                        original_cost = handler.area(original_lib);
    auto                        original_libs_set =
        handler.truthTableToCells(handler.cellToTruthTable(original_lib));
    auto  original_libs = std::vector<LibraryCell*>(original_libs_set.begin(),
                                                   original_libs_set.end());
    float orig_max_cap  = handler.largestInputCapacitance(original_lib);
    float orig_penalty  = handler.bufferChainDelayPenalty(orig_max_cap) +
                         area_penalty * handler.area(original_lib);
    std::sort(original_libs.begin(), original_libs.end(),
              [&](LibraryCell* a, LibraryCell* b) -> bool {
                  return handler.area(a) > handler.area(b);
              });

    int position = -1;
    for (size_t i = 0; i < original_libs.size(); i++)
    {
        if (original_libs[i] == original_lib)
        {
            position = i;
            break;
        }
    }
    if (position == -1)
    {
        throw "wrong mapping";
    }
    for (auto& tree : buffer_trees_)
    {
        if (tree->libraryMappingNode())
        {
            auto parent = tree->libraryMappingNode()->parent();
            if (parent->parent())
            {
                continue;
            }
            auto   drivers_set = handler.truthTableToCells(parent->id());
            auto   drivers     = std::vector<LibraryCell*>(drivers_set.begin(),
                                                     drivers_set.end());
            size_t adjusted_position = std::max(0, position);
            adjusted_position = std::min(adjusted_position, drivers.size() - 1);
            auto driver_range = std::vector<LibraryCell*>({
                // drivers[(position - 1) % drivers.size()],
                drivers[(adjusted_position) % drivers.size()],
                // drivers[(position + 1) % drivers.size()],
            });
            for (auto& d_type : driver_range)
            {
                if (handler.isSingleOutputCombinational(d_type))
                {
                    auto  d_pin = handler.libraryOutputPins(d_type)[0];
                    float delay =
                        handler.gateDelay(d_pin, tree->totalCapacitance());
                    float slack = tree->totalRequiredOrSlew() - delay;
                    float cost =
                        tree->cost() + handler.area(d_type) - original_cost;
                    if (isGreater(slack, max_slack) ||
                        (isEqual(slack, max_slack) && cost < max_cost))
                    {
                        max_slack = slack;
                        max_tree  = tree;
                        max_cost  = tree->cost();
                        max_tree->setDriverCell(d_type);
                        if (tree_slack)
                        {
                            *tree_slack = max_slack;
                        }
                    }
                }
            }
        }
        if (tree->polarity())
        {
            continue;
        }
        float delay = handler.gateDelay(driver_pin, tree->totalCapacitance());
        float slack = tree->totalRequiredOrSlew() - delay - orig_penalty;

        if (isGreater(slack, max_slack))
        {
            max_slack = slack;
            max_tree  = tree;
            max_cost  = tree->cost();
            if (tree_slack)
            {
                *tree_slack = max_slack;
            }
        }
    }
    return max_tree;
}

std::shared_ptr<BufferTree>
BufferSolution::optimalTimerlessDriverTree(Psn*          psn_inst,
                                           InstanceTerm* driver_pin)
{
    if (!buffer_trees_.size() || !isTimerless())
    {
        return nullptr;
    }
    std::sort(buffer_trees_.begin(), buffer_trees_.end(),
              [&](const std::shared_ptr<BufferTree>& a,
                  const std::shared_ptr<BufferTree>& b) -> bool {
                  return a->cost() < b->cost();
              });
    float slew_limit = psn_inst->handler()->pinSlewLimit(driver_pin);
    for (auto& tr : buffer_trees_)
    {
        float in_slew = psn_inst->handler()->slew(
            psn_inst->handler()->libraryPin(driver_pin),
            tr->totalCapacitance());
        if (tr->bufferCount() &&
            isLess(tr->totalRequiredOrSlew(), slew_limit, 1E-6F) &&
            !tr->hasDownstreamSlewViolation(psn_inst, slew_limit, in_slew))
        {
            return tr;
        }
    }
    return buffer_trees_[0];
}
std::shared_ptr<BufferTree>
BufferSolution::optimalDriverTree(Psn* psn_inst, InstanceTerm* driver_pin,
                                  std::shared_ptr<BufferTree>& inverted_sol,
                                  float*                       tree_slack)
{
    if (!buffer_trees_.size())
    {
        return nullptr;
    }

    auto first_tree = buffer_trees_[0];
    std::sort(buffer_trees_.begin(), buffer_trees_.end(),
              [&](const std::shared_ptr<BufferTree>& a,
                  const std::shared_ptr<BufferTree>& b) -> bool {
                  float a_delay = psn_inst->handler()->gateDelay(
                      driver_pin, a->totalCapacitance());
                  float a_slack = a->totalRequiredOrSlew() - a_delay;
                  float b_delay = psn_inst->handler()->gateDelay(
                      driver_pin, b->totalCapacitance());
                  float b_slack = b->totalRequiredOrSlew() - b_delay;
                  return a_slack > b_slack ||
                         (isEqual(a_slack, b_slack, 1E-6F) &&
                          a->cost() < b->cost());
              });

    float                       max_slack = -1E+30F;
    std::shared_ptr<BufferTree> max_tree  = nullptr;
    for (auto& tree : buffer_trees_)
    {
        if (tree->polarity())
        {
            if (inverted_sol == nullptr)
            {
                inverted_sol = tree;
            }
            continue;
        }
        float delay = psn_inst->handler()->gateDelay(driver_pin,
                                                     tree->totalCapacitance());
        float slack = tree->totalRequiredOrSlew() - delay;

        if (isGreater(slack, max_slack))
        {
            max_slack = slack;
            max_tree  = tree;
            if (tree_slack)
            {
                *tree_slack = max_slack;
            }
            break; // Already sorted, no need to continue searching.
        }
    }
    return max_tree;
}
std::shared_ptr<BufferTree>
BufferSolution::optimalCapacitanceTree(
    Psn* psn_inst, InstanceTerm* driver_pin,
    std::shared_ptr<BufferTree>& inverted_sol, float cap_limit)
{
    if (!buffer_trees_.size())
    {
        return nullptr;
    }

    auto first_tree = buffer_trees_[0];
    std::sort(buffer_trees_.begin(), buffer_trees_.end(),
              [&](const std::shared_ptr<BufferTree>& a,
                  const std::shared_ptr<BufferTree>& b) -> bool {
                  return a->cost() < b->cost();
              });

    std::shared_ptr<BufferTree> max_tree = nullptr;
    for (auto& tree : buffer_trees_)
    {
        if (tree->totalCapacitance() < cap_limit)
        {
            max_tree = tree;
            break;
        }
    }
    return max_tree;
}
std::shared_ptr<BufferTree>
BufferSolution::optimalSlewTree(Psn* psn_inst, InstanceTerm* driver_pin,
                                std::shared_ptr<BufferTree>& inverted_sol,
                                float                        slew_limit)
{
    if (!buffer_trees_.size())
    {
        return nullptr;
    }
    DatabaseHandler& handler = *(psn_inst->handler());

    auto first_tree = buffer_trees_[0];
    std::sort(buffer_trees_.begin(), buffer_trees_.end(),
              [&](const std::shared_ptr<BufferTree>& a,
                  const std::shared_ptr<BufferTree>& b) -> bool {
                  return a->cost() < b->cost();
              });

    std::shared_ptr<BufferTree> max_tree = nullptr;
    for (auto& tree : buffer_trees_)
    {
        if (handler.slew(handler.libraryPin(driver_pin),
                         tree->totalCapacitance()) < slew_limit)
        {
            max_tree = tree;
            break;
        }
    }
    return max_tree;
}

std::shared_ptr<BufferTree>
BufferSolution::optimalCostTree(Psn* psn_inst, InstanceTerm* driver_pin,
                                std::shared_ptr<BufferTree>& inverted_sol,
                                float slew_limit, float cap_limit)
{
    if (!buffer_trees_.size())
    {
        return nullptr;
    }
    DatabaseHandler& handler = *(psn_inst->handler());

    auto first_tree = buffer_trees_[0];
    std::sort(buffer_trees_.begin(), buffer_trees_.end(),
              [&](const std::shared_ptr<BufferTree>& a,
                  const std::shared_ptr<BufferTree>& b) -> bool {
                  return a->cost() < b->cost();
              });

    std::shared_ptr<BufferTree> max_tree  = nullptr;
    float                       max_slack = -1E+30F;
    size_t                      i         = 0;
    std::shared_ptr<BufferTree> second_best;
    for (auto& tree : buffer_trees_)
    {
        if (tree->polarity())
        {
            if (inverted_sol == nullptr)
            {
                inverted_sol = tree;
            }
            continue;
        }

        if (tree->checkLimits(psn_inst, handler.libraryPin(driver_pin),
                              slew_limit, cap_limit))
        {
            float delay =
                handler.gateDelay(driver_pin, tree->totalCapacitance());
            float slack = tree->totalRequiredOrSlew() - delay;
            if (second_best == nullptr)
            {
                second_best = tree;
                max_slack   = slack;
            }
            else
            {
                if (slack > max_slack)
                {
                    max_tree = tree;
                }
                else
                {
                    max_tree = second_best;
                }
                break;
            }
        }
        i++;
    }
    return max_tree == nullptr ? second_best : max_tree;
}

bool
BufferSolution::isGreater(float first, float second, float threshold)
{
    return first > second &&
           !(std::abs(first - second) <
             threshold * std::max(std::abs(first), std::abs(second)));
}
bool
BufferSolution::isLess(float first, float second, float threshold)
{
    return first < second &&
           !(std::abs(first - second) <
             threshold * std::max(std::abs(first), std::abs(second)));
}
bool
BufferSolution::isEqual(float first, float second, float threshold)
{
    return std::abs(first - second) <
           threshold * std::max(std::abs(first), std::abs(second));
}
bool
BufferSolution::isLessOrEqual(float first, float second, float threshold)
{
    return first < second ||
           (std::abs(first - second) <
            threshold * std::max(std::abs(first), std::abs(second)));
}
bool
BufferSolution::isGreaterOrEqual(float first, float second, float threshold)
{
    return first > second ||
           (std::abs(first - second) <
            threshold * std::max(std::abs(first), std::abs(second)));
}

void
BufferSolution::prune(Psn* psn_inst, LibraryCell* upstream_res_cell,
                      float       minimum_upstream_res_or_max_slew,
                      const float cap_prune_threshold,
                      const float cost_prune_threshold)
{
    if (!isTimerless()) // Timing-driven
    {
        // TODO Add squeeze pruning
        if (!upstream_res_cell)
        {
            PSN_LOG_WARN("Pruning without upstream resistance");

            return;
        }

        std::sort(buffer_trees_.begin(), buffer_trees_.end(),
                  [&](const std::shared_ptr<BufferTree>& a,
                      const std::shared_ptr<BufferTree>& b) -> bool {
                      float left_req =
                          a->bufferRequired(psn_inst, upstream_res_cell);
                      float right_req =
                          b->bufferRequired(psn_inst, upstream_res_cell);
                      return left_req > right_req;
                  });

        size_t index = 0;
        for (size_t i = 0; i < buffer_trees_.size(); i++)
        {
            index = i + 1;
            for (size_t j = i + 1; j < buffer_trees_.size(); j++)
            {
                if (isLess(buffer_trees_[j]->totalCapacitance(),
                           buffer_trees_[i]->totalCapacitance(),
                           cap_prune_threshold) ||
                    isLess(buffer_trees_[j]->cost(), buffer_trees_[i]->cost(),
                           cost_prune_threshold))
                {
                    buffer_trees_[index++] = buffer_trees_[j];
                }
            }
            buffer_trees_.resize(index);
        }
        if (minimum_upstream_res_or_max_slew)
        {
            std::sort(buffer_trees_.begin(), buffer_trees_.end(),
                      [](const std::shared_ptr<BufferTree>& a,
                         const std::shared_ptr<BufferTree>& b) -> bool {
                          return a->totalRequiredOrSlew() <
                                 b->totalRequiredOrSlew();
                      });

            index = 0;
            for (size_t i = 0; i < buffer_trees_.size(); i++)
            {
                index = i + 1;
                for (size_t j = i + 1; j < buffer_trees_.size(); j++)
                {
                    if (isGreaterOrEqual(buffer_trees_[i]->totalCapacitance(),
                                         buffer_trees_[j]->totalCapacitance(),
                                         cap_prune_threshold) ||
                        !((buffer_trees_[j]->totalRequiredOrSlew() -
                           buffer_trees_[i]->totalRequiredOrSlew()) /
                              (buffer_trees_[j]->totalCapacitance() -
                               buffer_trees_[i]->totalCapacitance()) <
                          minimum_upstream_res_or_max_slew))
                    {
                        buffer_trees_[index++] = buffer_trees_[j];
                    }
                }
                buffer_trees_.resize(index);
            }
        }
    }
    else
    {
        buffer_trees_.erase(
            std::remove_if(buffer_trees_.begin(), buffer_trees_.end(),
                           [&](const std::shared_ptr<BufferTree>& t) -> bool {
                               return isGreaterOrEqual(
                                   t->totalRequiredOrSlew(),
                                   minimum_upstream_res_or_max_slew,
                                   cap_prune_threshold);
                           }),
            buffer_trees_.end());
        std::sort(buffer_trees_.begin(), buffer_trees_.end(),
                  [](const std::shared_ptr<BufferTree>& a,
                     const std::shared_ptr<BufferTree>& b) -> bool {
                      return a->cost() < b->cost();
                  });
        size_t index = 0;
        for (size_t i = 0; i < buffer_trees_.size(); i++)
        {
            index = i + 1;
            for (size_t j = i + 1; j < buffer_trees_.size(); j++)
            {
                if ((isLess(buffer_trees_[j]->totalRequiredOrSlew(),
                            buffer_trees_[i]->totalRequiredOrSlew(),
                            cap_prune_threshold) ||
                     isLess(buffer_trees_[j]->totalCapacitance(),
                            buffer_trees_[i]->totalCapacitance(),
                            cap_prune_threshold)))
                {
                    buffer_trees_[index++] = buffer_trees_[j];
                }
            }
            buffer_trees_.resize(index);
        }
    }
}
void
BufferSolution::setMode(BufferMode buffer_mode)
{
    mode_ = buffer_mode;
}

BufferMode
BufferSolution::mode() const
{
    return mode_;
}

bool
BufferSolution::isTimerless() const
{
    return mode_ == BufferMode::Timerless;
}

TimerlessBufferSolution::TimerlessBufferSolution()
    : BufferSolution(BufferMode::Timerless)
{
}
TimerlessBufferSolution::TimerlessBufferSolution(
    Psn* psn_inst, std::shared_ptr<BufferSolution>& left,
    std::shared_ptr<BufferSolution>& right, Point location,
    LibraryCell* upstream_res_cell, float minimum_upstream_res_or_max_slew,
    BufferMode buffer_mode)
    : BufferSolution(psn_inst, left, right, location, upstream_res_cell,
                     minimum_upstream_res_or_max_slew, BufferMode::Timerless)
{
}

std::shared_ptr<BufferSolution>
BufferSolution::bottomUp(Psn* psn_inst, InstanceTerm* driver_pin,
                         SteinerPoint pt, SteinerPoint prev,
                         std::shared_ptr<SteinerTree>          st_tree,
                         std::unique_ptr<OptimizationOptions>& options)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    if (pt != SteinerNull)
    {
        auto  pt_pin        = st_tree->pin(pt);
        float wire_length   = handler.dbuToMeters(st_tree->distance(prev, pt));
        float wire_res      = wire_length * handler.resistancePerMicron();
        float wire_cap      = wire_length * handler.capacitancePerMicron();
        auto  location      = st_tree->location(pt);
        auto  prev_location = st_tree->location(prev);
        PSN_LOG_DEBUG("Bottomup Point: (", location.getX(), ",",
                      location.getY(), ")");

        PSN_LOG_TRACE("Prev: (", prev_location.getX(), ",",
                      prev_location.getY(), ")");

        if (pt_pin && handler.isLoad(pt_pin))
        {
            PSN_LOG_TRACE(handler.name(pt_pin), "(", location.getX(), ",",
                          location.getY(), ") bottomUp leaf");

            float                       cap = handler.pinCapacitance(pt_pin);
            float                       req = handler.required(pt_pin);
            std::shared_ptr<BufferTree> base_buffer_tree =
                std::make_shared<BufferTree>(cap, req, 0, location,
                                             handler.libraryPin(driver_pin),
                                             pt_pin);
            std::shared_ptr<BufferSolution> buff_sol =
                std::make_shared<BufferSolution>();
            buff_sol->addTree(base_buffer_tree);

            buff_sol->addWireDelayAndCapacitance(wire_res, wire_cap);

            buff_sol->addLeafTrees(psn_inst, driver_pin, prev_location,
                                   options->buffer_lib, options->inverter_lib);
            buff_sol->addUpstreamReferences(psn_inst, base_buffer_tree);

            return buff_sol;
        }
        else if (!pt_pin)
        {
            PSN_LOG_TRACE("(", location.getX(), ",", location.getY(),
                          ") bottomUp ---> left");

            auto left = bottomUp(psn_inst, driver_pin, st_tree->left(pt), pt,
                                 st_tree, options);
            PSN_LOG_TRACE("(", location.getX(), ",", location.getY(),
                          ") bottomUp ---> right");

            auto right = bottomUp(psn_inst, driver_pin, st_tree->right(pt), pt,
                                  st_tree, options);

            PSN_LOG_TRACE("(", location.getX(), ",", location.getY(),
                          ") bottomUp merging");

            std::shared_ptr<BufferSolution> buff_sol =
                std::make_shared<BufferSolution>(
                    psn_inst, left, right, location,
                    options->buffer_lib[options->buffer_lib.size() / 2],
                    options->minimum_upstream_resistance);

            buff_sol->addWireDelayAndCapacitance(wire_res, wire_cap);
            buff_sol->addLeafTrees(psn_inst, driver_pin, prev_location,
                                   options->buffer_lib, options->inverter_lib);

            return buff_sol;
        }
    }
    return nullptr;
}

std::shared_ptr<BufferSolution>
BufferSolution::bottomUpWithResynthesis(
    Psn* psn_inst, InstanceTerm* driver_pin, SteinerPoint pt, SteinerPoint prev,
    std::shared_ptr<SteinerTree>                          st_tree,
    std::unique_ptr<OptimizationOptions>&                 options,
    std::vector<std::shared_ptr<LibraryCellMappingNode>>& mapping_terminals)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    if (pt != SteinerNull)
    {
        auto  pt_pin        = st_tree->pin(pt);
        float wire_length   = handler.dbuToMeters(st_tree->distance(prev, pt));
        float wire_res      = wire_length * handler.resistancePerMicron();
        float wire_cap      = wire_length * handler.capacitancePerMicron();
        auto  location      = st_tree->location(pt);
        auto  prev_location = st_tree->location(prev);
        PSN_LOG_DEBUG("Bottomup Point: (", location.getX(), ",",
                      location.getY(), ")");

        PSN_LOG_TRACE("Prev: (", prev_location.getX(), ",",
                      prev_location.getY(), ")");

        if (pt_pin && handler.isLoad(pt_pin))
        {
            PSN_LOG_TRACE(handler.name(pt_pin), "(", location.getX(), ",",
                          location.getY(), ") bottomUp leaf");

            float                       cap = handler.pinCapacitance(pt_pin);
            float                       req = handler.required(pt_pin);
            std::shared_ptr<BufferTree> base_buffer_tree =
                std::make_shared<BufferTree>(cap, req, 0, location,
                                             handler.libraryPin(driver_pin),
                                             pt_pin);
            std::shared_ptr<BufferSolution> buff_sol =
                std::make_shared<BufferSolution>();
            buff_sol->addTree(base_buffer_tree);

            buff_sol->addWireDelayAndCapacitance(wire_res, wire_cap);

            buff_sol->addLeafTreesWithResynthesis(
                psn_inst, driver_pin, prev_location, options->buffer_lib,
                options->inverter_lib, mapping_terminals);
            buff_sol->addUpstreamReferences(psn_inst, base_buffer_tree);

            return buff_sol;
        }
        else if (!pt_pin)
        {
            PSN_LOG_TRACE("(", location.getX(), ",", location.getY(),
                          ") bottomUp ---> left");

            auto left = bottomUp(psn_inst, driver_pin, st_tree->left(pt), pt,
                                 st_tree, options);
            PSN_LOG_TRACE("(", location.getX(), ",", location.getY(),
                          ") bottomUp ---> right");

            auto right = bottomUp(psn_inst, driver_pin, st_tree->right(pt), pt,
                                  st_tree, options);

            PSN_LOG_TRACE("(", location.getX(), ",", location.getY(),
                          ") bottomUp merging");

            std::shared_ptr<BufferSolution> buff_sol =
                std::make_shared<BufferSolution>(
                    psn_inst, left, right, location,
                    options->buffer_lib[options->buffer_lib.size() / 2],
                    options->minimum_upstream_resistance);

            buff_sol->addWireDelayAndCapacitance(wire_res, wire_cap);
            buff_sol->addLeafTreesWithResynthesis(
                psn_inst, driver_pin, prev_location, options->buffer_lib,
                options->inverter_lib, mapping_terminals);

            return buff_sol;
        }
    }
    return nullptr;
}

void
BufferSolution::topDown(Psn* psn_inst, InstanceTerm* pin,
                        std::shared_ptr<BufferTree> tree, float& area,
                        int& net_index, int& buff_index,
                        std::unordered_set<Instance*>& added_buffers,
                        std::unordered_set<Net*>&      affected_nets)
{
    auto net = psn_inst->handler()->net(pin);
    if (!net)
    {
        net = psn_inst->handler()->net(psn_inst->handler()->term(pin));
    }
    if (!net)
    {
        PSN_LOG_ERROR("No net for", psn_inst->handler()->name(pin));
    }
    topDown(psn_inst, net, tree, area, net_index, buff_index, added_buffers,
            affected_nets);
}
void
BufferSolution::topDown(Psn* psn_inst, Net* net,
                        std::shared_ptr<BufferTree> tree, float& area,
                        int& net_index, int& buff_index,
                        std::unordered_set<Instance*>& added_buffers,
                        std::unordered_set<Net*>&      affected_nets)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    if (!net)
    {
        PSN_LOG_WARN("topDown buffering without target net!");

        return;
    }
    if (!tree)
    {
        PSN_LOG_WARN("Buffer tree is required!");

        return;
    }
    if (tree->isLoadNode())
    {
        PSN_LOG_DEBUG(handler.name(net), ": unbuffered at (",
                      tree->location().getX(), ",", tree->location().getY(),
                      ")");

        auto tree_pin = tree->pin();
        auto tree_net = handler.net(tree_pin);
        if (!tree_net)
        {
            // Top-level pin
            tree_net = handler.net(handler.term(tree_pin));
        }
        if (tree_net != net)
        {
            auto inst = handler.instance(tree_pin);
            handler.disconnect(tree_pin);
            auto lib_pin = handler.libraryPin(tree_pin);
            if (!lib_pin)
            {
                handler.connect(net, inst, handler.topPort(tree_pin));
            }
            else
            {
                handler.connect(net, inst, handler.libraryPin(tree_pin));
            }
            affected_nets.insert(net);
            affected_nets.insert(tree_net);
        }
    }
    else if (tree->isBufferNode())
    {

        PSN_LOG_DEBUG(handler.name(net), ": adding buffer [",
                      handler.name(tree->bufferCell()), "] at (",
                      tree->location().getX(), ",", tree->location().getY(),
                      ")..");

        auto buffer_name =
            handler.generateInstanceName("psn_buff_", buff_index);
        auto buffer_net_name = handler.generateNetName(net_index);
        auto buf_net = handler.bufferNet(net, tree->bufferCell(), buffer_name,
                                         buffer_net_name, tree->location());
        affected_nets.insert(net);
        affected_nets.insert(buf_net);
        area += handler.area(tree->bufferCell());
        added_buffers.insert(handler.instance(handler.faninPin(buf_net)));
        topDown(psn_inst, buf_net, tree->left(), area, net_index, buff_index,
                added_buffers, affected_nets);
    }
    else if (tree->isBranched())
    {
        PSN_LOG_DEBUG(handler.name(net), ": Buffering left..");

        topDown(psn_inst, net, tree->left(), area, net_index, buff_index,
                added_buffers, affected_nets);
        PSN_LOG_DEBUG(handler.name(net), ": Buffering right..");

        topDown(psn_inst, net, tree->right(), area, net_index, buff_index,
                added_buffers, affected_nets);
    }
}

}; // namespace psn
