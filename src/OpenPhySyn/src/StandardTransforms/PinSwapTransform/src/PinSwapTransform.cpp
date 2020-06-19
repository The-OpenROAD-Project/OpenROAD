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

#include "PinSwapTransform.hpp"
#include "OpenPhySyn/DatabaseHandler.hpp"
#include "OpenPhySyn/PathPoint.hpp"
#include "OpenPhySyn/PsnGlobal.hpp"
#include "OpenPhySyn/PsnLogger.hpp"
#include "OpenPhySyn/StringUtils.hpp"
#include "db_sta/dbSta.hh"
#include "sta/Graph.hh"
#include "sta/Search.hh"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace psn
{

PinSwapTransform::PinSwapTransform() : swap_count_(0)
{
}

int
PinSwapTransform::powerPinSwap(psn::Psn* psn_inst, int path_count)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    PSN_LOG_WARN("This is an experimental transform that might negatively "
                 "affect your timing, use at your own risk.");

    auto paths      = handler.bestPath(path_count);
    int  path_index = 1;
    for (auto& path : paths)
    {
        PSN_LOG_DEBUG("Optimizing path", path_index++, "/", paths.size());

        for (auto& point : path)
        {
            auto pin     = point.pin();
            auto is_rise = point.isRise();
            auto inst    = handler.instance(pin);
            if (!handler.isInput(pin))
            {
                continue;
            }
            auto input_pins  = handler.inputPins(inst);
            auto output_pins = handler.outputPins(inst);

            if (input_pins.size() < 2 || output_pins.size() != 1)
            {
                continue;
            }

            auto out_pin = output_pins[0];
            for (auto& in_pin : input_pins)
            {
                if (in_pin != pin && handler.isCommutative(in_pin, pin))
                {
                    float current_slew = handler.slew(out_pin, is_rise);
                    handler.swapPins(pin, in_pin);
                    float new_slew = handler.slew(out_pin, is_rise);
                    if (new_slew > current_slew)
                    {
                        PSN_LOG_DEBUG("Accepted Swap:", handler.name(pin),
                                      "<->", handler.name(in_pin));

                        swap_count_++;
                    }
                    else
                    {
                        handler.swapPins(pin, in_pin);
                    }
                }
            }
        }
    }
    psn_inst->handler()->notifyDesignAreaChanged();

    return swap_count_;
}
int
PinSwapTransform::timingPinSwap(psn::Psn* psn_inst, int path_count)
{
    DatabaseHandler& handler     = *(psn_inst->handler());
    auto             driver_pins = handler.levelDriverPins(true);
    auto             cp          = handler.criticalPaths(path_count);

    int p_count = 0;
    for (auto& path : cp)
    {
        PSN_LOG_DEBUG("Path", p_count + 1, "/", cp.size());

        p_count++;
        for (auto& pt : path)
        {
            auto pin = pt.pin();
            if (pin && handler.isOutput(pin))
            {
                handler.sta()->vertexRequired(handler.vertex(pin),
                                              sta::MinMax::min());
                handler.sta()->findDelays(handler.vertex(pin));
                auto                     wp = handler.worstSlackPath(pin, true);
                std::unordered_set<Net*> affected_nets;
                auto                     driver_cell = handler.instance(pin);

                if (wp.size() > 1)
                {
                    auto inpin      = wp[wp.size() - 2].pin();
                    auto swap_pin   = inpin;
                    auto commu_pins = handler.commutativePins(inpin);
                    if (commu_pins.size())
                    {
                        auto  pre_swap_wp = handler.worstSlackPath(pin, true);
                        float pre_swap_slack = handler.worstSlack(
                            pre_swap_wp[pre_swap_wp.size() - 1].pin());

                        for (auto& cp : commu_pins)
                        {
                            handler.swapPins(swap_pin, cp);
                            handler.sta()->vertexRequired(handler.vertex(pin),
                                                          sta::MinMax::min());
                            handler.sta()->findDelays(handler.vertex(pin));
                            auto post_swap_wp =
                                handler.worstSlackPath(pin, true);
                            if (post_swap_wp.size())
                            {
                                float post_swap_slack = handler.worstSlack(
                                    post_swap_wp[post_swap_wp.size() - 1].pin());
                                if (post_swap_slack > pre_swap_slack)
                                {
                                    pre_swap_slack = post_swap_slack;
                                    swap_pin       = cp;
                                }
                                else
                                {
                                    handler.swapPins(swap_pin, cp);
                                    // handler.sta()->vertexRequired(
                                    //     handler.vertex(swap_pin),
                                    //     sta::MinMax::min());
                                    // handler.sta()->findDelays(handler.vertex(swap_pin));
                                    // handler.sta()->vertexRequired(handler.vertex(cp),
                                    //                               sta::MinMax::min());
                                    // handler.sta()->findDelays(handler.vertex(cp));
                                }
                            }
                        }
                        if (swap_pin != inpin)
                        {
                            swap_count_++;
                            std::vector<Net*> fanin_nets;
                            for (auto& fpin : handler.inputPins(driver_cell))
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
                            // is_fixed = !vio_check_func(pin);
                        }
                    }
                }
            }
        }
    }
    psn_inst->handler()->notifyDesignAreaChanged();
    return swap_count_;
}

int
PinSwapTransform::run(Psn* psn_inst, std::vector<std::string> args)
{
    bool power_opt     = false;
    int  max_opt_paths = 50;
    if (args.size() > 2 || args.size() < 1)
    {
        PSN_LOG_ERROR(help());

        return -1;
    }
    else if (args.size())
    {
        std::transform(args[0].begin(), args[0].end(), args[0].begin(),
                       ::tolower);
        for (auto& arg : args)
        {
            if (arg == "-power" || arg == "--power")
            {
                power_opt = true;
            }
            else if (StringUtils::isNumber(arg))
            {
                max_opt_paths = atoi(arg.c_str());
            }
            else
            {
                PSN_LOG_INFO("---", arg);
                PSN_LOG_ERROR(help());

                return -1;
            }
        }
    }
    swap_count_ = 0;
    if (power_opt)
    {
        return powerPinSwap(psn_inst, max_opt_paths);
    }
    else
    {
        return timingPinSwap(psn_inst, max_opt_paths);
    }
}
} // namespace psn
