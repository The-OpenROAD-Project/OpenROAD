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

#include "TritonCTSKernel.h"

// DB includes
#include "db.h"

#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

namespace TritonCTS {

void TritonCTSKernel::set_only_characterization(bool enable) {
        _options.setOnlyCharacterization(enable);
}

void TritonCTSKernel::set_lut_file(const char* file) {
        _options.setLutFile(file);
}

void TritonCTSKernel::set_sol_list_file(const char* file) {
        _options.setSolListFile(file);
}

int TritonCTSKernel::set_clock_nets(const char* names) {
        odb::dbDatabase* db    = odb::dbDatabase::getDatabase(_options.getDbId());
        odb::dbChip* chip  = db->getChip();
        odb::dbBlock* block = chip->getBlock();

        _options.setClockNets(names);
        std::stringstream ss(names);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> nets(begin, end);

        std::vector<odb::dbNet*> netObjects;

        for (std::string name : nets){
                odb::dbNet* net = block->findNet(name.c_str());
                bool netFound = false;
                if (net != nullptr) {
                        //Since a set is unique, only the nets not found by dbSta are added.
                        netObjects.push_back(net);
                        netFound = true;
                } else {
                        //User input was a pin, transform it into an iterm if possible
                        odb::dbITerm* iterm = block->findITerm(name.c_str());
                        if (iterm != nullptr) {
                                net = iterm->getNet();
                                 if (net != nullptr) {
                                        //Since a set is unique, only the nets not found by dbSta are added.
                                        netObjects.push_back(net);
                                        netFound = true;
                                }
                        }
                        
                }
                if (!netFound) {
                        return 1;
                }
        }
        _options.setClockNetsObjs(netObjects);
        return 0;
}

void TritonCTSKernel::set_max_char_cap(double cap) {
        _options.setMaxCharCap(cap);
}

void TritonCTSKernel::set_max_char_slew(double slew) {
        _options.setMaxCharSlew(slew);
}

void TritonCTSKernel::set_wire_segment_distance_unit(unsigned unit) {
        _options.setWireSegmentUnit(unit);
}

void TritonCTSKernel::run_triton_cts() {
        runTritonCts();
}

void TritonCTSKernel::report_characterization() {
        _techChar.report();
}

void TritonCTSKernel::report_wire_segments(unsigned length, unsigned load, unsigned outputSlew) {
        _techChar.reportSegments(length, load, outputSlew);
}

void TritonCTSKernel::export_characterization(const char* file) {
        _techChar.write(file);
}

void TritonCTSKernel::set_root_buffer(const char* buffer) {
        _options.setRootBuffer(buffer);
}

void TritonCTSKernel::set_buffer_list(const char* buffers) {
        std::stringstream ss(buffers);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> bufferVector(begin, end);
        _options.setBufferList(bufferVector);
}

void TritonCTSKernel::set_out_path(const char* path) {
        _options.setOutputPath(path);
}

void TritonCTSKernel::set_cap_per_sqr(double cap) {
        _options.setCapPerSqr(cap);
}

void TritonCTSKernel::set_res_per_sqr(double res) {
        _options.setResPerSqr(res);
}

void TritonCTSKernel::set_slew_inter(double slew){
         _options.setSlewInter(slew);
}

void TritonCTSKernel::set_cap_inter(double cap){
        _options.setCapInter(cap);
}

void TritonCTSKernel::set_metric_output(const char* file){
        _options.setMetricsFile(file);
}

void TritonCTSKernel::report_cts_metrics(){
        reportCtsMetrics();
};

}
