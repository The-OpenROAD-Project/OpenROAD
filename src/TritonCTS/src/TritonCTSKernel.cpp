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
#include "PostCtsOpt.h"

#include <unordered_set>
#include <chrono>
#include <ctime>

namespace TritonCTS {

void TritonCTSKernel::runTritonCts() {
        printHeader();
        importCharacterization();
        findClockRoots();
        populateTritonCts();
        checkCharacterization();
        buildClockTrees();
        runPostCtsOpt();
        writeDataToDb();
        printFooter();
}

void TritonCTSKernel::printHeader() const {
        std::cout << " *****************\n";
        std::cout << " * TritonCTS 2.0 *\n";        
        std::cout << " *****************\n";
}

void TritonCTSKernel::importCharacterization() {
        std::cout << " *****************************\n";
        std::cout << " *  Import characterization  *\n";
        std::cout << " *****************************\n";
       
        _techChar.parse(_options.getLutFile(), _options.getSolListFile()); 
}

void TritonCTSKernel::checkCharacterization() {
        std::cout << " ****************************\n";
        std::cout << " *  Check characterization  *\n";
        std::cout << " ****************************\n";

        std::unordered_set<std::string> visitedMasters;
        _techChar.forEachWireSegment( [&] (unsigned idx, const WireSegment& wireSeg) {
                for (unsigned buf = 0; buf < wireSeg.getNumBuffers(); ++buf) {
                        std::string master = wireSeg.getBufferMaster(buf);
                        if (visitedMasters.count(master) != 0) {
                                continue;
                        }
                        
                        if (_dbWrapper.masterExists(master)) {
                                visitedMasters.insert(master);
                        } else {
                                std::cout << "    [ERROR] Buffer " << master 
                                          << " is not in the loaded DB.\n";
                                std::exit(1);
                        }           
                }
        });

        std::cout << "    The chacterization used " << visitedMasters.size() << " buffer(s) types."
                  << " All of them are in the loaded DB.\n";
}

void TritonCTSKernel::findClockRoots() {
        std::cout << " **********************\n";
        std::cout << " *  Find clock roots  *\n";
        std::cout << " **********************\n";
        
        if(_options.getClockNets() != "") {
                std::cout << " Running TritonCTS with user-specified clock roots: ";
                std::cout << _options.getClockNets() << "\n";
                return;
        }

        std::cout << " User did not specify clock roots.\n";
        std::cout << " Using OpenSTA to find clock roots.\n";
        _staEngine.init();
        _staEngine.findClockRoots();
}

void TritonCTSKernel::populateTritonCts() {
        std::cout << " ************************\n";
        std::cout << " *  Populate TritonCTS  *\n";
        std::cout << " ************************\n";
        
        _dbWrapper.populateTritonCTS();

        if (_builders.size() < 1) {
                std::cout << "    [ERROR] No valid clock nets in the design. Exiting...\n";
                std::exit(0);
        }
}

void TritonCTSKernel::buildClockTrees() {
        std::cout << " ***********************\n";
        std::cout << " *  Build clock trees  *\n";
        std::cout << " ***********************\n";
        
        for (TreeBuilder* builder: _builders) {
                builder->setTechChar(_techChar);
                builder->run();
        }
}

void TritonCTSKernel::runPostCtsOpt() {
        if (!_options.runPostCtsOpt()) {
                return;
        }

        std::cout << " ****************\n";
        std::cout << " * Post CTS opt *\n";
        std::cout << " ****************\n";

        for (TreeBuilder* builder: _builders) {
                PostCtsOpt opt(builder->getClock(), _options);
                opt.run();
        }
}

void TritonCTSKernel::writeDataToDb() {
        std::cout << " ********************\n";
        std::cout << " * Write data to DB *\n";
        std::cout << " ********************\n";
        
        for (TreeBuilder* builder: _builders) {
                _dbWrapper.writeClockNetsToDb(builder->getClock());
        }
} 

void TritonCTSKernel::forEachBuilder(const std::function<void(const TreeBuilder*)> func) const {
        for (const TreeBuilder* builder: _builders) {
                func(builder);
        }
}

void TritonCTSKernel::printFooter() const {
        std::cout << " ... End of TritonCTS execution.\n";
}

}
