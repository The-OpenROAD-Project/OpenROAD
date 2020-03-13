////////////////////////////////////////////////////////////////////////////////
// Authors: Mateus Fogaça, Isadora Oliveira and Marcelo Danigno
// 
//          (Advisor: Ricardo Reis and Paulo Butzen)
//
// BSD 3-Clause License
//
// Copyright (c) 2020, Federal University of Rio Grande do Sul (UFRGS)
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
////////////////////////////////////////////////////////////////////////////////

#include "PartClusManagerKernel.h"
namespace PartClusManager {

void PartClusManagerKernel::runPartitioning() {
        if (_options.getTool() == "mlpart") {
                runMlPart();
        } else if (_options.getTool() == "gpmetis") {
                runGpMetis();
        } else {
                runChaco();
        }
}

void PartClusManagerKernel::runChaco() {
        std::cout << "Running chaco...\n";
}

void PartClusManagerKernel::runChaco(const Graph& graph, const PartOptions& options) {
}

void PartClusManagerKernel::runGpMetis() {
        std::cout << "Running GPMetis...\n";
}

void PartClusManagerKernel::runGpMetis(const Graph& graph, const PartOptions& options) {
}

void PartClusManagerKernel::runMlPart() {        
        std::cout << "Running MLPart...\n";
}

void PartClusManagerKernel::runMlPart(const Graph& graph, const PartOptions& options) {        
}

void PartClusManagerKernel::graph(){
	GraphDecomposition graphDecomp;
	graphDecomp.init(_dbId);
	graphDecomp.createGraph(GraphType::HYBRID, _graph);
}

}
