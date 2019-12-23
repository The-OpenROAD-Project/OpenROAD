////////////////////////////////////////////////////////////////////////////////////
// Authors: Mateus Fogaca
//          (Ph.D. advisor: Ricardo Reis)
//          Jiajia Li
//          Andrew Kahng
// Based on:
//          K. Han, A. B. Kahng and J. Li, "Optimal Generalized H-Tree Topology and 
//          Buffering for High-Performance and Low-Power Clock Distribution", 
//          IEEE Trans. on CAD (2018), doi:10.1109/TCAD.2018.2889756.
//
//
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
// All rights reserved.
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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////

#ifndef TRITONCTSKERNEL_H
#define TRITONCTSKERNEL_H

#include "DBWrapper.h"
#include "ParametersForCTS.h"
#include "Characterization.h"
#include "ClockTreeBuilder.h"
#include "STAEngine.h"

#include <functional>

namespace TritonCTS {

class TritonCTSKernel {
public:
        TritonCTSKernel() : _dbWrapper(_parms, *this),
                            _staEngine(_parms) {}

        void addBuilder(ClockTreeBuilder* builder) { _builders.push_back(builder); }
        void buildClockTrees();
        void forEachBuilder(const std::function<void(const ClockTreeBuilder*)> func) const;
        ParametersForCTS& getParms() { return _parms; }

private:
        ParametersForCTS _parms;
        DBWrapper        _dbWrapper;
        Characterization _characterization;
        STAEngine        _staEngine;
        std::vector<ClockTreeBuilder*> _builders;

//-----------------------------------------------------------------------------

// TCL commands
public:
        void import_characterization(const char* file);
        void import_sol_list(const char* file);
        void export_characterization(const char* file);
        void set_root_buffer(const char* buffer);
        void set_clock_nets(const char* names);
        void set_wire_segment_distance_unit(unsigned unit);
        void run_triton_cts();
        void report_characterization();
        void report_wire_segments(unsigned length, unsigned load, unsigned outputSlew); 

        void set_sta_verilog_file(const char* file);
        void set_sta_sdc_file(const char* file);
        void set_sta_liberty_files(const char* file);
        void init_sta_engine();
};

}

#endif
