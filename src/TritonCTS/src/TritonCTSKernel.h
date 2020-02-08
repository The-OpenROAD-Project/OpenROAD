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

#include "DbWrapper.h"
#include "CtsOptions.h"
#include "TechChar.h"
#include "TreeBuilder.h"
#include "StaEngine.h"

#include <functional>

namespace TritonCTS {

class TritonCTSKernel {
public:
        TritonCTSKernel() : _dbWrapper(_options, *this),
                            _techChar(_options),
                            _staEngine(_options) {}

        void runTritonCts();
        CtsOptions& getParms() { return _options; }
        void addBuilder(TreeBuilder* builder) { _builders.push_back(builder); }
        void forEachBuilder(const std::function<void(const TreeBuilder*)> func) const;

private:
        void printHeader() const;
        void importCharacterization();
        void checkCharacterization();
        void findClockRoots();
        void populateTritonCts();
        void buildClockTrees();
        void runPostCtsOpt();
        void writeDataToDb();
        void printFooter() const;
        
        CtsOptions _options;
        DbWrapper  _dbWrapper;
        TechChar   _techChar;
        StaEngine  _staEngine;
        std::vector<TreeBuilder*> _builders;

//-----------------------------------------------------------------------------

// TCL commands
public:
        void set_lut_file(const char* file);
        void set_sol_list_file(const char* file);
        void export_characterization(const char* file);
        void set_root_buffer(const char* buffer);
        void set_clock_nets(const char* names);
        void set_wire_segment_distance_unit(unsigned unit);
        void run_triton_cts();
        void report_characterization();
        void report_wire_segments(unsigned length, unsigned load, unsigned outputSlew); 
};

}

#endif
