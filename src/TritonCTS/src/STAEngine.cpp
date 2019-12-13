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

#include "STAEngine.h"

#include "Machine.hh"
#include "Sta.hh"
//#include "StaMain.hh"
#include "VerilogReader.hh"
#include "Network.hh"
#include "Graph.hh"
#include "Sdc.hh"
#include "openroad/OpenRoad.hh"
#include "db_sta/dbSta.hh"

#include <tcl.h>
#include <iostream>
#include <sstream>
#include <cassert>

extern "C" {
        extern int Sta_Init(Tcl_Interp *interp);
}

namespace sta { 
        extern const char *tcl_inits[];
}

namespace TritonCTS {

void STAEngine::init() {
        std::cout << "\n";
        std::cout << " ***************************\n";
        std::cout << " * Initializing STA engine *\n";
        std::cout << " ***************************\n";

        ord::OpenRoad* openRoad = ord::OpenRoad::openRoad();
        _openSta = openRoad->getSta();
        _graph = _openSta->graph();
        //initTclShell();
        //initOpenSTA();
        //readLiberty();
        //readVerilog();
        //linkDesign();
        //readSdcFile();
        //updateTiming();
        findClockRoots();
}

void STAEngine::initTclShell() {
        //std::cout << " Initializing OpenSTA tcl shell...\n";
        //_interp = Tcl_CreateInterp();
        //Tcl_Init(_interp);

        //Sta_Init(_interp);
        //sta::initSta();

        //sta::evalTclInit(_interp, sta::tcl_inits);
        //Tcl_Eval(_interp, "namespace import sta::*");
        //Tcl_Eval(_interp, "define_sta_cmds");
}

void STAEngine::initOpenSTA() {
        //std::cout << " Initializing OpenSTA data structures...\n";
        //_openSta = new sta::Sta();
        //sta::Sta::setSta(_openSta);
        //_openSta->makeComponents();
        //_openSta->setTclInterp(_interp);

}

void STAEngine::readLiberty() {
        //std::cout << " Reading liberty files...\n";
        //
        //std::string cornerName="wst";
        //sta::StringSet cornerNameSet;
        //cornerNameSet.insert(cornerName.c_str());

        //_openSta->makeCorners(&cornerNameSet);
        //sta::Corner *corner = _openSta->findCorner(cornerName.c_str());

        //std::stringstream libertyFiles(_parms->getLibertyFiles());
        //std::string file;
        //unsigned numLibertyFiles = 0;
        //while (libertyFiles >> file) {
        //        std::cout << "    Reading \"" << file << "\"\n";
        //        _openSta->readLiberty(file.c_str(), corner, sta::MinMaxAll::max(), false);
        //        ++numLibertyFiles;
        //}

        //std::cout << "    " << numLibertyFiles << " liberty file(s) have been read.\n";
        //if (numLibertyFiles < 1) {
        //        std::cout << "    Please specify liberty files with \"set_sta_liberty_files\" command.\n";
        //        std::exit(1);
        //}
}

void STAEngine::readVerilog() {
        //std::cout << " Reading verilog file...\n";

        ////read_netlist
        //_network = _openSta->networkReader();
        //assert(_network);        
        //_openSta->readNetlistBefore();       
  
        //std::cout << "    File path: \"" << _parms->getVerilogFile() << "\"\n";
        //bool success = readVerilogFile(_parms->getVerilogFile().c_str(), _network);
        //if (!success) {
        //        std::cout << "    [ERROR] Failed to read the verilog file!\n";
        //        std::exit(1);
        //}
}

void STAEngine::linkDesign() {
        //std::cout << " Linking design...\n";
        //std::cout << "    Block name: \"" << _parms->getBlockName() << "\"\n";
        //Tcl_Eval(_interp, std::string("set link_make_block_boxes 0").c_str());
        //Tcl_Eval(_interp, std::string("link_design " + _parms->getBlockName()).c_str());

        //bool isLinked = _network->isLinked();
        //assert(isLinked);
}

void STAEngine::readSdcFile() {
        //std::cout << " Reading SDC file...\n";
        //std::cout << "    File path: \"" << _parms->getSdcFile() << "\"\n";
        //Tcl_Eval(_interp, std::string( "sta::read_sdc " + _parms->getSdcFile()).c_str() );
}

void STAEngine::updateTiming() {
        std::cout << " Update timing...\n";
        _openSta->updateTiming(true);
        _graph = _openSta->graph();
}

void STAEngine::findClockRoots() {
        std::cout << " Looking for clock sources...\n";

        unsigned numEdges = _graph->edgeCount();
        unsigned numVertices = _graph->vertexCount();
        std::cout << "    # edges: " << numEdges << "\n";        
        std::cout << "    # vertices: " << numVertices << "\n";

        std::string clockNames = "";
        for (unsigned vertexIdx = 1; vertexIdx <= numVertices; ++vertexIdx) {
                sta::Vertex* vertex = _openSta->graph()->vertex(vertexIdx);
                sta::Pin* pin = vertex->pin();
                
                if( !_openSta->network()->isTopLevelPort(pin) ||
                    !(_openSta->network()->isCheckClk(pin) ||  _openSta->sdc()->isClock(pin)) ) {
                        continue;
                }

                clockNames += std::string(_openSta->network()->name(pin)) + " ";

        }

        std::cout << "    Clock names: " << clockNames << "\n";
        _parms->setClockNets(clockNames);
}

}
