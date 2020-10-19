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

#include "Hypergraph.h"

#include <algorithm>

namespace PartClusManager {

void Hypergraph::computeEdgeWeightRange(int maxEdgeWeight)
{
  std::vector<float> edgeWeight = _edgeWeights;
  double percentile = 0.99;  // Exclude possible outliers

  std::sort(edgeWeight.begin(), edgeWeight.end());

  int eSize = edgeWeight.size();

  eSize = (int) (eSize * percentile);
  edgeWeight.resize(eSize);
  edgeWeight.shrink_to_fit();

  float maxEWeight = *std::max_element(edgeWeight.begin(), edgeWeight.end());
  float minEWeight = *std::min_element(edgeWeight.begin(), edgeWeight.end());

  for (float& weight : _edgeWeights) {
    int auxWeight;
    weight = std::min(weight, maxEWeight);
    if (minEWeight == maxEWeight) {
      auxWeight = maxEdgeWeight;
    } else {
      auxWeight = (int) ((((weight - minEWeight) * (maxEdgeWeight - 1))
                          / (maxEWeight - minEWeight))
                         + 1);
    }
    _edgeWeightsNormalized.push_back(auxWeight);
  }
}

void Hypergraph::computeVertexWeightRange(int maxVertexWeight)
{
  std::vector<long long int> vertexWeight = _vertexWeights;
  double percentile = 0.99;  // Exclude possible outliers

  std::sort(vertexWeight.begin(), vertexWeight.end());

  int vSize = vertexWeight.size();

  vSize = (int) (vSize * percentile);
  vertexWeight.resize(vSize);
  vertexWeight.shrink_to_fit();

  long long int maxVWeight
      = *std::max_element(vertexWeight.begin(), vertexWeight.end());
  long long int minVWeight
      = *std::min_element(vertexWeight.begin(), vertexWeight.end());

  for (long long int& weight : _vertexWeights) {
    int auxWeight;
    weight = std::min(weight, maxVWeight);
    if (minVWeight == maxVWeight) {
      auxWeight = maxVertexWeight;
    } else {
      auxWeight = (int) ((((weight - minVWeight) * (maxVertexWeight - 1))
                          / (maxVWeight - minVWeight))
                         + 1);
    }
    _vertexWeightsNormalized.push_back(auxWeight);
  }
  _vertexWeights.clear();
  _vertexWeights.shrink_to_fit();
}

}  // namespace PartClusManager
