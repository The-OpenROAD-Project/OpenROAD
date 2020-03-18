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
#ifdef OPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED
#ifndef __PSN_CONSTANT_PROPAGATION_TRANSFORM__
#define __PSN_CONSTANT_PROPAGATION_TRANSFORM__

#include <OpenPhySyn/DatabaseHandler.hpp>
#include <OpenPhySyn/Types.hpp>
#include <OpenPhySyn/Psn.hpp>
#include <OpenPhySyn/SteinerTree.hpp>
#include <OpenPhySyn/PsnTransform.hpp>
#include <cstring>
#include <memory>
#include <unordered_set>

namespace psn
{
class ConstantPropagationTransform : public PsnTransform
{
private:
    bool isNumber(const std::string& s);
    int  prop_count_;

    int  propagateConstants(Psn* psn_inst, std::string tiehi_cell_name,
                            std::string tielo_cell_name,
                            std::string inverter_cell_name, int max_depth,
                            bool invereter_replace);
    void propagateTieHiLoCell(
        Psn* psn_inst, bool is_tiehi, InstanceTerm* constant_term,
        int max_depth, bool invereter_replace, Instance* tiehi_cell,
        Instance* tielo_cell, LibraryCell* inverter_lib_cell,
        LibraryCell* smallest_buffer_lib_cell, LibraryCell* tiehi_lib_cell,
        LibraryCell* tielo_lib_cell, std::unordered_set<Instance*>& visited,
        std::unordered_set<Instance*>&     deleted_inst,
        std::unordered_set<InstanceTerm*>& deleted_pins);
    int isTiedToConstant(Psn* psn_inst, InstanceTerm* constant_term,
                         bool constant_val);
    int isTiedToInput(Psn* psn_inst, InstanceTerm* input_term,
                      InstanceTerm* constant_term, bool constant_val);

public:
    ConstantPropagationTransform();

    int run(Psn* psn_inst, std::vector<std::string> args) override;
#ifdef OPENPHYSYN_AUTO_LINK
    const char* help() override;
    const char* version() override;
    const char* name() override;
    const char* description() override;
    std::shared_ptr<psn::PsnTransform> load() override;
#endif
};

DEFINE_TRANSFORM(
    ConstantPropagationTransform, "constant_propagation", "1.0.0",
    "Performs design optimization through constant propagation",
    "Usage: transform constant_propagation [enable-inverter-replacement] "
    "[max-depth] [tie-hi cell] [tie-lo]"
    "cell] [inverter_cell]")
} // namespace psn

#endif
#endif