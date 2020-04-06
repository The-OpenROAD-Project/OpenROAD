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
#include <fstream>

namespace PartClusManager{

void GraphDecomposition::init(int dbId){
	_db = odb::dbDatabase::getDatabase(dbId);
	_chip = _db->getChip();
	_block = _chip->getBlock();
}

GraphType GraphDecomposition::resolveModel(std::string graphModel){
	if (graphModel == "clique"){
		return CLIQUE;
	}
	if (graphModel == "star"){
		return STAR;
	}
	if (graphModel == "hybrid"){
		return HYBRID;
	}
}

void GraphDecomposition::createGraph(Graph &graph, std::string graphModelS, unsigned weightingOption, 
				unsigned maxEdgeWeight, unsigned maxVertexWeight, unsigned threshold){

	_weightingOption = weightingOption;
	GraphType graphModel = resolveModel(graphModelS);

	for (odb::dbNet* net : _block->getNets()){
		int nITerms = (net->getITerms()).size();
		int nBTerms = (net->getBTerms()).size();
		if (nITerms + nBTerms < 2)
			continue;
		switch(graphModel){
			case CLIQUE:
				if (nITerms + nBTerms > threshold)
					continue;
				createCliqueGraph(graph, net);
				break;
			case STAR:
				createStarGraph(graph, net);
				break;
			case HYBRID:
				if (nITerms + nBTerms > threshold){ 
					createStarGraph(graph, net);
				} else{ 
					createCliqueGraph(graph, net);
				}
				break;
		}
	}
	createCompressedMatrix(graph);
	graph.computeVertexWeightRange(maxVertexWeight);
	graph.computeEdgeWeightRange(maxEdgeWeight);
}

float GraphDecomposition::computeWeight(int nPins){
	switch(_weightingOption){
		case 1:
			return 1.0/(nPins-1);
		case 2:
			return 4.0/(nPins*(nPins-1));
		case 3:
			return 4.0/(nPins*nPins - (nPins % 2));
		case 4:
			return 6.0/(nPins*(nPins+1));
		case 5:
			return pow((2.0/nPins),1.5);
		case 6:
			return pow((2.0/nPins),3);
		case 7:
			return 2.0/nPins;
	}
}

void GraphDecomposition::connectStarPins(int firstPin, int secondPin, float weight){
	bool isConnected = false;
	if (firstPin == secondPin)
		return;
        if (adjMatrix[firstPin].find(secondPin) != adjMatrix[firstPin].end())
			isConnected = true;

	if (!isConnected)
		adjMatrix[firstPin][secondPin] = weight;
	
}

void GraphDecomposition::connectPins(int firstPin, int secondPin, float weight){
	bool isConnected = false;
	if (firstPin == secondPin)
		return;
        if (adjMatrix[firstPin].find(secondPin) != adjMatrix[firstPin].end())
			isConnected = true;

	if (!isConnected){
		adjMatrix[firstPin][secondPin] = weight;
	} else{
		adjMatrix[firstPin][secondPin] += weight;
	}
	
}

void GraphDecomposition::createStarGraph(Graph & graph, odb::dbNet* net){
	std::vector<int> netVertices;
	int driveIdx = -1;

	for (odb::dbBTerm* bterm : net->getBTerms()){
		for (odb::dbBPin * pin : bterm->getBPins()){
			if (!graph.isInMap(bterm->getName())){ 
				odb::dbBox * box = pin->getBox();
				long long int length = box->getLength();
				long long int width = box->getWidth();
				long long int area = length * width;
				int nextIdx = graph.computeNextVertexIdx(); 
				graph.addMapping(bterm->getName(), nextIdx);
				graph.addVertexWeight(area);
				std::map<int,float> aux;
				adjMatrix.push_back(aux);
			}
			netVertices.push_back(graph.getMapping(bterm->getName()));
			if (bterm->getIoType() == odb::dbIoType::INPUT)
				driveIdx = netVertices.size() - 1;
		}
	}
	
	for (odb::dbITerm * iterm : net->getITerms()){
		odb::dbInst * inst = iterm->getInst();
		if (!graph.isInMap(inst->getName())){
			odb::dbBox * bbox = inst->getBBox();
			long long int length = bbox->getLength();
			long long int width = bbox->getWidth();
			long long int area = length * width;	
			int nextIdx = graph.computeNextVertexIdx();
			graph.addMapping(inst->getName(), nextIdx);
			graph.addVertexWeight(area);
			std::map<int,float> aux;
			adjMatrix.push_back(aux);
		}
		netVertices.push_back(graph.getMapping(inst->getName()));
		if (driveIdx == -1)
			if (iterm->isOutputSignal())
				driveIdx= netVertices.size() - 1;
	}
	float weight = 1; 
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
	int nITerms = (net->getITerms()).size();
	int nBTerms = (net->getBTerms()).size();
	for (odb::dbBTerm* bterm : net->getBTerms()){
		for (odb::dbBPin * pin : bterm->getBPins()){
			if (!graph.isInMap(bterm->getName())){ 
				odb::dbBox * box = pin->getBox();
				long long int length = box->getLength();
				long long int width = box->getWidth();
				long long int area = length * width;
				int nextIdx = graph.computeNextVertexIdx(); 
				graph.addMapping(bterm->getName(), nextIdx);
				graph.addVertexWeight(area);
				std::map<int,float> aux;
				adjMatrix.push_back(aux);
			}
			netVertices.push_back(graph.getMapping(bterm->getName()));
		}
	}
	
	for (odb::dbITerm * iterm : net->getITerms()){
		odb::dbInst * inst = iterm->getInst();
		if (!graph.isInMap(inst->getName())){
			odb::dbBox * bbox = inst->getBBox();
			long long int length = bbox->getLength();
			long long int width = bbox->getWidth();
			long long int area = length * width;	
			int nextIdx = graph.computeNextVertexIdx();
			graph.addMapping(inst->getName(), nextIdx);
			graph.addVertexWeight(area);
			std::map<int,float> aux;
			adjMatrix.push_back(aux);
		}
		netVertices.push_back(graph.getMapping(inst->getName()));
	}
	float weight = computeWeight(nITerms + nBTerms); 
	for (int i =0; i < netVertices.size(); i++){
		for (int j = i+1; j < netVertices.size(); j++){
			connectPins(netVertices[i], netVertices[j], weight);		
			connectPins(netVertices[j], netVertices[i], weight);		
		}
	}
	

}

void GraphDecomposition::createCompressedMatrix(Graph & graph){

	for (std::map<int,float> & node : adjMatrix){
		int nextPtr = graph.computeNextRowPtr();
		graph.addRowPtr(nextPtr);
		for (std::pair<const int,float> & connection : node){
			graph.addEdgeWeight(connection.second);
			graph.addColIdx(connection.first);
		}
	}	
	
}

}
