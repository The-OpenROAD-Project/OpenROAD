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

#include "Graph.h"

namespace PartClusManager {

void Graph::computeWeightRange(int maxEdgeWeight, int maxVertexWeight){
	std::vector<int> edgeWeight = _edgeWeights;
        std::vector<double> vertexWeight = _vertexWeights;
        double percentile = 0.99; //Exclude possible outliers

        std::sort(edgeWeight.begin(), edgeWeight.end());
        std::sort(vertexWeight.begin(), vertexWeight.end());

        int eSize = edgeWeight.size();
        int vSize = vertexWeight.size();
	
	eSize = (int)(eSize * percentile);
        vSize = (int)(vSize * percentile);
        edgeWeight.resize(eSize);
        edgeWeight.shrink_to_fit();
        vertexWeight.resize(vSize);
        vertexWeight.shrink_to_fit();

        int maxEWeight = *std::max_element(edgeWeight.begin(), edgeWeight.end());
        double maxVWeight = *std::max_element(vertexWeight.begin(), vertexWeight.end());
        int minEWeight = *std::min_element(edgeWeight.begin(), edgeWeight.end());
        double minVWeight = *std::min_element(vertexWeight.begin(), vertexWeight.end());

        for (int & weight : _edgeWeights){
                weight = std::min(weight, maxEWeight);
                if (minEWeight == maxEWeight){
                        weight = maxEdgeWeight;
                }
                else{
                        weight = (int)((((weight - minEWeight) * (maxEdgeWeight -1))/(maxEWeight - minEWeight)) + 1);
                }
        }
        for (double & weight : _vertexWeights){
                weight = std::min(weight, maxVWeight);
                if (minVWeight == maxVWeight){
                        weight = maxVertexWeight;
                }
                else{
                        weight = (double)((((weight - minVWeight) * (maxVertexWeight -1))/(maxVWeight - minVWeight)) + 1);
                }
        }	

}

}
