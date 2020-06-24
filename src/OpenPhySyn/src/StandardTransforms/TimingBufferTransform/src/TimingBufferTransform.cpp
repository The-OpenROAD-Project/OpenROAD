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

#include "TimingBufferTransform.hpp"
#include "OpenPhySyn/LibraryMapping.hpp"
#include "OpenPhySyn/PsnGlobal.hpp"
#include "OpenPhySyn/PsnLogger.hpp"
#include "OpenPhySyn/StringUtils.hpp"
#include "db_sta/dbSta.hh"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <sstream>

namespace psn
{

TimingBufferTransform::TimingBufferTransform()
    : buffer_count_(0),
      resize_count_(0),
      clone_count_(0),
      resynth_count_(0),
      net_count_(0),
      timerless_rebuffer_count_(0),
      net_index_(0),
      buff_index_(0),
      clone_index_(0),
      transition_violations_(0),
      capacitance_violations_(0),
      slack_violations_(0),
      current_area_(0.0),
      saved_slack_(0.0)
{
}

std::unordered_set<Instance*>
TimingBufferTransform::bufferPin(Psn* psn_inst, InstanceTerm* pin,
                                 RepairTarget                          target,
                                 std::unique_ptr<OptimizationOptions>& options)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    if (handler.isTopLevel(pin))
    {
        PSN_LOG_WARN("Not handled yet!");

        return std::unordered_set<Instance*>();
    }
    auto pin_net = handler.net(pin);
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
    pin_net      = handler.net(pin);
    auto st_tree = SteinerTree::create(pin_net, psn_inst);
    if (!st_tree)
    {
        PSN_LOG_DEBUG("Failed to create steiner tree for", handler.name(pin));

        return std::unordered_set<Instance*>();
    }

    auto driver_point = st_tree->driverPoint();
    auto driver_pin   = st_tree->pin(driver_point);
    auto driver_cell  = handler.instance(pin);

    auto top_point = st_tree->top();

    std::shared_ptr<BufferTree>     inv_buff_tree = nullptr;
    std::shared_ptr<BufferSolution> buff_sol      = nullptr;

    psn::LibraryCell* replace_driver;
    auto              mapping = handler.getLibraryCellMapping(driver_cell);
    bool              remap   = mapping && options->repair_by_resynthesis;

    if (remap)
    {
        auto terminals = mapping->terminals();
        buff_sol       = BufferSolution::bottomUpWithResynthesis(
            psn_inst, driver_pin, top_point, driver_point, std::move(st_tree),
            options, terminals);
    }
    else
    {
        buff_sol =
            BufferSolution::bottomUp(psn_inst, driver_pin, top_point,
                                     driver_point, std::move(st_tree), options);
    }
    std::unordered_set<Instance*> added_buffers;
    std::unordered_set<Net*>      affected_nets;
    if (buff_sol->bufferTrees().size())
    {
        std::shared_ptr<BufferTree> buff_tree    = nullptr;
        auto                        no_buff_tree = buff_sol->bufferTrees()[0];
        auto driver_lib = handler.libraryCell(driver_cell);
        if (options->driver_resize && driver_cell &&
            handler.outputPins(driver_cell).size() == 1)
        {
            buff_tree =
                buff_sol->optimalDriverTree(psn_inst, pin, inv_buff_tree, 0);
            auto driver_types =
                handler.equivalentCells(handler.libraryCell(driver_cell));
            if (driver_types.size() == 1)
            {
                buff_tree = buff_sol->optimalDriverTree(psn_inst, pin,
                                                        inv_buff_tree, 0);
            }
            else
            {
                buff_tree = buff_sol->optimalDriverTreeWithResize(
                    psn_inst, pin, driver_types, options->area_penalty);
            }
        }
        else if (remap)
        {
            buff_tree = buff_sol->optimalDriverTreeWithResynthesis(
                psn_inst, pin, options->area_penalty);
        }
        else
        {
            buff_tree =
                buff_sol->optimalDriverTree(psn_inst, pin, inv_buff_tree, 0);
        }

        if (buff_tree)
        {
            if (options->repair_by_resynthesis &&
                options->current_iteration == 0 &&
                (handler.isAnyANDOR(driver_lib) ||
                 handler.isBuffer(driver_lib) ||
                 handler.isInverter(driver_lib)) &&
                (buff_tree->isBranched() && buff_tree->left()->isBufferNode() &&
                 buff_tree->right()->isBufferNode()))
            {
                LibraryCell* shattered = nullptr;
                if (handler.libraryInputPins(driver_lib).size() == 4)
                {
                    shattered = handler.closestDriver(
                        driver_lib, handler.cellSuperset(driver_lib, 2), 0.5);
                }
                if (shattered)
                {
                    PSN_LOG_INFO("Shattered", handler.name(driver_lib), "->",
                                 handler.name(shattered));
                }

                auto inverted = handler.inverseCells(driver_lib);
                auto closest_inverse =
                    handler.closestDriver(driver_lib, inverted);
                if (closest_inverse)
                {
                    handler.sta()->ensureLevelized();
                    handler.sta()->findRequireds();
                    handler.sta()->findDelays(handler.vertex(pin));
                    auto wp = handler.worstSlackPath(pin, true);
                    if (wp.size() &&
                        handler.worstSlack(wp[wp.size() - 1].pin()) > 0.0)
                    {
                        float        current_cost   = 0;
                        float        inverting_cost = 0;
                        LibraryCell* left_buff      = nullptr;
                        LibraryCell* right_buff     = nullptr;
                        LibraryCell* left_inv       = nullptr;
                        LibraryCell* right_inv      = nullptr;
                        if (buff_tree->left() &&
                            buff_tree->left()->isBufferNode())
                        {
                            left_buff = buff_tree->left()->bufferCell();
                            left_inv  = handler.closestDriver(
                                left_buff, handler.inverseCells(left_buff));
                            current_cost += handler.area(left_buff);
                            inverting_cost += handler.area(left_inv);
                        }
                        if (buff_tree->right() &&
                            buff_tree->right()->isBufferNode())
                        {
                            right_buff = buff_tree->right()->bufferCell();
                            right_inv  = handler.closestDriver(
                                right_buff, handler.inverseCells(right_buff));
                            current_cost += handler.area(right_buff);
                            inverting_cost += handler.area(right_inv);
                        }
                        if (handler.area(closest_inverse) + inverting_cost <
                            handler.area(driver_lib) + current_cost)
                        {
                            handler.replaceInstance(driver_cell,
                                                    closest_inverse);
                            current_area_ -= handler.area(driver_lib);
                            current_area_ += handler.area(closest_inverse);
                            std::vector<Net*> fanin_nets;
                            for (auto& fpin : handler.inputPins(driver_cell))
                            {
                                fanin_nets.push_back(handler.net(fpin));
                            }
                            affected_nets.insert(handler.net(pin));
                            affected_nets.insert(fanin_nets.begin(),
                                                 fanin_nets.end());

                            buff_tree->left()->setBufferCell(left_inv);
                            buff_tree->right()->setBufferCell(right_inv);
                            resynth_count_++;
                        }
                    }
                }
            }
            auto sol_buf_count = buff_tree->bufferCount();
            if (sol_buf_count)
            {
                buffer_count_ += sol_buf_count;
                net_count_++;
            }
            if (options->use_best_solution_threshold)
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
            float old_delay =
                handler.gateDelay(pin, no_buff_tree->totalCapacitance());
            float old_slack = no_buff_tree->totalRequiredOrSlew() - old_delay;

            float new_delay =
                handler.gateDelay(pin, buff_tree->totalCapacitance());
            float new_slack = buff_tree->totalRequiredOrSlew() - new_delay;

            float gain = new_slack - old_slack;
            saved_slack_ += gain;

            BufferSolution::topDown(psn_inst, pin, buff_tree, current_area_,
                                    net_index_, buff_index_, added_buffers,
                                    affected_nets);

            if (replace_driver)
            {
                handler.replaceInstance(driver_cell, replace_driver);
                current_area_ -= handler.area(driver_lib);
                current_area_ += handler.area(replace_driver);
                resize_count_++;
            }
            for (auto& net : affected_nets)
            {
                handler.calculateParasitics(net);
            }
            return added_buffers;
        }
        else
        {
            PSN_LOG_DEBUG("Discarding weak solution: ");

            buff_tree->logDebug();
        }
    }
    return added_buffers;
}

int
TimingBufferTransform::fixCapacitanceViolations(
    Psn* psn_inst, std::vector<InstanceTerm*>& driver_pins,
    std::unique_ptr<OptimizationOptions>& options)
{
    PSN_LOG_DEBUG("Fixing capacitance violations");

    DatabaseHandler& handler           = *(psn_inst->handler());
    auto             clock_nets        = handler.clockNets();
    int              last_buffer_count = buffer_count_;
    for (auto& pin : driver_pins)
    {
        auto pin_net = handler.net(pin);
        if (pin_net && !clock_nets.count(pin_net))
        {
            auto vio = handler.hasElectricalViolation(pin);
            if (vio == ElectircalViolation::Capacitance ||
                vio == ElectircalViolation::CapacitanceAndTransition)
            {
                PSN_LOG_DEBUG("Fixing cap. violations for pin",
                              handler.name(pin));

                bufferPin(psn_inst, pin, RepairTarget::RepairMaxCapacitance,
                          options);
                if (options->legalization_frequency >
                    (buffer_count_ - last_buffer_count >=
                     options->legalization_frequency))
                {
                    last_buffer_count = buffer_count_;
                    handler.legalize();
                }

                if (handler.hasMaximumArea() &&
                    current_area_ > handler.maximumArea())
                {
                    PSN_LOG_WARN("Maximum utilization reached");

                    return buffer_count_ + resize_count_;
                }
            }
        }
    }

    return buffer_count_;
}

int
TimingBufferTransform::fixNegativeSlack(
    Psn* psn_inst, std::vector<InstanceTerm*>& driver_pins,
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
    int                               iteration         = 0;
    int                               last_buffer_count = buffer_count_;

    // NOTE: This can be done in parallel..
    int unfixed_paths = 0;
    for (size_t i = 0; i < negative_slack_paths.size(); i++)
    {
        auto& pth = negative_slack_paths[i];
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
                if (!buffered_pins.count(pin))
                {
                    if (handler.isAnyOutput(pin))
                    {
                        bufferPin(psn_inst, pin, RepairTarget::RepairSlack,
                                  options);
                        if (options->legalization_frequency >
                            (buffer_count_ - last_buffer_count >=
                             options->legalization_frequency))
                        {
                            last_buffer_count = buffer_count_;
                            handler.legalize();
                        }

                        if (handler.hasMaximumArea() &&
                            current_area_ > handler.maximumArea())
                        {
                            PSN_LOG_WARN("Maximum utilization reached");

                            return buffer_count_ + resize_count_;
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

    return buffer_count_ + resize_count_;
    ;
}
int
TimingBufferTransform::fixTransitionViolations(
    Psn* psn_inst, std::vector<InstanceTerm*>& driver_pins,
    std::unique_ptr<OptimizationOptions>& options)
{
    PSN_LOG_DEBUG("Fixing transition violations");

    DatabaseHandler& handler = *(psn_inst->handler());
    handler.resetDelays();
    auto clock_nets        = handler.clockNets();
    int  last_buffer_count = buffer_count_;
    for (auto& pin : driver_pins)
    {
        auto pin_net = handler.net(pin);

        if (pin_net && !clock_nets.count(pin_net))
        {
            auto net_pins = handler.pins(pin_net);
            auto vio      = handler.hasElectricalViolation(pin);
            if (vio == ElectircalViolation::Transition ||
                vio == ElectircalViolation::CapacitanceAndTransition)
            {
                PSN_LOG_DEBUG("Fixing transition violations for pin",
                              handler.name(pin));

                auto added_buffers = bufferPin(
                    psn_inst, pin, RepairTarget::RepairMaxTransition, options);

                if (options->legalization_frequency > 0 &&
                    (buffer_count_ - last_buffer_count >=
                     options->legalization_frequency))
                {
                    last_buffer_count = buffer_count_;
                    handler.legalize();
                }
                if (handler.hasMaximumArea() &&
                    current_area_ > handler.maximumArea())
                {
                    PSN_LOG_WARN("Maximum utilization reached");

                    return buffer_count_ + resize_count_;
                }
            }
        }
    }
    return buffer_count_;
}

int
TimingBufferTransform::timingBuffer(
    Psn* psn_inst, std::unique_ptr<OptimizationOptions>& options,
    std::unordered_set<std::string> buffer_lib_names,
    std::unordered_set<std::string> inverter_lib_names)
{
    DatabaseHandler& handler = *(psn_inst->handler());

    if (options->cluster_buffers)
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
    std::unique(options->buffer_lib.begin(), options->buffer_lib.end());
    std::unique(options->inverter_lib.begin(), options->inverter_lib.end());
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

    handler.buildLibraryMappings(4, options->buffer_lib, options->inverter_lib);

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

    PSN_LOG_INFO("Driver sizing:",
                 options->driver_resize ? "enabled" : "disabled");

    PSN_LOG_INFO("Mode:", options->timerless ? "Timerless" : "Timing-Driven");

    for (int i = 0; i < options->max_iterations; i++)
    {
        PSN_LOG_INFO("Iteration", i + 1);

        bool hasVio         = false;
        int  pre_fix_buff   = buffer_count_;
        int  pre_fix_resize = resize_count_;
        if (options->repair_negative_slack)
        {
            auto driver_pins = handler.levelDriverPins(true);
            fixNegativeSlack(psn_inst, driver_pins, options);
            if (buffer_count_ != pre_fix_buff ||
                resize_count_ != pre_fix_resize)
            {
                hasVio = true;
            }
            if (options->legalization_frequency > 0)
            {
                handler.legalize();
            }
        }
        if (options->repair_capacitance_violations)
        {
            pre_fix_buff     = buffer_count_;
            pre_fix_resize   = resize_count_;
            auto driver_pins = handler.levelDriverPins(true);

            fixCapacitanceViolations(psn_inst, driver_pins, options);
            if (buffer_count_ != pre_fix_buff ||
                resize_count_ != pre_fix_resize)
            {
                hasVio = true;
            }
            if (options->legalization_frequency > 0)
            {
                handler.legalize();
            }
        }
        if (options->repair_transition_violations)
        {
            pre_fix_buff     = buffer_count_;
            pre_fix_resize   = resize_count_;
            auto driver_pins = handler.levelDriverPins(true);
            fixTransitionViolations(psn_inst, driver_pins, options);
            if (buffer_count_ != pre_fix_buff ||
                resize_count_ != pre_fix_resize)
            {
                hasVio = true;
            }
            if (options->legalization_frequency > 0)
            {
                handler.legalize();
            }
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
    PSN_LOG_INFO("Initial area:",
                 handler.unitScaledArea(options->initial_area));

    PSN_LOG_INFO("New area:", handler.unitScaledArea(current_area_));

    if (options->repair_capacitance_violations)
    {
        PSN_LOG_INFO("Found", capacitance_violations_,
                     "maximum capacitance violations");
    }
    if (options->repair_transition_violations)
    {
        PSN_LOG_INFO("Found", transition_violations_,
                     "maximum transition violations");
    }
    PSN_LOG_INFO("Placed", buffer_count_, "buffers");

    PSN_LOG_INFO("Resized", resize_count_, "gates");

    if (options->timerless)
    {
        PSN_LOG_INFO("Ripped and rebuffered", timerless_rebuffer_count_,
                     "nets");
    }
    PSN_LOG_INFO("Buffered", net_count_, "nets");
    psn_inst->handler()->notifyDesignAreaChanged();

    return buffer_count_ + resize_count_;
}

int
TimingBufferTransform::run(Psn* psn_inst, std::vector<std::string> args)
{
    buffer_count_             = 0;
    resize_count_             = 0;
    timerless_rebuffer_count_ = 0;
    net_count_                = 0;
    resynth_count_            = 0;
    clone_count_              = 0;
    slack_violations_         = 0;
    current_area_             = psn_inst->handler()->area();
    saved_slack_              = 0.0;
    capacitance_violations_ =
        psn_inst->handler()->maximumCapacitanceViolations().size();
    transition_violations_ =
        psn_inst->handler()->maximumTransitionViolations().size();

    std::unique_ptr<OptimizationOptions> options(new OptimizationOptions);

    options->initial_area = current_area_;

    std::unordered_set<std::string> buffer_lib_names;
    std::unordered_set<std::string> inverter_lib_names;
    std::unordered_set<std::string> keywords(
        {"-buffers", "-inverters", "-enable_driver_resize", "-iterations",
         "-minimum_gain", "-area_penalty", "-auto_buffer_library",
         "-minimize_buffer_library", "-use_inverting_buffer_library",
         "-timerless", "-repair_by_resynthesis", "-post_global_place",
         "-post_place", "-post_route", "-legalization_frequency",
         "-high_effort"});
    bool high_effort;
    if (args.size() < 2)
    {
        PSN_LOG_ERROR(help());

        return -1;
    }
    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i].size() > 2 && args[i][0] == '-' && args[i][1] == '-')
        {
            args[i] = args[i].substr(1);
        }
    }
    for (size_t i = 0; i < args.size(); i++)
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
        else if (args[i] == "-legalization_frequency")
        {
            i++;
            if (i >= args.size() || !StringUtils::isNumber(args[i]))
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
            else
            {
                options->legalization_frequency = atoi(args[i].c_str());
            }
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
        else if (args[i] == "-enable_driver_resize")
        {
            options->driver_resize = true;
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
        else if (args[i] == "-timerless")
        {
            options->timerless = true;
        }
        else if (args[i] == "-minimize_buffer_library")
        {
            options->minimize_cluster_buffers = true;
        }
        else if (args[i] == "-use_inverting_buffer_library")
        {
            options->cluster_inverters = true;
        }
        else if (args[i] == "-repair_by_resynthesis")
        {
            options->repair_by_resynthesis = true;
        }
        else if (args[i] == "-high_effort")
        {
            high_effort = true;
        }
        else if (args[i] == "-post_place")
        {
            options->phase = DesignPhase::PostPlace;
        }
        else if (args[i] == "-post_route")
        {
            PSN_LOG_ERROR("Post routing timing repair is not supported");
            options->phase = DesignPhase::PostRoute;
            return -1;
        }
        else
        {
            PSN_LOG_ERROR(help());
            return -1;
        }
    }
    if (high_effort)
    {
        options->minimum_upstream_resistance = 120;
    }
    else
    {
        options->minimum_upstream_resistance = 600;
    }

    if (options->timerless)
    {
        if (options->repair_capacitance_violations ||
            options->repair_negative_slack)
        {
            PSN_LOG_ERROR("Timerless mode cannot be used to repair maximum "
                          "capacitance violations or negative slack");
            return -1;
        }
        options->repair_transition_violations  = true;
        options->repair_capacitance_violations = false;
        options->repair_negative_slack         = false;
    }
    else if (!options->repair_capacitance_violations &&
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
    if (options->timerless &&
        (options->cluster_inverters || inverter_lib_names.size()))
    {
        PSN_LOG_ERROR("Cannot use inverting buffer library in timerless mode");
        return -1;
    }
    PSN_LOG_DEBUG("repair_timing", StringUtils::join(args, " "));

    return timingBuffer(psn_inst, options, buffer_lib_names,
                        inverter_lib_names);
}

} // namespace psn
