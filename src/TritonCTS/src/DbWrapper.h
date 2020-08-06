/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
        void writeClockNetsToDb(Clock& clockNet);

        void incrementNumClocks() { _numberOfClocks = _numberOfClocks + 1; }
        void clearNumClocks() { _numberOfClocks = 0; }
        unsigned getNumClocks() const { return _numberOfClocks; }

private:
        sta::dbSta*       _openSta              = nullptr;
        odb::dbDatabase*  _db                   = nullptr;
        odb::dbChip*      _chip                 = nullptr;
        odb::dbBlock*     _block                = nullptr;
        CtsOptions*       _options              = nullptr;
        TritonCTSKernel*  _kernel               = nullptr;  
        unsigned          _numberOfClocks       = 0;       
        unsigned          _numClkNets           = 0;   
        unsigned          _numFixedNets         = 0;

        void parseClockNames(std::vector<std::string>& clockNetNames) const;

        void initDB();
        void initAllClocks();
        void initClock(odb::dbNet* net);
        
        void disconnectAllSinksFromNet(odb::dbNet* net);
        void disconnectAllPinsFromNet(odb::dbNet* net);
        void checkUpstreamConnections(odb::dbNet* net);
        void createClockBuffers(Clock& clk);
        void removeNonClockNets();
        void computeITermPosition(odb::dbITerm* term, DBU &x, DBU &y) const;
        std::pair<int,int> branchBufferCount(ClockInst *inst, int bufCounter, Clock& clockNet);
        odb::dbITerm* getFirstInput(odb::dbInst* inst) const;
};  

}

#endif
