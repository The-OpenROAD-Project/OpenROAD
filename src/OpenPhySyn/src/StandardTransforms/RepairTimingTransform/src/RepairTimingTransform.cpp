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

#include "RepairTimingTransform.hpp"
#include "OpenPhySyn/BufferTree.hpp"
#include "OpenPhySyn/DatabaseHandler.hpp"
#include "OpenPhySyn/LibraryMapping.hpp"
#include "OpenPhySyn/PsnGlobal.hpp"
#include "OpenPhySyn/PsnLogger.hpp"
#include "OpenPhySyn/StringUtils.hpp"
#include "db_sta/dbSta.hh"
#include "sta/Search.hh"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <sstream>

namespace psn
{

RepairTimingTransform::RepairTimingTransform()
    : buffer_count_(0),
      resize_up_count_(0),
      resize_down_count_(0),
      pin_swap_count_(0),
      net_count_(0),
      net_index_(0),
      buff_index_(0),
      transition_violations_(0),
      capacitance_violations_(0),
      current_area_(0.0),
      saved_slack_(0.0)
{
}

std::unordered_set<Instance*>
RepairTimingTransform::repairPin(Psn* psn_inst, InstanceTerm* pin,
                                 RepairTarget                          target,
                                 std::unique_ptr<OptimizationOptions>& options)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    if (handler.isTopLevel(pin))
    {
        PSN_LOG_DEBUG("Top-level");
        return std::unordered_set<Instance*>();
    }
    auto pin_net = handler.net(pin);

    // Remove existing buffers if rip-up enabled
    if (options->ripup_existing_buffer_max_levels)
    {
        std::unordered_set<Instance*> fanout_buff;
        auto connected_insts = handler.fanoutInstances(pin_net);
        for (auto& inst : connected_insts)
        {
            if (options->buffer_lib_set.count(handler.libraryCell(inst)))
            {
                fanout_buff.insert(inst);
            }
        }
        handler.ripupBuffers(fanout_buff);
    }

    // Create the Steiner tree
    pin_net      = handler.net(pin);
    auto st_tree = SteinerTree::create(pin_net, psn_inst);
    if (!st_tree)
    {
        if (handler.connectedPins(pin_net).size() >= 2)
        {
            PSN_LOG_ERROR("Failed to create steiner tree for",
                          handler.name(pin));
        }
        return std::unordered_set<Instance*>();
    }

    bool is_slack_repair = target == RepairTarget::RepairSlack;
    bool is_trans_repair = target == RepairTarget::RepairMaxTransition;
    bool is_cap_repair   = target == RepairTarget::RepairMaxCapacitance;

    auto driver_point = st_tree->driverPoint();
    auto driver_pin   = st_tree->pin(driver_point);
    auto driver_cell  = handler.instance(pin);

    auto top_point = st_tree->top();

    std::shared_ptr<BufferSolution> buff_sol;
    psn::LibraryCell*               replace_driver;

    // 1. Construct candidate buffer trees without insertion (bottomUp only)
    buff_sol =
        BufferSolution::bottomUp(psn_inst, driver_pin, top_point, driver_point,
                                 std::move(st_tree), options);

    std::unordered_set<Instance*> added_buffers;
    std::unordered_set<Net*>      affected_nets;

    // Pick the optimal solution
    if (buff_sol->bufferTrees().size())
    {
        bool use_min_cost = false;
        if (options->minimum_cost) // Check if minimum cost tree should be
                                   // constructed for pins with positive slack
        {
            handler.sta()->ensureLevelized();
            handler.sta()->vertexRequired(handler.vertex(pin),
                                          sta::MinMax::min());
            handler.sta()->findDelays(handler.vertex(pin));
            auto wp      = handler.worstSlackPath(pin);
            use_min_cost = !is_slack_repair && wp.size() &&
                           handler.worstSlack(wp[wp.size() - 1].pin()) > 0.0;
        }

        std::shared_ptr<BufferTree> buff_tree     = nullptr;
        std::shared_ptr<BufferTree> max_req_tree  = nullptr;
        std::shared_ptr<BufferTree> inv_buff_tree = nullptr;
        auto                        no_buff_tree  = buff_sol->bufferTrees()[0];
        float                       old_delay =
            handler.gateDelay(pin, no_buff_tree->totalCapacitance());
        float old_slack = no_buff_tree->totalRequiredOrSlew() - old_delay;

        auto driver_lib = handler.libraryCell(driver_cell);
        // 2. Construct minimum cost buffer tree
        if (use_min_cost)
        {
            bool lmt_exists;
            auto slew_limit = handler.pinSlewLimit(pin, &lmt_exists);
            if (lmt_exists)
            {
                auto cap_limit = handler.maxLoad(handler.libraryPin(pin));
                buff_tree      = buff_sol->optimalCostTree(
                    psn_inst, pin, inv_buff_tree, slew_limit, cap_limit);
            }

            if (!buff_tree)
            {
                buff_tree = buff_sol->optimalDriverTree(psn_inst, pin,
                                                        inv_buff_tree, 0);
            }
            else // If no tree satisfies the constraints use the optimal tree
            {
                max_req_tree = buff_sol->optimalDriverTree(psn_inst, pin,
                                                           inv_buff_tree, 0);
                if (max_req_tree->cost() <= buff_tree->cost())
                {
                    buff_tree    = max_req_tree;
                    max_req_tree = nullptr;
                }
            }
        }
        else
        {
            // Pick the tree with maximum slack gain
            buff_tree =
                buff_sol->optimalDriverTree(psn_inst, pin, inv_buff_tree, 0);
        }

        if (buff_tree)
        {
            auto sol_buf_count = buff_tree->bufferCount();
            if (sol_buf_count)
            {
                net_count_++;
            }
            if (!is_slack_repair &&
                options->use_best_solution_threshold) // Pick another close
                                                      // solution if better
                                                      // overhead
            {
                float buff_tree_delay =
                    handler.gateDelay(pin, buff_tree->totalCapacitance());
                float buff_tree_slack =
                    buff_tree->totalRequiredOrSlew() - buff_tree_delay;

                for (size_t i = 1; i < buff_sol->bufferTrees().size() &&
                                   i < options->best_solution_threshold_range;
                     i++)
                {
                    auto& tr = buff_sol->bufferTrees()[i];
                    float tr_delay =
                        handler.gateDelay(pin, tr->totalCapacitance());
                    float tr_slack = tr->totalRequiredOrSlew() - tr_delay;
                    if (buff_tree_slack - tr_slack <
                            options->best_solution_threshold &&
                        tr->cost() < buff_tree->cost())
                    {
                        buff_tree       = tr;
                        buff_tree_delay = tr_delay;
                        buff_tree_slack = tr_slack;
                    }
                }
            }

            replace_driver = (buff_tree->hasDriverCell() &&
                              buff_tree->driverCell() != driver_lib)
                                 ? buff_tree->driverCell()
                                 : nullptr;
            if (replace_driver)
            {
                PSN_LOG_DEBUG("Replace", handler.name(driver_lib), "with",
                              handler.name(replace_driver));
            }

            float new_delay =
                handler.gateDelay(pin, buff_tree->totalCapacitance());
            float new_slack = buff_tree->totalRequiredOrSlew() - new_delay;

            float gain = new_slack - old_slack;
            saved_slack_ += gain;

            std::function<bool(InstanceTerm * pin)> vio_check_func;
            if (is_trans_repair)
            {
                vio_check_func = [&](InstanceTerm* pin) -> bool {
                    return handler.violatesMaximumTransition(
                        pin, options->transition_pessimism_factor);
                };
            }
            if (is_cap_repair)
            {
                vio_check_func = [&](InstanceTerm* pin) -> bool {
                    return handler.violatesMaximumCapacitance(
                        pin, options->capacitance_pessimism_factor);
                };
            }
            if (is_slack_repair)
            {
                vio_check_func = [&](InstanceTerm* pin) -> bool {
                    return handler.worstSlack(pin) < 0.0;
                };
            }

            if (buff_tree->cost() <= std::numeric_limits<float>::epsilon() ||
                std::fabs(gain - options->minimum_gain) >=
                    -std::numeric_limits<float>::epsilon())
            {
                bool is_fixed = false;
                // 3. Run pin-swap
                if (!is_fixed && options->repair_by_pinswap &&
                    options->current_iteration == 0)
                {
                    handler.sta()->ensureLevelized();
                    handler.sta()->vertexRequired(handler.vertex(pin),
                                                  sta::MinMax::min());
                    handler.sta()->findDelays(handler.vertex(pin));
                    auto wp = handler.worstSlackPath(pin, true);
                    if (wp.size() > 1)
                    {
                        auto inpin      = wp[wp.size() - 2].pin();
                        auto swap_pin   = inpin;
                        auto commu_pins = handler.commutativePins(inpin);
                        if (commu_pins.size())
                        {
                            auto pre_swap_wp =
                                handler.worstSlackPath(pin, true);
                            float pre_swap_slack = handler.worstSlack(
                                pre_swap_wp[pre_swap_wp.size() - 1].pin());

                            for (auto& cp : commu_pins)
                            {
                                handler.swapPins(swap_pin, cp);
                                handler.sta()->ensureLevelized();
                                handler.sta()->vertexRequired(
                                    handler.vertex(pin), sta::MinMax::min());
                                handler.sta()->findDelays(handler.vertex(pin));
                                auto post_swap_wp =
                                    handler.worstSlackPath(pin, true);
                                if (post_swap_wp.size())
                                {
                                    float post_swap_slack = handler.worstSlack(
                                        post_swap_wp[post_swap_wp.size() - 1]
                                            .pin());
                                    if (post_swap_slack > pre_swap_slack)
                                    {
                                        pre_swap_slack = post_swap_slack;
                                        swap_pin       = cp;
                                    }
                                    else
                                    {
                                        handler.swapPins(swap_pin, cp);
                                        handler.sta()
                                            ->search()
                                            ->findAllArrivals();
                                        handler.sta()->search()->findRequireds();
                                    }
                                }
                            }
                            if (swap_pin != inpin)
                            {
                                pin_swap_count_++;
                                std::vector<Net*> fanin_nets;
                                for (auto& fpin :
                                     handler.inputPins(driver_cell))
                                {
                                    fanin_nets.push_back(handler.net(fpin));
                                }
                                affected_nets.insert(handler.net(pin));
                                affected_nets.insert(fanin_nets.begin(),
                                                     fanin_nets.end());
                                for (auto& net : affected_nets)
                                {
                                    handler.calculateParasitics(net);
                                }
                                affected_nets.clear();
                                is_fixed = !vio_check_func(pin);
                            }
                        }
                    }
                }
                // Try to resize before inserting the buffers
                // 4. Upsize till no violations
                if (!is_fixed && options->repair_by_resize &&
                    (!is_slack_repair || options->resize_for_negative_slack))
                {
                    auto driver_types = handler.equivalentCells(
                        handler.libraryCell(driver_cell));
                    float current_area    = handler.area(driver_lib);
                    auto  replaced_driver = driver_lib;
                    int   attmepts        = 0;
                    for (auto& driver_size : driver_types)
                    {
                        float new_driver_area = handler.area(driver_size);
                        // Only test larger drivers for now
                        if ((new_driver_area - current_area) <
                                buff_tree->cost() &&
                            handler.area(driver_size) > current_area)
                        {
                            handler.replaceInstance(driver_cell, driver_size);
                            handler.sta()->vertexRequired(handler.vertex(pin),
                                                          sta::MinMax::min());
                            handler.sta()->findDelays(handler.vertex(pin));
                            is_fixed =
                                !handler.hasElectricalViolation(
                                    pin, options->capacitance_pessimism_factor,
                                    options->transition_pessimism_factor) &&
                                !vio_check_func(pin);
                            replaced_driver = driver_size;
                            attmepts++;
                            // Only try three upsizes
                            if (is_fixed || attmepts > 5)
                            {
                                if (is_fixed)
                                {
                                    std::vector<Net*> fanin_nets;
                                    for (auto& fpin :
                                         handler.inputPins(driver_cell))
                                    {
                                        fanin_nets.push_back(handler.net(fpin));
                                    }
                                    affected_nets.insert(handler.net(pin));
                                    affected_nets.insert(fanin_nets.begin(),
                                                         fanin_nets.end());
                                }
                                break;
                            }
                        }
                    }
                    if (!is_fixed)
                    {
                        // Return to the original size
                        handler.replaceInstance(driver_cell, driver_lib);
                        replaced_driver = driver_lib;
                    }
                    if (driver_lib != replaced_driver)
                    {
                        current_area_ -= handler.area(driver_lib);
                        current_area_ += handler.area(replaced_driver);
                        resize_up_count_++;
                    }
                    handler.sta()->vertexRequired(handler.vertex(pin),
                                                  sta::MinMax::min());
                    handler.sta()->findDelays(handler.vertex(pin));
                }
                // 5. Buffer if not fixed by resizing
                if (!is_fixed && !options->disable_buffering)
                {
                    BufferSolution::topDown(
                        psn_inst, pin, buff_tree, current_area_, net_index_,
                        buff_index_, added_buffers, affected_nets);
                    buffer_count_ += buff_tree->bufferCount();
                    for (auto& net : affected_nets)
                    {
                        handler.calculateParasitics(net);
                    }
                    affected_nets.clear();
                    handler.sta()->vertexRequired(handler.vertex(pin),
                                                  sta::MinMax::min());
                    handler.sta()->findDelays(handler.vertex(pin));
                    is_fixed = !vio_check_func(pin);
                    if (options->minimum_cost)
                    {
                        if (handler.hasElectricalViolation(
                                pin, options->capacitance_pessimism_factor,
                                options->transition_pessimism_factor) &&
                            max_req_tree && max_req_tree != buff_tree)
                        {
                            handler.ripupBuffers(added_buffers);
                            added_buffers.clear();
                            buffer_count_ -= buff_tree->bufferCount();
                            BufferSolution::topDown(psn_inst, pin, max_req_tree,
                                                    current_area_, net_index_,
                                                    buff_index_, added_buffers,
                                                    affected_nets);
                            buffer_count_ += max_req_tree->bufferCount();

                            for (auto& net : affected_nets)
                            {
                                handler.calculateParasitics(net);
                            }
                            affected_nets.clear();
                            handler.sta()->vertexRequired(handler.vertex(pin),
                                                          sta::MinMax::min());
                            handler.sta()->findDelays(handler.vertex(pin));
                            is_fixed = !vio_check_func(pin);
                        }
                    }
                }
                // 6. Resize again if not fixed by buffering
                if (options->repair_by_resize && !is_fixed && !is_slack_repair)
                {
                    auto driver_types = handler.equivalentCells(
                        handler.libraryCell(driver_cell));
                    float current_area    = handler.area(driver_lib);
                    auto  replaced_driver = driver_lib;
                    is_fixed              = !vio_check_func(pin);
                    int attempts          = 0;
                    for (auto& driver_size : driver_types)
                    {
                        // Only test larger drivers for now
                        if (handler.area(driver_size) > current_area)
                        {
                            handler.replaceInstance(driver_cell, driver_size);
                            is_fixed        = !vio_check_func(pin);
                            replaced_driver = driver_size;
                            attempts++;
                            // Only try three upsizes
                            if (is_fixed || attempts > 3)
                            {
                                if (is_fixed)
                                {
                                    std::vector<Net*> fanin_nets;
                                    for (auto& fpin :
                                         handler.inputPins(driver_cell))
                                    {
                                        fanin_nets.push_back(handler.net(fpin));
                                    }
                                    affected_nets.insert(handler.net(pin));
                                    affected_nets.insert(fanin_nets.begin(),
                                                         fanin_nets.end());
                                }

                                break;
                            }
                        }
                    }
                    if (!is_fixed && !is_slack_repair)
                    {
                        // Return to the original size
                        replaced_driver = driver_lib;
                        handler.replaceInstance(driver_cell, driver_lib);
                    }
                    if (driver_lib != replaced_driver)
                    {
                        current_area_ -= handler.area(driver_lib);
                        current_area_ += handler.area(replaced_driver);
                        resize_up_count_++;
                    }
                }

                if (replace_driver)
                {
                    handler.replaceInstance(driver_cell, replace_driver);
                    current_area_ -= handler.area(driver_lib);
                    current_area_ += handler.area(replace_driver);
                    resize_up_count_++;
                    std::vector<Net*> fanin_nets;
                    for (auto& fpin : handler.inputPins(driver_cell))
                    {
                        fanin_nets.push_back(handler.net(fpin));
                    }
                    affected_nets.insert(handler.net(pin));
                    affected_nets.insert(fanin_nets.begin(), fanin_nets.end());
                }
                for (auto& net : affected_nets)
                {
                    handler.calculateParasitics(net);
                }
                is_fixed = !vio_check_func(pin);
                if (is_fixed)
                {
                    resizeDown(psn_inst, pin, options);
                }
                return added_buffers;
            }
            else
            {
                PSN_LOG_DEBUG("Discarding weak solution: ");

                buff_tree->logDebug();
            }
        }
    }
    return added_buffers;
}

int
RepairTimingTransform::fixCapacitanceViolations(
    Psn* psn_inst, std::vector<InstanceTerm*>& driver_pins,
    std::unique_ptr<OptimizationOptions>& options)
{
    PSN_LOG_DEBUG("Fixing capacitance violations");

    DatabaseHandler& handler         = *(psn_inst->handler());
    auto             clock_nets      = handler.clockNets();
    int              last_edit_count = getEditCount();
    for (auto& pin : driver_pins)
    {
        auto pin_net = handler.net(pin);
        if (pin_net && !clock_nets.count(pin_net) &&
            !handler.isSpecial(pin_net))
        {
            auto vio = handler.hasElectricalViolation(
                pin, options->capacitance_pessimism_factor,
                options->transition_pessimism_factor);
            if (vio == ElectircalViolation::Capacitance ||
                vio == ElectircalViolation::CapacitanceAndTransition)
            {
                PSN_LOG_DEBUG("Fixing cap. violations for pin",
                              handler.name(pin));

                repairPin(psn_inst, pin, RepairTarget::RepairMaxCapacitance,
                          options);
                if (options->legalization_frequency >
                    (getEditCount() - last_edit_count >=
                     options->legalization_frequency))
                {
                    last_edit_count = buffer_count_;
                    handler.legalize();
                }

                if (handler.hasMaximumArea() &&
                    current_area_ > handler.maximumArea())
                {
                    PSN_LOG_WARN("Maximum utilization reached");

                    return getEditCount();
                }
            }
        }
    }

    return getEditCount();
}
int
RepairTimingTransform::fixTransitionViolations(
    Psn* psn_inst, std::vector<InstanceTerm*>& driver_pins,
    std::unique_ptr<OptimizationOptions>& options)
{
    PSN_LOG_DEBUG("Fixing transition violations");

    DatabaseHandler& handler = *(psn_inst->handler());
    handler.resetDelays();
    auto clock_nets      = handler.clockNets();
    int  last_edit_count = getEditCount();
    for (auto& pin : driver_pins)
    {
        auto pin_net = handler.net(pin);

        if (pin_net && !clock_nets.count(pin_net) &&
            !handler.isSpecial(pin_net))
        {
            auto net_pins = handler.pins(pin_net);
            auto vio      = handler.hasElectricalViolation(
                pin, options->capacitance_pessimism_factor,
                options->transition_pessimism_factor);
            if (vio == ElectircalViolation::Transition ||
                vio == ElectircalViolation::CapacitanceAndTransition)
            {
                PSN_LOG_DEBUG("Fixing transition violations for pin",
                              handler.name(pin));

                auto added_buffers = repairPin(
                    psn_inst, pin, RepairTarget::RepairMaxTransition, options);

                if (options->legalization_frequency > 0 &&
                    (getEditCount() - last_edit_count >=
                     options->legalization_frequency))
                {
                    last_edit_count = getEditCount();
                    handler.legalize();
                }
                if (handler.hasMaximumArea() &&
                    current_area_ > handler.maximumArea())
                {
                    PSN_LOG_WARN("Maximum utilization reached");

                    return getEditCount();
                }
            }
        }
    }
    return getEditCount();
}
int
RepairTimingTransform::fixNegativeSlack(
    Psn* psn_inst, std::unordered_set<InstanceTerm*>& filter_pins,
    std::unique_ptr<OptimizationOptions>& options)
{
    PSN_LOG_DEBUG("Fixing negative slack violations");

    DatabaseHandler& handler              = *(psn_inst->handler());
    auto             negative_slack_paths = handler.getNegativeSlackPaths();

    if (!negative_slack_paths.size())
    {
        return 0;
    }
    PSN_LOG_INFO("Found", negative_slack_paths.size(), "negative slack paths");

    int                               check_negative_slack_freq = 10;
    std::unordered_set<InstanceTerm*> buffered_pins;
    int                               iteration       = 0;
    int                               last_edit_count = buffer_count_;

    // NOTE: This can be done in parallel..
    int unfixed_paths = 0;
    for (int i = 0; (size_t)i < negative_slack_paths.size() &&
                    (!options->max_negative_slack_paths ||
                     i < options->max_negative_slack_paths);
         i++)
    {
        int   fixed_pin_count = 0;
        auto& pth             = negative_slack_paths[i];
        std::reverse(pth.begin(), pth.end());
        auto end_pin = pth[0].pin();
        // Refresh path
        pth                     = handler.worstSlackPath(end_pin);
        negative_slack_paths[i] = pth;
        std::reverse(pth.begin(), pth.end());
        float worst_slack = handler.worstSlack(end_pin);
        float init_slack  = worst_slack;
        for (auto& pt : pth)
        {
            if (worst_slack < 0.0)
            {
                auto pin = pt.pin();
                if ((!filter_pins.size() || filter_pins.count(pin)) &&
                    !buffered_pins.count(pin))
                {
                    if (handler.isAnyOutput(pin) &&
                        (!options->max_negative_slack_path_depth ||
                         fixed_pin_count <
                             options->max_negative_slack_path_depth))
                    {
                        fixed_pin_count++;
                        repairPin(psn_inst, pin, RepairTarget::RepairSlack,
                                  options);
                        if (options->legalization_frequency >
                            (buffer_count_ - last_edit_count >=
                             options->legalization_frequency))
                        {
                            last_edit_count = buffer_count_;
                            handler.legalize();
                        }

                        if (handler.hasMaximumArea() &&
                            current_area_ > handler.maximumArea())
                        {
                            PSN_LOG_WARN("Maximum utilization reached");

                            return getEditCount();
                        }
                        iteration++;
                        if (iteration % check_negative_slack_freq == 0)
                        {
                            worst_slack = handler.worstSlack(end_pin);
                        }
                    }
                }
            }
            else
            {
                PSN_LOG_DEBUG("Slack is positive");

                break;
            }
        }
        float new_slack = handler.worstSlack(end_pin);
        if (new_slack < 0.0 && init_slack == new_slack)
        {

            unfixed_paths++;
            if (unfixed_paths >= 20)
            {
                PSN_LOG_DEBUG("Failed to fix 20 successive paths, giving up");

                break;
            }
        }
        else
        {
            unfixed_paths = 0;
        }
    }

    return getEditCount();
    ;
}

int
RepairTimingTransform::resizeDown(Psn*                        psn_inst,
                                  std::vector<InstanceTerm*>& driver_pins,
                                  std::unique_ptr<OptimizationOptions>& options)
{
    PSN_LOG_INFO("Resize down");

    DatabaseHandler& handler    = *(psn_inst->handler());
    auto             clock_nets = handler.clockNets();
    handler.sta()->findDelays();
    float wns = handler.worstSlack();
    if (wns > 0)
    {
        wns = 0;
    }

    for (auto& pin : driver_pins)
    {
        auto pin_net = handler.net(pin);
        if (pin_net && !clock_nets.count(pin_net) &&
            !handler.isSpecial(pin_net))
        {
            resizeDown(psn_inst, pin, options);
        }
    }

    return resize_down_count_;
}
void
RepairTimingTransform::resizeDown(Psn* psn_inst, InstanceTerm* pin,
                                  std::unique_ptr<OptimizationOptions>& options)
{
    DatabaseHandler& handler  = *(psn_inst->handler());
    auto             net_pins = handler.pins(handler.net(pin));
    handler.sta()->ensureLevelized();
    handler.sta()->vertexRequired(handler.vertex(pin), sta::MinMax::min());
    handler.sta()->findDelays(handler.vertex(pin));
    bool fix =
        handler.hasElectricalViolation(
            pin, options->capacitance_pessimism_factor,
            options->transition_pessimism_factor) != ElectircalViolation::None;
    auto  wp  = handler.worstSlackPath(pin);
    float wns = handler.worstSlack();
    if (!fix && wp.size() && handler.worstSlack(wp[wp.size() - 1].pin()) > 0.0)
    {
        PSN_LOG_DEBUG("Resize down pin", handler.name(pin));

        auto replace_lib = handler.libraryCell(pin);
        auto init_lib    = replace_lib;
        auto inst        = handler.instance(pin);
        if (inst && handler.isSingleOutputCombinational(init_lib))
        {
            auto driver_types =
                handler.equivalentCells(handler.libraryCell(inst));
            auto current_area = handler.area(replace_lib);
            auto load_cap     = handler.loadCapacitance(pin);
            for (auto& d_type : driver_types)
            {
                auto area = handler.area(d_type);
                std::sort(options->buffer_lib.begin(),
                          options->buffer_lib.end(),
                          [&](LibraryCell* a, LibraryCell* b) -> bool {
                              return handler.area(a) > handler.area(b);
                          });
                if (area < current_area &&
                    handler.pinCapacitance(
                        handler.libraryInputPins(d_type).at(0)) <=
                        handler.pinCapacitance(
                            handler.libraryInputPins(init_lib).at(0)))
                {
                    if (handler.maxLoad(d_type) > load_cap)
                    {
                        handler.replaceInstance(inst, d_type);
                        handler.sta()->ensureLevelized();
                        handler.sta()->vertexRequired(handler.vertex(pin),
                                                      sta::MinMax::min());
                        handler.sta()->findDelays(handler.vertex(pin));
                        wp            = handler.worstSlackPath(pin);
                        float new_wns = handler.worstSlack();

                        if (!wp.size() ||
                            handler.hasElectricalViolation(
                                pin, options->capacitance_pessimism_factor,
                                options->transition_pessimism_factor) !=
                                ElectircalViolation::None ||
                            handler.worstSlack(wp[wp.size() - 1].pin()) < 0.0 ||
                            new_wns < wns)
                        {
                            handler.replaceInstance(inst, replace_lib);
                        }
                        else
                        {
                            current_area = area;
                            replace_lib  = d_type;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if (replace_lib != init_lib)
            {
                current_area_ -= handler.area(init_lib);
                current_area_ += handler.area(replace_lib);
                resize_down_count_++;
            }
        }
    }
}
int
RepairTimingTransform::getEditCount() const
{
    return buffer_count_ + resize_up_count_ + resize_down_count_ +
           pin_swap_count_;
}

int
RepairTimingTransform::repairTiming(
    Psn* psn_inst, std::unique_ptr<OptimizationOptions>& options,
    std::unordered_set<std::string> buffer_lib_names,
    std::unordered_set<std::string> inverter_lib_names,
    std::unordered_set<std::string> pin_names)
{
    DatabaseHandler& handler = *(psn_inst->handler());

    auto start = std::chrono::high_resolution_clock::now();

    std::unordered_set<InstanceTerm*> pins;
    for (auto& pin_name : pin_names)
    {
        auto pin = handler.pin(pin_name.c_str());
        if (!pin)
        {
            PSN_LOG_ERROR("Pin {} not found in the design");
            return -1;
        }
        pins.insert(pin);
    }

    if (options->cluster_buffers) // Cluster the buffer library to auto-select
                                  // the buffer cells
    {
        PSN_LOG_DEBUG("Generating buffer library");

        auto cluster_libs = handler.bufferClusters(
            options->cluster_threshold, options->minimize_cluster_buffers,
            options->cluster_inverters);
        options->buffer_lib   = cluster_libs.first;
        options->inverter_lib = cluster_libs.second;
        buffer_lib_names.clear();
        inverter_lib_names.clear();
        for (auto& b : options->buffer_lib)
        {
            buffer_lib_names.insert(handler.name(b));
        }
        for (auto& b : options->inverter_lib)
        {
            inverter_lib_names.insert(handler.name(b));
        }
        PSN_LOG_DEBUG("Using", options->buffer_lib.size(), "Buffers and",
                      options->inverter_lib.size(), "Inverters");
    }
    for (auto& buf_name : buffer_lib_names)
    {
        auto lib_cell = handler.libraryCell(buf_name.c_str());
        if (!lib_cell)
        {
            PSN_LOG_ERROR("Buffer cell", buf_name, "not found in the library.");

            return -1;
        }
        options->buffer_lib.push_back(lib_cell);
    }

    for (auto& inv_name : inverter_lib_names)
    {
        auto lib_cell = handler.libraryCell(inv_name.c_str());
        if (!lib_cell)
        {
            PSN_LOG_ERROR("Inverter cell", inv_name,
                          "not found in the library.");

            return -1;
        }
        options->inverter_lib.push_back(lib_cell);
    }
    // Remove duplicates
    std::unique(options->buffer_lib.begin(), options->buffer_lib.end());
    std::unique(options->inverter_lib.begin(), options->inverter_lib.end());

    // Sort by area
    options->buffer_lib_set = std::unordered_set<LibraryCell*>(
        options->buffer_lib.begin(), options->buffer_lib.end());
    options->inverter_lib_set = std::unordered_set<LibraryCell*>(
        options->inverter_lib.begin(), options->inverter_lib.end());

    std::sort(options->buffer_lib.begin(), options->buffer_lib.end(),
              [&](LibraryCell* a, LibraryCell* b) -> bool {
                  return handler.area(a) < handler.area(b);
              });
    std::sort(options->inverter_lib.begin(), options->inverter_lib.end(),
              [&](LibraryCell* a, LibraryCell* b) -> bool {
                  return handler.area(a) < handler.area(b);
              });

    auto buf_names_vec = std::vector<std::string>(buffer_lib_names.begin(),
                                                  buffer_lib_names.end());
    auto inv_names_vec = std::vector<std::string>(inverter_lib_names.begin(),
                                                  inverter_lib_names.end());
    PSN_LOG_INFO("Buffer library:", buffer_lib_names.size()
                                        ? StringUtils::join(buf_names_vec, ", ")
                                        : "None");

    PSN_LOG_INFO("Inverter library:",
                 inverter_lib_names.size()
                     ? StringUtils::join(inv_names_vec, ", ")
                     : "None");

    PSN_LOG_INFO("Buffering:",
                 !options->disable_buffering ? "enabled" : "disabled");
    PSN_LOG_INFO("Driver sizing:",
                 options->repair_by_resize ? "enabled" : "disabled");
    PSN_LOG_INFO("Pin-swapping:",
                 options->repair_by_pinswap ? "enabled" : "disabled");

    PSN_LOG_INFO("Mode:", options->timerless ? "Timerless" : "Timing-Driven");

    for (int i = 0; i < options->max_iterations; i++)
    {
        PSN_LOG_INFO("Iteration", i + 1);

        options->current_iteration = i;
        auto driver_pins           = handler.levelDriverPins(true, pins);
        bool hasVio                = false;
        int  pre_fix_count         = 0;

        if (options->repair_transition_violations)
        {
            pre_fix_count = getEditCount();
            // Run electrical correction (transition violation) pass
            fixTransitionViolations(psn_inst, driver_pins, options);
            if (pre_fix_count != getEditCount())
            {
                hasVio = true;
            }
            if (options->legalization_frequency > 0)
            {
                handler.legalize(1);
                handler.setWireRC(handler.resistancePerMicron(),
                                  handler.capacitancePerMicron(), false);
            }
            driver_pins = handler.levelDriverPins(true, pins);
        }

        if (options->repair_capacitance_violations)
        {
            pre_fix_count = getEditCount();
            // Run electrical correction (capacitance violation) pass
            fixCapacitanceViolations(psn_inst, driver_pins, options);
            if (pre_fix_count != getEditCount())
            {
                hasVio = true;
            }

            if (options->legalization_frequency > 0)
            {
                handler.legalize(1);
                handler.setWireRC(handler.resistancePerMicron(),
                                  handler.capacitancePerMicron(), false);
            }
            driver_pins = handler.levelDriverPins(true, pins);
        }

        if (options->repair_negative_slack)
        {
            pre_fix_count = getEditCount();
            // Run negative slack pass
            fixNegativeSlack(psn_inst, pins, options);
            if (pre_fix_count != getEditCount())
            {
                hasVio = true;
            }
            if (options->legalization_frequency > 0)
            {
                handler.legalize(1);
                handler.setWireRC(handler.resistancePerMicron(),
                                  handler.capacitancePerMicron(), false);
            }
            driver_pins = handler.levelDriverPins(true, pins);
        }
        handler.setWireRC(handler.resistancePerMicron(),
                          handler.capacitancePerMicron(), false);
        if (options->legalize_each_iteration)
        {
            handler.legalize(1);
            handler.setWireRC(handler.resistancePerMicron(),
                              handler.capacitancePerMicron(), false);
        }
        if (!hasVio)
        {
            PSN_LOG_INFO(
                "No more violations or cannot find more optimal buffer");

            PSN_LOG_DEBUG(
                "No more violations or cannot find more optimal buffer");

            break;
        }
    }

    if (options->repair_by_downsize)
    {
        // Run final downsizing phase for any extra area recovery
        auto driver_pins = handler.levelDriverPins(true, pins);
        resizeDown(psn_inst, driver_pins, options);
        if (options->legalization_frequency > 0)
        {
            handler.legalize(1);
            handler.setWireRC(handler.resistancePerMicron(),
                              handler.capacitancePerMicron(), false);
        }
    }
    if (options->legalize_eventually)
    {
        handler.legalize(1);
        handler.setWireRC(handler.resistancePerMicron(),
                          handler.capacitancePerMicron(), false);
    }
    auto end      = std::chrono::high_resolution_clock::now();
    auto runtime  = end - start;
    current_area_ = handler.area();
    PSN_LOG_DEBUG(
        "Runtime:",
        std::chrono::duration_cast<std::chrono::seconds>(runtime).count(), "s");

    PSN_LOG_INFO("Buffers:", buffer_count_);

    PSN_LOG_INFO("Resize up:", resize_up_count_);

    PSN_LOG_INFO("Resize down:", resize_down_count_);

    PSN_LOG_INFO("Pin Swap:", pin_swap_count_);

    PSN_LOG_INFO("Buffered nets:", net_count_);

    PSN_LOG_INFO("Transition violations:", transition_violations_);

    PSN_LOG_INFO("Capacitance violations:", capacitance_violations_);

    PSN_LOG_DEBUG("Slack gain:", saved_slack_);

    PSN_LOG_INFO("Initial area:",
                 handler.unitScaledArea(options->initial_area));

    PSN_LOG_INFO("New area:", handler.unitScaledArea(current_area_));
    psn_inst->handler()->notifyDesignAreaChanged(current_area_);

    return getEditCount();
}

int
RepairTimingTransform::run(Psn* psn_inst, std::vector<std::string> args)
{
    buffer_count_      = 0;
    resize_up_count_   = 0;
    resize_down_count_ = 0;
    net_count_         = 0;
    pin_swap_count_    = 0;
    current_area_      = psn_inst->handler()->area();
    saved_slack_       = 0.0;
    capacitance_violations_ =
        psn_inst->handler()->maximumCapacitanceViolations().size();
    transition_violations_ =
        psn_inst->handler()->maximumTransitionViolations().size();

    std::unique_ptr<OptimizationOptions> options(new OptimizationOptions);

    options->initial_area                  = current_area_;
    options->repair_capacitance_violations = false;
    options->repair_negative_slack         = false;
    options->repair_transition_violations  = false;
    options->minimize_cluster_buffers      = true;

    std::unordered_set<std::string> buffer_lib_names;
    std::unordered_set<std::string> inverter_lib_names;
    std::unordered_set<std::string> pin_names;
    std::unordered_set<std::string> keywords(
        {"-capacitance_violations",    // Repair capacitance violations
         "-transition_violations",     // Repair transition violations
         "-negative_slack_violations", // Repair paths with negative slacks
         "-iterations",                // Maximum number of iterations
         "-buffers",                   // Manually specify buffer cells to use
         "-inverters",                 // Manually specify inverter cells to use
         "-minimum_gain",        // Minimum slack gain to accept an optimization
         "-auto_buffer_library", // Auto-select buffer library
         "-auto_buffer_library_inverters_enabled", // Include inverters in the
                                                   // selected buffer library
         "-no_minimize_buffer_library",  // Do not run initial pruning phase for
                                         // buffer selection
         "-buffer_disabled",             // Disable all buffering
         "-minimum_cost_buffer_enabled", // Enable minimum cost buffering
         "-resize_disabled",             // Disable repair by resizing
         "-downsize_enabled",            // Enable repair by downsizing
         "-pin_swap_disabled",           // Enable pin-swapping
         "-no_resize_for_negative_slack", // Disable resizing when solving
                                          // negative slack violation
         "-maximum_negative_slack_paths", // Maximum number of negative slack
                                          // paths to check (0 for no limit)
         "-maximum_negative_slack_path_depth", // Maximum vertices in the
                                               // negative slack path to check
                                               // (0 for no limit)
         "-legalize_eventually",     // Legalize at the end of the optimization
         "-legalize_each_iteration", // Legalize after each iteration
         "-post_place",              // Post placement phase mode
         "-post_route", // Post routing phase mode (not currently supported)
         "-legalization_frequency",       // Legalize after how many edit
         "-capacitance_pessimism_factor", // Cap limit scaling factor
         "-transition_pessimism_factor",  // Transition limit scaling factor
         "-high_effort", // Trade-off runtime versus optimization quality by
                         // weaker pruning
         "-upstream_resistance"}); // Override default minimum upstream
                                   // resistance
    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i].size() > 2 && args[i][0] == '-' && args[i][1] == '-')
        {
            args[i] = args[i].substr(1);
        }
    }
    bool high_effort         = false;
    bool custom_upstream_res = false;
    for (size_t i = 0; i < args.size(); i++) // Capture the user arguments
    {
        if (args[i] == "-buffers")
        {
            i++;
            while (i < args.size())
            {
                if (args[i] == "-buffers")
                {
                    PSN_LOG_ERROR(help());
                    return -1;
                }
                else if (keywords.count(args[i]))
                {
                    break;
                }
                else if (args[i][0] == '-')
                {
                    PSN_LOG_ERROR(help());
                    return -1;
                }
                else
                {
                    buffer_lib_names.insert(args[i]);
                }
                i++;
            }
            i--;
        }
        else if (args[i] == "-inverters")
        {
            i++;
            while (i < args.size())
            {
                if (args[i] == "-inverters")
                {
                    PSN_LOG_ERROR(help());
                    return -1;
                }
                else if (keywords.count(args[i]))
                {
                    break;
                }
                else if (args[i][0] == '-')
                {
                    PSN_LOG_ERROR(help());
                    return -1;
                }
                else
                {
                    inverter_lib_names.insert(args[i]);
                }
                i++;
            }
            i--;
        }
        else if (args[i] == "-pins")
        {
            i++;
            while (i < args.size())
            {
                if (args[i] == "-pins")
                {
                    PSN_LOG_ERROR(help());
                    return -1;
                }
                else if (keywords.count(args[i]))
                {
                    break;
                }
                else if (args[i][0] == '-')
                {
                    PSN_LOG_ERROR(help());
                    return -1;
                }
                else
                {
                    pin_names.insert(args[i]);
                }
                i++;
            }
            i--;
        }
        else if (args[i] == "-auto_buffer_library")
        {
            options->cluster_buffers = true;
            i++;
            if (i >= args.size())
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
            else
            {
                if (args[i] == "single")
                {
                    options->cluster_threshold = PSN_CLUSTER_SIZE_SINGLE;
                }
                else if (args[i] == "small")
                {
                    options->cluster_threshold = PSN_CLUSTER_SIZE_SMALL;
                }
                else if (args[i] == "medium")
                {
                    options->cluster_threshold = PSN_CLUSTER_SIZE_MEDIUM;
                }
                else if (args[i] == "large")
                {
                    options->cluster_threshold = PSN_CLUSTER_SIZE_LARGE;
                }
                else if (args[i] == "all")
                {
                    options->cluster_threshold = PSN_CLUSTER_SIZE_ALL;
                }
                else
                {
                    PSN_LOG_ERROR(help());
                    return -1;
                }
            }
        }
        else if (args[i] == "-auto_buffer_library_inverters_enabled")
        {
            options->cluster_inverters = true;
        }
        else if (args[i] == "-no_minimize_buffer_library")
        {
            options->minimize_cluster_buffers = false;
        }
        else if (args[i] == "-iterations")
        {
            i++;
            if (i >= args.size() || !StringUtils::isNumber(args[i]))
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
            else
            {
                options->max_iterations = atoi(args[i].c_str());
            }
        }
        else if (args[i] == "-maximum_negative_slack_paths")
        {
            i++;
            if (i >= args.size() || !StringUtils::isNumber(args[i]))
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
            else
            {
                options->max_negative_slack_paths = atoi(args[i].c_str());
            }
        }
        else if (args[i] == "-maximum_negative_slack_path_depth")
        {
            i++;
            if (i >= args.size() || !StringUtils::isNumber(args[i]))
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
            else
            {
                options->max_negative_slack_path_depth = atoi(args[i].c_str());
            }
        }
        else if (args[i] == "-minimum_gain")
        {
            i++;
            if (i >= args.size() || !StringUtils::isNumber(args[i]))
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
            else
            {
                options->minimum_gain = atof(args[i].c_str());
            }
        }
        else if (args[i] == "-capacitance_pessimism_factor")
        {
            i++;
            if (i >= args.size() || !StringUtils::isNumber(args[i]))
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
            else
            {
                options->capacitance_pessimism_factor = atof(args[i].c_str());
            }
        }
        else if (args[i] == "-transition_pessimism_factor")
        {
            i++;
            if (i >= args.size() || !StringUtils::isNumber(args[i]))
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
            else
            {
                options->transition_pessimism_factor = atof(args[i].c_str());
            }
        }
        else if (args[i] == "-capacitance_violations")
        {
            options->repair_capacitance_violations = true;
        }
        else if (args[i] == "-transition_violations")
        {
            options->repair_transition_violations = true;
        }
        else if (args[i] == "-negative_slack_violations")
        {
            options->repair_negative_slack = true;
        }
        else if (args[i] == "-enable_driver_resize")
        {
            options->driver_resize = true;
        }
        else if (args[i] == "-area_penalty")
        {
            i++;
            if (i >= args.size() || !StringUtils::isNumber(args[i]))
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
            else
            {
                options->area_penalty = atof(args[i].c_str());
            }
        }
        else if (args[i] == "-upstream_resistance")
        {
            i++;
            if (i >= args.size() || !StringUtils::isNumber(args[i]))
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
            else
            {
                options->minimum_upstream_resistance = atof(args[i].c_str());
                custom_upstream_res                  = true;
            }
        }
        else if (args[i] == "-buffer_disabled")
        {
            options->disable_buffering = true;
        }
        else if (args[i] == "-minimum_cost_buffer_enabled")
        {
            options->minimum_cost = true;
        }
        else if (args[i] == "-pin_swap_disabled")
        {
            options->repair_by_pinswap = false;
        }
        else if (args[i] == "-resize_disabled")
        {
            options->repair_by_resize = false;
        }
        else if (args[i] == "-downsize_enabled")
        {
            options->repair_by_downsize = true;
        }
        else if (args[i] == "-no_resize_for_negative_slack")
        {
            options->resize_for_negative_slack = false;
        }
        else if (args[i] == "-legalize_eventually")
        {
            options->legalize_eventually = true;
        }
        else if (args[i] == "-legalize_each_iteration")
        {
            options->legalize_each_iteration = true;
        }
        else if (args[i] == "-post_place")
        {
            options->phase = DesignPhase::PostPlace;
        }
        else if (args[i] == "-post_route")
        {
            options->phase = DesignPhase::PostRoute;
            PSN_LOG_WARN(
                "Post-routing optimization is not currently supported.");
            return -1;
        }
        else if (args[i] == "-high_effort")
        {
            high_effort = true;
        }
        else
        {
            PSN_LOG_ERROR(help());
            return -1;
        }
    }
    if (!custom_upstream_res)
    {
        if (high_effort)
        {
            options->minimum_upstream_resistance = 120;
        }
        else
        {
            options->minimum_upstream_resistance = 600;
        }
    }

    if (!options->repair_capacitance_violations &&
        !options->repair_transition_violations &&
        !options->repair_negative_slack)
    {
        options->repair_transition_violations  = true;
        options->repair_capacitance_violations = true;
        options->repair_negative_slack         = true;
    }
    if (!options->cluster_buffers && !buffer_lib_names.size())
    {
        options->cluster_buffers   = true;
        options->cluster_threshold = PSN_CLUSTER_SIZE_SMALL;
    }

    PSN_LOG_DEBUG("repair_timing", StringUtils::join(args, " "));

    return repairTiming(psn_inst, options, buffer_lib_names, inverter_lib_names,
                        pin_names);
}

} // namespace psn
