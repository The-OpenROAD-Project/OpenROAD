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
#include <OpenPhySyn/PathPoint.hpp>
#include "Psn.hpp"
#include "PsnException.hpp"

#include "FileUtils.hpp"
#include "Readers.hpp"
#include "doctest.h"

using namespace psn;

TEST_CASE("Should perform timing-driven pin swapping")
{
    Psn& psn_inst = Psn::instance();
    try
    {
        psn_inst.clearDatabase();
        psn::readLiberty(psn_inst.sta(), "../test/data/libraries/Nangate45/"
                                         "NangateOpenCellLibrary_typical.lib");
        psn::readLef(psn_inst.database(), psn_inst.sta(),
                     "../test/data/libraries/Nangate45/"
                     "NangateOpenCellLibrary.mod.lef",
                     "nangate45", true, true);
        psn::readDef(psn_inst.database(), psn_inst.sta(),
                     "../test/data/designs/gcd/gcd.def");
        CHECK(psn_inst.database()->getChip() != nullptr);
        auto& handler = *(psn_inst.handler());
        handler.createClock("core_clock", {"clk"}, 10);
        auto                     critical_path = handler.criticalPath();
        std::vector<std::string> actual_critical_path_pin_names(
            {"_860_/CK", "_860_/Q", "_718_/A", "_718_/Z", "_438_/A", "_438_/ZN",
             "_439_/A1", "_439_/ZN", "_462_/A1", "_462_/ZN", "_463_/A1",
             "_463_/ZN", "_465_/A1", "_465_/ZN", "_762_/A", "_762_/Z",
             "_858_/D"});
        for (size_t i = 0; i < critical_path.size(); i++)
        {
            CHECK(handler.name(critical_path[i].pin()) ==
                  actual_critical_path_pin_names[i]);
        }
    }
    catch (PsnException& e)
    {
        FAIL(e.what());
    }
}
