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

#include "GateCloningTransform.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include "OpenPhySyn/Psn.hpp"
#include "OpenPhySyn/PsnGlobal.hpp"
#include "OpenPhySyn/PsnLogger.hpp"
#include "OpenPhySyn/StringUtils.hpp"
#include "db_sta/dbSta.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Parasitics.hh"
#include "sta/Search.hh"

namespace psn
{

GateCloningTransform::GateCloningTransform() : net_index_(0), clone_index_(0)
{
}
int
GateCloningTransform::gateClone(Psn* psn_inst, float cap_factor,
                                bool clone_largest_only)
{
    clone_count_             = 0;
    DatabaseHandler& handler = *(psn_inst->handler());
    PSN_LOG_DEBUG("Clone", cap_factor, clone_largest_only);

    std::vector<InstanceTerm*> level_drvrs = handler.levelDriverPins(true);
    for (auto& pin : level_drvrs)
    {
        Instance* inst = handler.instance(pin);
        if (handler.isSingleOutputCombinational(inst))
        {
            cloneTree(psn_inst, inst, cap_factor, clone_largest_only);
        }
    }
    psn_inst->handler()->notifyDesignAreaChanged();
    return clone_count_;
}
void
GateCloningTransform::cloneTree(Psn* psn_inst, Instance* inst, float cap_factor,
                                bool clone_largest_only)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    float cap_per_micron     = psn_inst->handler()->capacitancePerMicron();

    auto output_pins = handler.outputPins(inst);
    if (!output_pins.size())
    {
        return;
    }
    InstanceTerm* output_pin = *(output_pins.begin());
    Net*          net        = handler.net(output_pin);
    if (!net)
    {
        return;
    }
    std::unique_ptr<SteinerTree> tree = SteinerTree::create(net, psn_inst);
    if (tree == nullptr)
    {
        return;
    }
    auto driver_lib = handler.libraryCell(inst);

    float        total_net_load = tree->totalLoad(cap_per_micron);
    LibraryCell* cell           = handler.libraryCell(inst);

    float output_target_load = handler.targetLoad(cell);

    float c_limit = cap_factor * output_target_load;

    if (!handler.violatesMaximumTransition(output_pin) &&
        !handler.violatesMaximumCapacitance(output_pin))
    {
        return;
    }
    auto  slew = handler.slew(output_pin);
    auto  cap  = handler.loadCapacitance(output_pin);
    bool  exists;
    auto  slew_limit = handler.pinSlewLimit(output_pin, &exists);
    auto  cap_limit  = handler.maxLoad(handler.libraryPin(output_pin));
    float slew_ratio = slew / slew_limit;
    auto  cap_ratio  = cap / cap_limit;

    PSN_LOG_TRACE(handler.name(inst), handler.name(cell),
                  "output_target_load:", output_target_load);

    PSN_LOG_TRACE(handler.name(inst), handler.name(cell),
                  "cap_per_micron:", cap_per_micron);

    PSN_LOG_TRACE(handler.name(inst), handler.name(cell), "c_limit:", c_limit);

    PSN_LOG_TRACE(handler.name(inst), handler.name(cell),
                  "total_net_load:", total_net_load);

    if (slew_ratio < 1.4 && cap_ratio < 1.4)
    {
        return;
    }
    clone_largest_only = false;

    if (clone_largest_only && cell != handler.largestLibraryCell(cell))
    {
        PSN_LOG_TRACE(handler.name(inst), handler.name(cell),
                      "is not the largest cell");

        return;
    }
    int fanout_count = handler.fanoutPins(net).size();

    if (fanout_count <= 1)
    {
        return;
    }

    auto half_drvr = handler.halfDrivingPowerCell(driver_lib);
    if (half_drvr == driver_lib)
    {
        return;
    }

    PSN_LOG_DEBUG("Cloning", handler.name(inst), handler.name(cell));

    auto prec          = clone_count_;
    output_target_load = handler.targetLoad(half_drvr);

    c_limit = cap_factor * output_target_load;

    topDownClone(psn_inst, tree, tree->top(), tree->driverPoint(), c_limit,
                 half_drvr);
    auto postc = clone_count_;
    if (prec != postc)
    {
        handler.replaceInstance(inst, half_drvr);
    }
}
void
GateCloningTransform::topDownClone(Psn*                          psn_inst,
                                   std::unique_ptr<SteinerTree>& tree,
                                   SteinerPoint k, SteinerPoint prev,
                                   float c_limit, LibraryCell* driver_cell)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    float cap_per_micron     = psn_inst->handler()->capacitancePerMicron();

    SteinerPoint drvr = tree->driverPoint();

    float src_wire_len = handler.dbuToMeters(tree->distance(drvr, k));
    float src_wire_cap = src_wire_len * cap_per_micron;
    if (src_wire_cap > c_limit)
    {
        return;
    }

    SteinerPoint left  = tree->left(k);
    SteinerPoint right = tree->right(k);
    if (left != SteinerNull)
    {
        float cap_left = tree->subtreeLoad(cap_per_micron, left) + src_wire_cap;
        bool  is_leaf =
            tree->left(left) == SteinerNull && tree->right(left) == SteinerNull;
        if (cap_left < c_limit || is_leaf)
        {
            cloneInstance(psn_inst, tree, left, k, driver_cell);
        }
        else
        {
            topDownClone(psn_inst, tree, left, k, c_limit, driver_cell);
        }
    }

    if (right != SteinerNull)
    {
        float cap_right =
            tree->subtreeLoad(cap_per_micron, right) + src_wire_cap;
        bool is_leaf = tree->left(right) == SteinerNull &&
                       tree->right(right) == SteinerNull;
        if (cap_right < c_limit || is_leaf)
        {
            cloneInstance(psn_inst, tree, right, k, driver_cell);
        }
        else
        {
            topDownClone(psn_inst, tree, right, k, c_limit, driver_cell);
        }
    }
}
void
GateCloningTransform::topDownConnect(Psn*                          psn_inst,
                                     std::unique_ptr<SteinerTree>& tree,
                                     SteinerPoint k, Net* net)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    if (k == SteinerNull)
    {
        return;
    }

    if (tree->left(k) == SteinerNull && tree->right(k) == SteinerNull)
    {
        handler.connect(net, tree->pin(k));
    }
    else
    {
        topDownConnect(psn_inst, tree, tree->left(k), net);
        topDownConnect(psn_inst, tree, tree->right(k), net);
    }
}
void
GateCloningTransform::cloneInstance(Psn*                          psn_inst,
                                    std::unique_ptr<SteinerTree>& tree,
                                    SteinerPoint k, SteinerPoint prev,
                                    LibraryCell* driver_cell)
{
    DatabaseHandler& handler    = *(psn_inst->handler());
    SteinerPoint     drvr       = tree->driverPoint();
    auto             output_pin = tree->pin(drvr);
    auto             inst       = handler.instance(output_pin);
    Net*             output_net = handler.net(output_pin);
    handler.calculateParasitics(output_net);
    handler.sta()->ensureLevelized();
    handler.sta()->findRequireds();
    handler.sta()->findDelays();

    auto wp = handler.worstSlackPath(output_pin);
    if (!wp.size())
    {
        return;
    }
    auto preslack = handler.worstSlack(wp[wp.size() - 1].pin());
    auto ws       = handler.worstSlack();

    std::string clone_net_name = handler.generateNetName(net_index_);
    Net*        clone_net      = handler.createNet(clone_net_name.c_str());
    auto        output_port    = handler.libraryPin(output_pin);

    topDownConnect(psn_inst, tree, k, clone_net);
    std::unordered_set<Net*> para_nets;
    para_nets.insert(clone_net);
    para_nets.insert(output_net);
    handler.calculateParasitics(clone_net);

    int fanout_count = handler.fanoutPins(handler.net(output_pin)).size();
    if (fanout_count == 0)
    {
        handler.disconnectAll(clone_net);
        topDownConnect(psn_inst, tree, k, output_net);
        handler.del(clone_net);
        para_nets.erase(clone_net);
    }
    else
    {
        if (wp.size())
        {

            std::string instance_name =
                handler.generateInstanceName("clone_", clone_index_);

            Instance* cloned_inst =
                handler.createInstance(instance_name.c_str(), driver_cell);
            handler.setLocation(cloned_inst, tree->location(prev));
            handler.connect(clone_net, cloned_inst, output_port);

            auto pins = handler.pins(inst);
            for (auto& p : pins)
            {
                handler.sta()->delaysInvalidFrom(p);
                handler.sta()->delaysInvalidFrom(cloned_inst);
                handler.sta()->delaysInvalidFromFanin(p);
                if (handler.isInput(p) && p != output_pin)
                {
                    Net* target_net  = handler.net(p);
                    auto target_port = handler.libraryPin(p);
                    handler.connect(target_net, cloned_inst, target_port);
                    para_nets.insert(target_net);
                }
            }

            for (auto& net : para_nets)
            {
                handler.calculateParasitics(net);
            }

            handler.sta()->ensureLevelized();
            handler.sta()->findRequireds();
            handler.sta()->findDelays();

            wp = handler.worstSlackPath(output_pin);

            auto new_ws = handler.worstSlack();

            float postslack = preslack;
            if (wp.size())
            {
                postslack = handler.worstSlack(wp[wp.size() - 1].pin());
            }

            if (postslack <= preslack || new_ws < ws)
            {

                auto clone_pin     = handler.outputPins(cloned_inst)[0];
                auto clone_out_net = handler.net(clone_pin);
                auto clone_sinks   = handler.fanoutPins(clone_out_net, true);

                handler.disconnectAll(clone_out_net);
                for (auto& p : clone_sinks)
                {
                    handler.connect(output_net, p);
                }

                handler.del(clone_net);

                para_nets.erase(clone_net);

                handler.del(cloned_inst);

                for (auto& net : para_nets)
                {
                    handler.calculateParasitics(net);
                }

                handler.sta()->graphDelayCalc()->delaysInvalid();
            }
            else
            {

                clone_count_++;
            }
        }
        else
        {

            handler.disconnectAll(clone_net);
            topDownConnect(psn_inst, tree, k, output_net);
            handler.del(clone_net);
            para_nets.erase(clone_net);
        }
    }

    for (auto& net : para_nets)
    {
        handler.calculateParasitics(net);
    }
}

int
GateCloningTransform::run(Psn* psn_inst, std::vector<std::string> args)
{
    if (args.size() > 2)
    {
        PSN_LOG_ERROR(help());

        return -1;
    }
    float cap_factor         = 1.4;
    bool  clone_largest_only = false;
    if (args.size() >= 1)
    {
        if (!StringUtils::isNumber(args[0]))
        {
            PSN_LOG_ERROR(help());

            return -1;
        }
        cap_factor = std::stof(args[0].c_str());
        if (args.size() >= 2)
        {
            if (StringUtils::isTruthy(args[1]))
            {
                clone_largest_only = true;
            }
            else if (StringUtils::isFalsy(args[1]))
            {
                clone_largest_only = false;
            }
            else
            {
                PSN_LOG_ERROR(help());

                return -1;
            }
        }
    }
    psn_inst->handler()->sta()->search()->endpointsInvalid();
    psn_inst->handler()->sta()->ensureLevelized();
    psn_inst->handler()->sta()->findRequireds();
    psn_inst->handler()->sta()->checkCapacitanceLimitPreamble();
    psn_inst->handler()->sta()->checkSlewLimitPreamble();

    int rc = gateClone(psn_inst, cap_factor, clone_largest_only);

    return rc;
}

} // namespace psn
