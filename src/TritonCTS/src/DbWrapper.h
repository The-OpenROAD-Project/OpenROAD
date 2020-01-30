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

#ifndef DBWRAPPER_H
#define DBWRAPPER_H

#include "CtsOptions.h"
#include "TreeBuilder.h"

#include <string>
#include <vector>

namespace odb {
class dbDatabase;
class dbChip;
class dbBlock;
class dbInst;
class dbNet;
class dbITerm;
}

namespace TritonCTS {

class TritonCTSKernel;

class DbWrapper {
public:
        DbWrapper(CtsOptions& options,
                  TritonCTSKernel& kernel);

        bool masterExists(const std::string& master) const;

        void populateTritonCTS();
        void writeClockNetsToDb(const Clock& clockNet);

private:
        odb::dbDatabase*  _db      = nullptr;
        odb::dbChip*      _chip    = nullptr;
        odb::dbBlock*     _block   = nullptr;
        CtsOptions*       _options = nullptr;
        TritonCTSKernel*  _kernel  = nullptr;         

        void parseClockNames(std::vector<std::string>& clockNetNames) const;

        void initDB();
        void initAllClocks();
        void initClock(odb::dbNet* net);
        
        void disconnectAllSinksFromNet(std::string netName);
        void createClockBuffers(const Clock& clk);
        void removeNonClockNets();
        void computeITermPosition(odb::dbITerm* term, DBU &x, DBU &y) const;
        odb::dbITerm* getFirstInput(odb::dbInst* inst) const;
};  

}

#endif
