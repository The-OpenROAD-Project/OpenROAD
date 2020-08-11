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


#include "HungarianMatching.h"

namespace ioPlacer { 

HungarianMatching::HungarianMatching(Section_t& section, slotVector_t& slots)
    : _netlist(section.net), _slots(slots) {
        _numIOPins = _netlist.numIOPins();
        _beginSlot = section.beginSlot;
        _endSlot = section.endSlot;
        _numSlots = _endSlot - _beginSlot;
        _nonBlockedSlots = section.numSlots;
}

void HungarianMatching::run() {
        createMatrix();
        _hungarianSolver.Solve(_hungarianMatrix, _assignment);
}

void HungarianMatching::createMatrix() {
	_hungarianMatrix.resize(_nonBlockedSlots);
        unsigned slotIndex = 0;
        for (unsigned i = _beginSlot; i < _endSlot; ++i) {
                unsigned pinIndex = 0;
                Coordinate newPos = _slots[i].pos;
                if (_slots[i].blocked) {
                        continue;
                }
		_hungarianMatrix[slotIndex].resize(_numIOPins);
                _netlist.forEachIOPin([&](unsigned idx, IOPin& ioPin) {
                        DBU hpwl = _netlist.computeIONetHPWL(idx, newPos);
                        _hungarianMatrix[slotIndex][pinIndex] = hpwl;
                        pinIndex++;
                });
                slotIndex++;
        }
}

inline bool samePos(Coordinate& a, Coordinate& b) {
        return (a.getX() == b.getX() && a.getY() == b.getY());
}

void HungarianMatching::getFinalAssignment(std::vector<IOPin>& assigment) {
        size_t rows = _nonBlockedSlots;
        size_t col = 0;
        unsigned slotIndex = 0;
        _netlist.forEachIOPin([&](unsigned idx, IOPin& ioPin) {
                slotIndex = _beginSlot;
                for (size_t row = 0; row < rows; row++) {
                        while (_slots[slotIndex].blocked &&
                               slotIndex < _slots.size())
                                slotIndex++;
                        if (_assignment[row] != col) {
                                slotIndex++;
                                continue;
                        }
                        ioPin.setPos(_slots[slotIndex].pos);
                        assigment.push_back(ioPin);
                        Coordinate sPos = _slots[slotIndex].pos;
                        for (unsigned i = 0; i < _slots.size(); i++) {
                                if (samePos(_slots[i].pos, sPos)) {
                                        _slots[i].used = true;
                                        break;
                                }
                        }
                        break;
                }
                col++;
        });
}

}
