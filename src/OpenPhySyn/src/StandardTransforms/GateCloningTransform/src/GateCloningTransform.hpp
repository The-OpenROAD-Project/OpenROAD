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
#ifdef OPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED
#ifndef __PSN_GATE_CLONING_TRANSFORM__
#define __PSN_GATE_CLONING_TRANSFORM__

#include <OpenPhySyn/DatabaseHandler.hpp>
#include <OpenPhySyn/Psn.hpp>
#include <OpenPhySyn/PsnTransform.hpp>
#include <OpenPhySyn/SteinerTree.hpp>
#include <OpenPhySyn/Types.hpp>
#include <cstring>
#include <memory>

namespace psn {
class GateCloningTransform : public PsnTransform
{
private:
    void cloneTree(Psn* psn_inst, Instance* inst, float cap_factor,
                   bool clone_largest_only);
    void topDownClone(Psn*                          psn_inst,
                      std::unique_ptr<SteinerTree>& tree,
                      SteinerPoint k, float c_limit);
    void topDownConnect(Psn*                          psn_inst,
                        std::unique_ptr<SteinerTree>& tree,
                        SteinerPoint k, Net* net);
    void cloneInstance(Psn*                          psn_inst,
                       std::unique_ptr<SteinerTree>& tree,
                       SteinerPoint                  k);
    std::string makeUniqueNetName(Psn* psn_inst);
    std::string makeUniqueCloneName(Psn* psn_inst);

    int net_index_;
    int clone_index_;
    int clone_count_;

public:
    GateCloningTransform();
    int gateClone(Psn* psn_inst, float cap_factor,
                  bool clone_largest_only);

    int run(Psn* psn_inst, std::vector<std::string> args) override;
#ifdef OPENPHYSYN_AUTO_LINK
    const char* help() override;
    const char* version() override;
    const char* name() override;
    const char* description() override;
    std::shared_ptr<psn::PsnTransform> load() override;
#endif
};


DEFINE_TRANSFORM(GateCloningTransform, "gate_clone", "1.0.0",
                 "Performs load-driven gate cloning",
                 "Usage: transform gate_clone "
                 "<float: max-cap-factor> <boolean: clone-gates-only>")
}
#endif
#endif