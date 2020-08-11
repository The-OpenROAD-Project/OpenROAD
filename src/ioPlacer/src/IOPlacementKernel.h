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


#ifndef __IOPLACEMENTKERNEL_H_
#define __IOPLACEMENTKERNEL_H_

#include "Core.h"
#include "HungarianMatching.h"
#include "Netlist.h"
#include "Parameters.h"
#include "Slots.h"
#include "DBWrapper.h"

namespace ioPlacer {

enum RandomMode { None, Full, Even, Group };

class IOPlacementKernel {
       protected:
        Netlist _netlist;
        Core _core;
        std::string _horizontalMetalLayer;
        std::string _verticalMetalLayer;
        std::vector<IOPin> _assignment;
        bool _reportHPWL = false;

        unsigned _slotsPerSection = 200;
        float _slotsIncreaseFactor = 0.01f;

        float _usagePerSection = .8f;
        float _usageIncreaseFactor = 0.01f;

        bool _forcePinSpread = true;
        std::string _blockagesFile;
        std::vector<std::pair<Coordinate, Coordinate>> _blockagesArea;

       private:
        DBWrapper _dbWrapper;
        Parameters* _parms;
        Netlist _netlistIOPins;
        slotVector_t _slots;
        sectionVector_t _sections;
        std::vector<IOPin> _zeroSinkIOs;
        RandomMode _randomMode = RandomMode::Full;
        bool _cellsPlaced = true;

        void initNetlistAndCore();
        void initIOLists();
        void initParms();
        void randomPlacement(const RandomMode);
        void defineSlots();
        void createSections();
        void setupSections();
        bool assignPinsSections();
        DBU returnIONetsHPWL(Netlist&);

        inline void updateOrientation(IOPin&);
        inline void updatePinArea(IOPin&);
        inline bool checkBlocked(DBU, DBU);

       public:
        IOPlacementKernel(Parameters&);
        IOPlacementKernel() = default;
        void run();
        void writeDEF();
        void printConfig();
        void parseLef(const std::string& file) { _dbWrapper.parseLEF(file); }
        void parseDef(const std::string& file) { _dbWrapper.parseDEF(file); }
        DBU returnIONetsHPWL();
        void addBlockedArea(long long int llx, long long int lly, long long int urx, long long int ury);
};

}
#endif /* __IOPLACEMENTKERNEL_H_ */
