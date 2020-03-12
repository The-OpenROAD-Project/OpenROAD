////////////////////////////////////////////////////////////////////////////////
//// Authors: Mateus Fogaça, Isadora Oliveira and Marcelo Danigno
////
////          (Advisor: Ricardo Reis and Paulo Butzen)
////
//// BSD 3-Clause License
////
//// Copyright (c) 2020, Federal University of Rio Grande do Sul (UFRGS)
//// All rights reserved.
////
//// Redistribution and use in source and binary forms, with or without
//// modification, are permitted provided that the following conditions are met:
////
//// * Redistributions of source code must retain the above copyright notice, this
////   list of conditions and the following disclaimer.
////
//// * Redistributions in binary form must reproduce the above copyright notice,
////   this list of conditions and the following disclaimer in the documentation
////   and/or other materials provided with the distribution.
////
//// * Neither the name of the copyright holder nor the names of its
////   contributors may be used to endorse or promote products derived from
////   this software without specific prior written permission.
////
//// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//// POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////////

#include "GraphDecomposition.h"
#include "opendb/db.h"

namespace PartClusManager{

void GraphDecomposition::initStructs(unsigned dbId){
	_dbWrapper.setDb(dbId);
	_block = _dbWrapper.getBlock();
}

void GraphDecomposition::createGraph(GraphType graphType, Graph &graph){
	for (odb::dbNet* net : _block->getNets()){
		int nITerms = (net->getITerms()).size();
		int nBTerms = (net->getBTerms()).size();
		//if (nITerms + nBTerms > 50)
		//	continue;
		createStarGraph(graph, net);
	}
	transformGraph(graph);
	weightRange(graph, 100);
}

void GraphDecomposition::setWeightingOption(int option){
	_weightingOption = option;
}

float GraphDecomposition::computeWeight(int nPins){
	float weight;
	switch(_weightingOption){
		case 1:
			weight = 1/(float)(nPins-1);
			break;
		case 2:
			weight = 4/(float)(nPins*(nPins-1));
			break;
		case 3:
			weight = 4/(float)(nPins*nPins - (nPins % 2));
			break;
		case 4:
			weight = 6/(float) (nPins*(nPins+1));
			break;
		case 5:
			weight = pow((2/(float)nPins),1.5);
			break;
		case 6:
			weight = pow((2/(float)nPins),3);
			break;			
		case 7:
			weight = 2/(float) nPins;
			break;
	}
	return weight;
}

void GraphDecomposition::weightRange(Graph & graph, int maxRange){
	std::vector<int> edgeWeight = graph.getEdgeWeights();
	std::vector<int> vertexWeight = graph.getVertexWeights();
	
	std::sort(edgeWeight.begin(), edgeWeight.end());
	std::sort(vertexWeight.begin(), vertexWeight.end());

	int eSize = edgeWeight.size();
	int vSize = vertexWeight.size();
	
	eSize = (int)(eSize * 0.99);
	vSize = (int)(vSize * 0.99);
	edgeWeight.resize(eSize);
	edgeWeight.shrink_to_fit();
	vertexWeight.resize(vSize);
	vertexWeight.shrink_to_fit();

	int maxEWeight = *std::max_element(edgeWeight.begin(), edgeWeight.end());
	int maxVWeight = *std::max_element(vertexWeight.begin(), vertexWeight.end());
	int minEWeight = *std::min_element(edgeWeight.begin(), edgeWeight.end());
	int minVWeight = *std::min_element(vertexWeight.begin(), vertexWeight.end());

	for (int & weight : graph.getEdgeWeights()){
		if (weight > maxEWeight)
			weight = maxEWeight;
		if (minEWeight == maxEWeight)
			weight = maxRange;
		else
			weight = (int)((((weight - minEWeight) * (maxRange -1))/(maxEWeight - minEWeight)) + 1);
	}
	for (int & weight : graph.getVertexWeights()){
		if (weight > maxVWeight)
			weight = maxVWeight;
		if (minVWeight == maxVWeight)
			weight = maxRange;
		else
			weight = (int)((((weight - minVWeight) * (maxRange -1))/(maxVWeight - minVWeight)) + 1);
	}
}

void GraphDecomposition::connectStarPins(int firstPin, int secondPin, int weight){
	bool isConnected = false;
	if (firstPin == secondPin)
		return;
	int idx = -1;
	for (std::pair<int, int> i : adjMatrix[firstPin]) {
		idx++;
		if (i.first == secondPin){
			isConnected = true;
			break;
		}
	}	
	if (!isConnected)
		adjMatrix[firstPin].push_back(std::make_pair(secondPin, weight));
	
}

void GraphDecomposition::connectPins(int firstPin, int secondPin, int weight){
	bool isConnected = false;
	if (firstPin == secondPin)
		return;
	int idx = -1;
	for (std::pair<int, int> i : adjMatrix[firstPin]) {
		idx++;
		if (i.first == secondPin){
			isConnected = true;
			break;
		}
	}	
	if (!isConnected){
		adjMatrix[firstPin].push_back(std::make_pair(secondPin, weight));
	} else{
		adjMatrix[firstPin][idx].second += weight;
	}
	
}

void GraphDecomposition::createStarGraph(Graph & graph, odb::dbNet* net){
	std::vector<int> netVertices;
	std::vector<int> & vertexWeights = graph.getVertexWeights();
	std::map<std::string, int> & map = graph.getMap();
	int driveIdx = -1;

	for (odb::dbBTerm* bterm : net->getBTerms()){
		for (odb::dbBPin * pin : bterm->getBPins()){
			if (!graph.isInMap(bterm->getName())){ 
				odb::dbBox * box = pin->getBox();
				long length = box->getLength();
				long width = box->getWidth();
				long area = length * width;
				int nextIdx = graph.getNextIdx(); 
				map[bterm->getName()] = nextIdx;
				vertexWeights.push_back(area);
				std::vector<std::pair<int,int>> aux;
				adjMatrix.push_back(aux);
			}
			netVertices.push_back(map[bterm->getName()]);
			if (bterm->getIoType() == odb::dbIoType::INPUT)
				driveIdx = netVertices.size() - 1;
		}
	}
	
	for (odb::dbITerm * iterm : net->getITerms()){
		odb::dbInst * inst = iterm->getInst();
		if (!graph.isInMap(inst->getName())){
			odb::dbBox * bbox = inst->getBBox();
			int length = bbox->getLength();
			int width = bbox->getWidth();
			int area = length * width;	
			int nextIdx = graph.getNextIdx();
			map[inst->getName()] = nextIdx;
			vertexWeights.push_back(area);
			std::vector<std::pair<int,int>> aux;
			adjMatrix.push_back(aux);
		}
		netVertices.push_back(map[inst->getName()]);
		if (driveIdx == -1)
			if (iterm->isOutputSignal())
				driveIdx= netVertices.size() - 1;
	}
	int weight = 1; 
	if (driveIdx == -1)
		driveIdx = 0;
	for (int i =0; i < netVertices.size(); i++){
		if (i != driveIdx)
			connectStarPins(netVertices[i], netVertices[driveIdx], weight);
			connectStarPins(netVertices[driveIdx], netVertices[i], weight);
	}
}

void GraphDecomposition::createCliqueGraph(Graph & graph, odb::dbNet* net){
	std::vector<int> netVertices;
	std::vector<int> & vertexWeights = graph.getVertexWeights();
	std::map<std::string, int> & map = graph.getMap();
	int nITerms = (net->getITerms()).size();
	int nBTerms = (net->getBTerms()).size();
	for (odb::dbBTerm* bterm : net->getBTerms()){
		for (odb::dbBPin * pin : bterm->getBPins()){
			if (!graph.isInMap(bterm->getName())){ 
				odb::dbBox * box = pin->getBox();
				long length = box->getLength();
				long width = box->getWidth();
				long area = length * width;
				int nextIdx = graph.getNextIdx(); 
				map[bterm->getName()] = nextIdx;
				vertexWeights.push_back(area);
				std::vector<std::pair<int,int>> aux;
				adjMatrix.push_back(aux);
			}
			netVertices.push_back(map[bterm->getName()]);
		}
	}
	
	for (odb::dbITerm * iterm : net->getITerms()){
		odb::dbInst * inst = iterm->getInst();
		if (!graph.isInMap(inst->getName())){
			odb::dbBox * bbox = inst->getBBox();
			int length = bbox->getLength();
			int width = bbox->getWidth();
			int area = length * width;	
			int nextIdx = graph.getNextIdx();
			map[inst->getName()] = nextIdx;
			vertexWeights.push_back(area);
			std::vector<std::pair<int,int>> aux;
			adjMatrix.push_back(aux);
		}
		netVertices.push_back(map[inst->getName()]);
	}
	int weight = (int) computeWeight(nITerms + nBTerms); 
	for (int i =0; i < netVertices.size(); i++){
		for (int j = i+1; j < netVertices.size(); j++){
			connectPins(netVertices[i], netVertices[j], weight);		
			connectPins(netVertices[j], netVertices[i], weight);		
		}
	}
	

}

void GraphDecomposition::transformGraph(Graph & graph){
	std::vector<int> & edgeWeights = graph.getEdgeWeights();
	std::vector<int> & colIdx = graph.getColIdx();
	std::vector<int> & rowPtr = graph.getRowPtr();
	for (std::vector<std::pair<int,int>> node : adjMatrix){
		rowPtr.push_back(edgeWeights.size());
		for (std::pair<int,int> connection : node){
			edgeWeights.push_back(connection.second);
			colIdx.push_back(connection.first);
		}
	}	

}

}
