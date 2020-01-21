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

#ifndef CLOCKTREEBUILDER_H
#define CLOCKTREEBUILDER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <deque>

#include "Clock.h"
#include "Util.h"
#include "CtsOptions.h"
#include "TechChar.h"

namespace TritonCTS {

class TreeBuilder {
public:
        TreeBuilder(CtsOptions& options, Clock& clk) : 
                    _options(&options), _clock(clk) {};
        
        virtual void run() = 0;
        void setTechChar(TechChar& techChar) { _techChar = &techChar; }
        const Clock& getClock() const { return _clock; }        
        Clock& getClock() { return _clock; }        

protected:
        CtsOptions* _options = nullptr;
        Clock       _clock;
        TechChar*   _techChar = nullptr;
};

}
#endif
