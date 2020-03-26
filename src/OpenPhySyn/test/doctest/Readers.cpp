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

// Reader utilities for unit tests

#include "Readers.hpp"

namespace psn
{

using odb::dbDatabase;
using odb::dbLib;
using sta::dbSta;

void
readLef(dbDatabase* db, dbSta* sta_state, const char* filename,
        const char* lib_name, bool make_tech, bool make_library)
{
    odb::lefin lef_reader(db, false);
    if (make_tech && make_library)
    {
        dbLib* lib = lef_reader.createTechAndLib(lib_name, filename);
        if (lib)
            sta_state->readLefAfter(lib);
    }
    else if (make_tech)
        lef_reader.createTech(filename);
    else if (make_library)
    {
        dbLib* lib = lef_reader.createLib(lib_name, filename);
        if (lib)
            sta_state->readLefAfter(lib);
    }
}

void
readDef(dbDatabase* db, dbSta* sta_state, const char* filename)
{
    odb::defin               def_reader(db);
    std::vector<odb::dbLib*> search_libs;
    for (odb::dbLib* lib : db->getLibs())
        search_libs.push_back(lib);
    def_reader.createChip(search_libs, filename);
    sta_state->readDefAfter();
}
void
readLiberty(dbSta* sta_state, const char* filename)
{
    sta::LibertyLibrary* lib = sta_state->readLiberty(
        filename, sta_state->cmdCorner(), sta::MinMaxAll::all(), false);
    sta_state->network()->readLibertyAfter(lib);
}
} // namespace psn