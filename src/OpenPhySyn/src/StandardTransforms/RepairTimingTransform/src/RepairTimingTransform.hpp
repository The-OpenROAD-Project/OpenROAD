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

#include <cstring>
#include <memory>
#include <unordered_set>
#include "OpenPhySyn/BufferTree.hpp"
#include "OpenPhySyn/LibraryMapping.hpp"
#include "OpenPhySyn/Psn.hpp"
#include "OpenPhySyn/PsnTransform.hpp"
#include "OpenPhySyn/SteinerTree.hpp"
#include "OpenPhySyn/Types.hpp"

namespace psn
{

class BufferSolution;
class OptimizationOptions;

class RepairTimingTransform : public PsnTransform
{
private:
    int buffer_count_;           // Number of inserted buffers
    int resize_up_count_;        // Number of upsized cells
    int resize_down_count_;      // Number of downsized cells
    int pin_swap_count_;         // Number of applied pin-swaps
    int net_count_;              // Number of repaired nets
    int net_index_;              // Used for naming new nets
    int buff_index_;             // Used for naming new cells
    int transition_violations_;  // Number of repaired (or attempted) transition
                                 // violations
    int capacitance_violations_; // Number of repaired (or attempted)
                                 // capacitance violations
    float current_area_;         // Incremental area holder
    float saved_slack_;          // Total slack gain

    // Repair a single pin
    std::unordered_set<Instance*>
    repairPin(Psn* psn_inst, InstanceTerm* pin, RepairTarget target,
              std::unique_ptr<OptimizationOptions>& options);

    // Number of applied design edit
    int getEditCount() const;

    // Run the whole repair flow
    int repairTiming(Psn*                                  psn_inst,
                     std::unique_ptr<OptimizationOptions>& options,
                     std::unordered_set<std::string>       buffer_lib_names =
                         std::unordered_set<std::string>(),
                     std::unordered_set<std::string> inverter_lib_names =
                         std::unordered_set<std::string>(),
                     std::unordered_set<std::string> pin_names =
                         std::unordered_set<std::string>());

    // Repair paths with maximum capacitance violations
    int fixCapacitanceViolations(Psn*                        psn_inst,
                                 std::vector<InstanceTerm*>& driver_pins,
                                 std::unique_ptr<OptimizationOptions>& options);

    // Repair paths with maximum transition violations
    int fixTransitionViolations(Psn*                        psn_inst,
                                std::vector<InstanceTerm*>& driver_pins,
                                std::unique_ptr<OptimizationOptions>& options);

    // Repair paths with negative slack
    int fixNegativeSlack(Psn*                                  psn_inst,
                         std::unordered_set<InstanceTerm*>&    filter_pins,
                         std::unique_ptr<OptimizationOptions>& options);

    // Final downsizing phases for any extra area recovery
    int resizeDown(Psn* psn_inst, std::vector<InstanceTerm*>& driver_pins,
                   std::unique_ptr<OptimizationOptions>& options);

    // Downsize a single pin
    void resizeDown(Psn* psn_inst, InstanceTerm* pin,
                    std::unique_ptr<OptimizationOptions>& options);

public:
    RepairTimingTransform();

    // Run the transform and capture user configurations
    int run(Psn* psn_inst, std::vector<std::string> args) override;
    OPENPHYSYN_DEFINE_TRANSFORM(
        "repair_timing", "1.2",
        "Repair design timing and electrical violations",
        "Usage: transform repair_timing [-capacitance_violations] "
        "[-transition_violations] [-negative_slack_violations] [-iterations "
        "num_iterations] [-buffers buffer_cells] [-inverters inverter cells] "
        "[-minimum_gain gain=0.0] [-auto_buffer_library "
        "<single|small|medium|large|all>] [-no_minimize_buffer_library] "
        "[-auto_buffer_library_inverters_enabled]  [-buffer_disabled] "
        "[-minimum_cost_buffer_enabled] [-resize_disabled] "
        "[-pin_swap_disabled] "
        "[-downsize_enabled] [-legalize_eventually] [-legalize_each_iteration] "
        "[-post_place|-post_route] [-legalization_frequency <num_edits>] "
        "[-high_effort] [-capacitance_pessimism_factor factor] "
        "[-transition_pessimism_factor factor] [-pins <pin names>] "
        "[-maximum_negative_slack_paths count] "
        "[-maximum_negative_slack_path_depth count]")
};

} // namespace psn
