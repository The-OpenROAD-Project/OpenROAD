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

#include "HypergraphDecomposition.h"
#include "opendb/db.h"
#include <fstream>

namespace PartClusManager{

void HypergraphDecomposition::init(int dbId){
	_db = odb::dbDatabase::getDatabase(dbId);
	_chip = _db->getChip();
	_block = _chip->getBlock();
}

void HypergraphDecomposition::constructMap(Graph & graph, unsigned maxVertexWeight){
	for (odb::dbNet* net : _block->getNets()){
		int nITerms = (net->getITerms()).size();
		int nBTerms = (net->getBTerms()).size();
		if (nITerms + nBTerms < 2)
			continue;
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
				}
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
			}
		}
	}
	graph.computeVertexWeightRange(maxVertexWeight);
}


void HypergraphDecomposition::createHypergraph(Graph &graph, std::vector<short> clusters , short currentCluster){
	for (odb::dbNet* net : _block->getNets()){
		int nITerms = (net->getITerms()).size();
		int nBTerms = (net->getBTerms()).size();
		if (nITerms + nBTerms < 2)
			continue;

		int nextPtr =  graph.computeNextRowPtr();
		graph.addRowPtr(nextPtr);
		graph.addEdgeWeightNormalized(1);
		
		for (odb::dbBTerm* bterm : net->getBTerms()){
			for (odb::dbBPin * pin : bterm->getBPins()){
				int mapping = graph.getMapping(bterm->getName());
				if (clusters[mapping] == currentCluster){
					graph.addColIdx(mapping);	
				} 
			}
		}
	
		for (odb::dbITerm * iterm : net->getITerms()){
			odb::dbInst * inst = iterm->getInst();
			int mapping = graph.getMapping(inst->getName());
			if (clusters[mapping] == currentCluster){
				graph.addColIdx(mapping);	
			} 
		}
	}
	int nextPtr =  graph.computeNextRowPtr();
	graph.addRowPtr(nextPtr);
}

}
