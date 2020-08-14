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
#include "OpenPhySyn/DatabaseHandler.hpp"
#include "OpenPhySyn/LibraryMapping.hpp"
#include "OpenPhySyn/Psn.hpp"
#include "OpenPhySyn/PsnTransform.hpp"
#include "OpenPhySyn/SteinerTree.hpp"
#include "OpenPhySyn/Types.hpp"

namespace psn
{

class BufferSolution;
class OptimizationOptions;

class TimingBufferTransform : public PsnTransform
{

private:
    int   buffer_count_;
    int   resize_count_;
    int   clone_count_;
    int   resynth_count_;
    int   net_count_;
    int   timerless_rebuffer_count_;
    int   net_index_;
    int   buff_index_;
    int   clone_index_;
    int   transition_violations_;
    int   capacitance_violations_;
    int   slack_violations_;
    float current_area_;
    float saved_slack_;
    std::unordered_set<Instance*>
    bufferPin(Psn* psn_inst, InstanceTerm* pin, RepairTarget target,
              std::unique_ptr<OptimizationOptions>& options);

    int timingBuffer(Psn*                                  psn_inst,
                     std::unique_ptr<OptimizationOptions>& options,
                     std::unordered_set<std::string>       buffer_lib_names =
                         std::unordered_set<std::string>(),
                     std::unordered_set<std::string> inverter_lib_names =
                         std::unordered_set<std::string>());
    int fixCapacitanceViolations(Psn*                        psn_inst,
                                 std::vector<InstanceTerm*>& driver_pins,
                                 std::unique_ptr<OptimizationOptions>& options);
    int fixTransitionViolations(Psn*                        psn_inst,
                                std::vector<InstanceTerm*>& driver_pins,
                                std::unique_ptr<OptimizationOptions>& options);
    int fixNegativeSlack(Psn* psn_inst, std::vector<InstanceTerm*>& driver_pins,
                         std::unique_ptr<OptimizationOptions>& options);

public:
    TimingBufferTransform();

    int run(Psn* psn_inst, std::vector<std::string> args) override;

    OPENPHYSYN_DEFINE_TRANSFORM(
        "timing_buffer", "1.5",
        "Performs several variations of buffering and resizing to fix timing "
        "violations",
        "Usage: transform timing_buffer [-capacitance_violations] "
        "[-transition_violations] [-negative_slack_violations] "
        "[-auto_buffer_library <single|small|medium|large|all>] "
        "[-minimize_buffer_library] [-use_inverting_buffer_library] [-buffers "
        "buffer_cells] [-inverters invereter_cells] [-repair_by_resynthesis] "
        "[-iterations num_iterations=1>] [-post_place|-post_route] "
        "[-legalization_frequency num_edits][-minimum_gain gain=0] "
        "[-enable_gate_resize] [-area_penalty penalty]")
};

} // namespace psn
