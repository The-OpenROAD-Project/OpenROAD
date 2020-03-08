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
// #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_IMPLEMENT

#include <tcl.h>
#include "OpenSTA/app/StaMain.hh"
#include "Psn.hpp"
#include "Readers.hpp"
#include "db.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "doctest.h"
#include "flute3/flute.h"

namespace sta
{
extern const char* openroad_tcl_inits[];
}
using sta::evalTclInit;

int
main(int argc, char** argv)
{
    doctest::Context context;

    odb::dbDatabase* db        = odb::dbDatabase::create();
    sta::dbSta*      sta_state = new sta::dbSta;

    Tcl_Interp* interp = Tcl_CreateInterp();
    Tcl_Init(interp);
    evalTclInit(interp, sta::openroad_tcl_inits);

    sta_state->init(interp, db);
    psn::Psn::initialize(sta_state);
    Flute::readLUT();
    if (psn::Psn::instance().setupInterpreter(interp, true, true, true) !=
        TCL_OK)
    {
        printf("Failed to initialize Tcl interpreter");
        return -1;
    }

    context.applyCommandLine(argc, argv);

    // overrides
    context.setOption(
        "no-breaks",
        false); // don't break in the debugger when assertions fail

    int res = context.run(); // run

    if (context.shouldExit()) // important - query flags (and --exit) rely on
                              // the user doing this
        return res;           // propagate the result of the tests

    int client_stuff_return_code = 0;

    return res + client_stuff_return_code; // the result from doctest is
                                           // propagated here as well
}