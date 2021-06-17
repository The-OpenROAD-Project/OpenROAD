/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdio.h>
#include <unistd.h>
#include <set>

namespace ord {
class OpenRoad;
} // namespace ord


namespace odb {
class dbInst;
class dbBlock;
}

namespace utl {
class Logger;
}

namespace sta {
class dbSta;
}  // namespace sta

using utl::Logger;


namespace rmp {

class blif
{
public:
    blif(ord::OpenRoad* openroad, std::string const0Cell, std::string const0CellPort,
                                    std::string const1Cell, std::string const1CellPort);
    void setReplaceableInstances(std::set<odb::dbInst*>& insts){
        instancesToOptimize = insts;
    }
    bool writeBlif(const char* fileName);
    bool readBlif(const char* fileName, odb::dbBlock* block);
    bool inspectBlif(const char* fileName, int& numInstances);
private:
    std::set<odb::dbInst*> instancesToOptimize;
    ord::OpenRoad* openroad_;
    Logger* logger_;
    sta::dbSta* openSta_ = nullptr;
    std::string const0Cell_;
    std::string const0CellPort_;
    std::string const1Cell_;
    std::string const1CellPort_;
};

} // namespace rmp