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


#include "Parameters.h"

#include <iostream>
#include <iomanip>

namespace ioPlacer {

void Parameters::printAll() const {
        // clang-format off
        std::cout << "\nOptions: \n";
        std::cout << std::setw(20) << std::left << "Input LEF file: ";
        std::cout << _inputLefFile << "\n";
        std::cout << std::setw(20) << std::left << "Input DEF file: ";
        std::cout << _inputDefFile << "\n";
        std::cout << std::setw(20) << std::left << "Output DEF file: ";
        std::cout << _outputDefFile << "\n";
        std::cout << std::setw(20) << std::left << "Horizontal metal layer: ";
        std::cout << "Metal" << _horizontalMetalLayer << "\n";
        std::cout << std::setw(20) << std::left << "Vertical metal layer: ";
        std::cout << "Metal" << _verticalMetalLayer << "\n";
        std::cout << "Report IO nets HPWL: " << _reportHPWL << "\n";

        std::cout << "Number of slots per section: " << _numSlots << "\n";
        std::cout << "Increase factor of slots per section: " << _slotsFactor << "\n";
        std::cout << "Percentage of usage for each section: " << _usage << "\n";
        std::cout << "Increase factor of usage for each section: " << _usageFactor << "\n";
        std::cout << "Force pin spread: " << _forceSpread << "\n";
        std::cout << "Blockage area file: " << _blockagesFile << "\n";
        std::cout << "Random mode: " << _randomMode << "\n";
        std::cout << "Horizontal pin length: " << _horizontalLength << "\n";
        std::cout << "Vertical pin length: " << _verticalLength << "\n";
        std::cout << "Horizontal pin length extend: " << _horizontalLengthExtend << "\n";
        std::cout << "Vertical pin length extend: " << _verticalLengthExtend << "\n";
        std::cout << "Horizontal pin thickness multiplier: " << _horizontalThicknessMultiplier << "\n";
        std::cout << "Vertical pin thickness multiplier: " << _verticalThicknessMultiplier << "\n";
        std::cout << "Interactive mode: " << _interactiveMode << "\n";
        std::cout << "Num threads: " << _numThreads << "\n";
        std::cout << "Rand seed: " << _randSeed << "\n";

        std::cout << "\n";
        // clang-format on
}

}
