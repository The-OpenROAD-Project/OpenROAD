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
#ifdef OPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED
#include "PinSwapTransform.hpp"
#include <OpenPhySyn/PathPoint.hpp>
#include <OpenPhySyn/PsnGlobal.hpp>
#include <OpenPhySyn/PsnLogger.hpp>
#include <OpenSTA/dcalc/ArcDelayCalc.hh>
#include <OpenSTA/dcalc/GraphDelayCalc.hh>
#include <OpenSTA/liberty/TimingArc.hh>
#include <OpenSTA/liberty/TimingModel.hh>
#include <OpenSTA/liberty/TimingRole.hh>
#include <OpenSTA/search/Corner.hh>

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

using namespace psn;

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
        PSN_LOG_DEBUG("Optimizing path {}/{}", path_index++, paths.size());
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
                        PSN_LOG_DEBUG("Accepted Swap: {} <-> {}",
                                      handler.name(pin), handler.name(in_pin));
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

    return swap_count_;
}
int
PinSwapTransform::timingPinSwap(psn::Psn* psn_inst)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    auto             cp      = handler.criticalPath();
    std::reverse(cp.begin(), cp.end());

    for (auto& point : cp)
    {

        auto pin      = point.pin();
        auto is_rise  = point.isRise();
        auto inst     = handler.instance(pin);
        auto ap_index = point.analysisPointIndex();

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
                float current_arrival =
                    handler.arrival(out_pin, ap_index, is_rise);
                handler.swapPins(pin, in_pin);
                float new_arrival = handler.arrival(out_pin, ap_index, is_rise);
                if (new_arrival < current_arrival)
                {
                    PSN_LOG_DEBUG("Accepted Swap: {} <-> {}", handler.name(pin),
                                  handler.name(in_pin));
                    swap_count_++;
                }
                else
                {
                    handler.swapPins(pin, in_pin);
                }
            }
        }
    }
    return swap_count_;
}

bool
PinSwapTransform::isNumber(const std::string& s)
{
    std::istringstream iss(s);
    float              f;
    iss >> std::noskipws >> f;
    return iss.eof() && !iss.fail();
}
int
PinSwapTransform::run(Psn* psn_inst, std::vector<std::string> args)
{
    bool power_opt           = false;
    int  max_power_opt_paths = 50;
    if (args.size() > 2)
    {
        PSN_LOG_ERROR(help());
        return -1;
    }
    else if (args.size())
    {
        std::transform(args[0].begin(), args[0].end(), args[0].begin(),
                       ::tolower);
        if (args[0] == "true" || args[0] == "1")
        {
            power_opt = true;
        }
        else if (args[0] == "false" || args[0] == "0")
        {
            power_opt = false;
        }
        else
        {
            PSN_LOG_ERROR(help());
            return -1;
        }
        if (args.size() > 1)
        {
            if (isNumber(args[1]))
            {
                max_power_opt_paths = std::stoi(args[1]);
            }
            else
            {
                PSN_LOG_ERROR(help());
                return -1;
            }
        }
    }
    swap_count_ = 0;
    if (power_opt)
    {
        return powerPinSwap(psn_inst, max_power_opt_paths);
    }
    else
    {
        return timingPinSwap(psn_inst);
    }
}
DEFINE_TRANSFORM_VIRTUALS(
    PinSwapTransform, "pin_swap", "1.0.0",
    "Performs timing-driven/power-driven commutative pin swapping optimization",
    "Usage: transform pin_swap [optimize_power] [max_num_optimize_power_paths]")

#endif