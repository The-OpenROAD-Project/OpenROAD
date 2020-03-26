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
#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <math.h>

namespace PartClusManager {

class Graph {
public:
        Graph() {}
	float getEdgeWeight (int idx) const { return _edgeWeights[idx];}
	long long int getVertexWeight(int idx) const { return _vertexWeights[idx];}
	int getColIdx(int idx) const {return _colIdx[idx];}
	int getRowPtr(int idx) const {return _rowPtr[idx];}
	int getMapping(std::string inst){return _instToIdx[inst];}
	std::vector<float> getEdgeWeight() const { return _edgeWeights;};
	std::vector<long long int> getVertexWeight() const { return _vertexWeights;};
	std::vector<int> getColIdx() const { return _colIdx;};
	std::vector<int> getRowPtr() const { return _rowPtr;};

	void addEdgeWeight(float weight){_edgeWeights.push_back(weight);} 
	void addVertexWeight(long long int weight){_vertexWeights.push_back(weight);} 
	void addColIdx(int idx){_colIdx.push_back(idx);} 
	void addRowPtr(int idx){_rowPtr.push_back(idx);} 
	void addMapping(std::string inst, int idx){_instToIdx[inst] = idx;}
	std::map<std::string, int> & getMap(){return _instToIdx;}

	inline bool isInMap (std::string pinName) const{
		if (_instToIdx.find(pinName) != _instToIdx.end())
        		return true;
    		else
        		return false;
	}

	void computeWeightRange(int maxEdgeWeight, int maxVertexWeight);
	int computeNextVertexIdx() const {return _vertexWeights.size();}
	int computeNextRowPtr() const {return _edgeWeights.size();}
	int getNumEdges() const {return _edgeWeights.size();}
	int getNumVertex() const {return _vertexWeights.size();}
private:
	std::vector<float> _edgeWeights;
	std::vector<long long int> _vertexWeights;
	std::vector<int> _colIdx;
	std::vector<int> _rowPtr;
	std::map<std::string, int> _instToIdx;

};

}
