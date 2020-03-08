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
#ifdef OPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED

#include "HelloTransform.hpp"
#include <OpenPhySyn/PsnLogger.hpp>
#include <algorithm>
#include <cmath>

using namespace psn;

int
HelloTransform::addWire(Psn* psn_inst, std::string name)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    Net*             n1      = handler.createNet(name.c_str());
    return (n1 != nullptr);
}

int
HelloTransform::run(Psn* psn_inst, std::vector<std::string> args)
{

    PSN_LOG_DEBUG("Passed arguments:");
    for (auto& arg : args)
    {
        PSN_LOG_DEBUG("{}", arg);
    }

    if (args.size() == 1)
    {
        std::string net_name = args[0];
        PSN_LOG_INFO("Adding random wire {}", net_name);
        return addWire(psn_inst, net_name);
    }
    else
    {
        PSN_LOG_ERROR("Usage:\n transform hello_transform "
                      "<net_name>\n");
    }

    return -1;
}
DEFINE_TRANSFORM_VIRTUALS(
    HelloTransform, "hello_transform", "1.0.0",
    "Hello transform, a toy transform that adds an unconnected net",
    "Usage:\n transform hello_transform "
    "<net_name>\n")
#endif