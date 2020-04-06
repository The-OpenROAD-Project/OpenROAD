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
extern "C" {
    #include "main/ChacoWrapper.h"
}
#include<time.h>
#include <chrono>
#include <fstream>
#include "opendb/db.h"
namespace PartClusManager {

// Partition Netlist

void PartClusManagerKernel::runPartitioning() {
        graph();
        if (_options.getTool() == "mlpart") {
                runMlPart();
        } else if (_options.getTool() == "gpmetis") {
                runGpMetis();
        } else {
                runChaco();
        }
}

void PartClusManagerKernel::runChaco() {
        std::cout << "\nRunning chaco...\n";

        PartSolutions currentResults;
        currentResults.setToolName(_options.getTool());
        unsigned partitionId = generatePartitionId();
        currentResults.setPartitionId(partitionId);
        currentResults.setNumOfRuns(_options.getSeeds().size());
        std::string evaluationFunction = _options.getEvaluationFunction();

        std::vector<int> edgeWeights = _graph.getEdgeWeight();
	std::vector<int> vertexWeights = _graph.getVertexWeight();
	std::vector<int> colIdx = _graph.getColIdx();
	std::vector<int> rowPtr = _graph.getRowPtr();
        int numVertices = vertexWeights.size();
        
        int architecture = _options.getArchTopology().size();
        int* mesh_dims = (int*) malloc((unsigned) 3 * sizeof(int));
        if (architecture > 0){
                std::vector<int> archTopology = _options.getArchTopology();
                for (int i = 0; ((i < architecture) && (i < 3)) ; i++)
                {
                        mesh_dims[i] = archTopology[i];
                }
        }

        int hypercubeDims = (int) (std::sqrt( (float) (_options.getTargetPartitions()) ));

        int numVertCoar = _options.getCoarVertices();

        int termPropagation = 0;
        if (_options.getTermProp()) {
                termPropagation = 1;
        }

        double inbalance = (double) _options.getBalanceConstraint() / 100;

        double coarRatio = _options.getCoarRatio();

        double cutCost = _options.getCutHopRatio();

        for (long seed : _options.getSeeds()) {
                auto start = std::chrono::system_clock::now();
                std::time_t startTime = std::chrono::system_clock::to_time_t(start);

                int* starts = (int*) malloc((unsigned) (numVertices + 1) * sizeof(int));
                int* currentIndex = starts;
                for (int pointer : rowPtr){
                        *currentIndex = (pointer); 
                        currentIndex++;
                }
                *currentIndex = colIdx.size(); // Needed so Chaco can find the end of the interval of the last vertex

                int* vweights = (int*) malloc((unsigned) numVertices * sizeof(int));
                currentIndex = vweights;
                for (int weigth : vertexWeights){
                        *currentIndex = weigth; 
                        currentIndex++;
                }

                int* adjacency = (int*) malloc((unsigned) colIdx.size() * sizeof(int));
                currentIndex = adjacency;
                for (int pointer : colIdx){
                        *currentIndex = (pointer + 1); 
                        currentIndex++;
                }

                float* eweights = (float*) malloc((unsigned) colIdx.size() * sizeof(float));
                float* currentIndexFloat = eweights;
                for (int weigth : edgeWeights){
                        *currentIndexFloat = weigth; 
                        currentIndexFloat++;
                }

                short* assigment = (short*) malloc((unsigned) numVertices * sizeof(short));
                interface_wrap(numVertices,                             /* number of vertices */
                               starts, adjacency, vweights, eweights,   /* graph definition for chaco */
                               NULL, NULL, NULL,                        /* x y z positions for the inertial method, not needed for multi-level KL */
                               NULL, NULL,                              /* output assigment name and file, isn't needed because internal methods of PartClusManager are used instead */
                               assigment,                               /* vertex assigment vector. Contains the set that each vector is present on.*/
                               architecture, hypercubeDims, mesh_dims,  /* architecture, architecture topology and the hypercube dimensions (number of 2-way divisions) */
                               NULL,                                    /* desired set sizes for each set, computed automatically, so it isn't needed */
                               1, 1,                                    /* constants that define the methods used by the partitioner -> multi-level KL, 2-way */
                               0, numVertCoar, 1,                       /* disables the eigensolver, number of vertices to coarsen down to and the number of eigenvectors (hard-coded, not used) */
                               0.001, seed,                             /* tolerance on eigenvectors (hard-coded, not used) and the seed */
                               termPropagation, inbalance,              /* terminal propagation enable and inbalance */
                               coarRatio, cutCost,                      /* coarsening ratio and cut to hop cost */
                               0);                                      /* debug text enable */
                
                std::vector<short> chacoResult;
                for (int i = 0; i < numVertices; i++)
                {
                        short* currentpointer = assigment + i;
                        chacoResult.push_back(*currentpointer);
                }

                auto end = std::chrono::system_clock::now();
                unsigned long runtime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                currentResults.addAssignment(chacoResult, runtime, seed);
                free(assigment);

                std::cout << "Partitioned graph for seed " << seed << " in " << runtime << " ms.\n";
        }
        _results.push_back(currentResults);
        free(mesh_dims);
        computePartitionResult(partitionId, evaluationFunction);

        std::cout << "Chaco run completed. Partition ID = " << partitionId << ".\n";
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
        _graph.clearGraph();
	GraphDecomposition graphDecomp;
	graphDecomp.init(_dbId);
	graphDecomp.createGraph(_graph, _options.getGraphModel(), _options.getWeightModel(), _options.getMaxEdgeWeight(), 
					_options.getMaxVertexWeight(), _options.getCliqueThreshold());
	
}

unsigned PartClusManagerKernel::generatePartitionId(){
        unsigned sizeOfResults = _results.size();
        return sizeOfResults;
}

// Evaluate Partitioning

void PartClusManagerKernel::evaluatePartitioning() {
        std::vector<int> partVector = _options.getPartitionsToTest();
        std::string evaluationFunction = _options.getEvaluationFunction();
        //Checks if IDs are valid
        for (int partId : partVector) {
                if (partId >= _results.size()){
                        std::exit(1);
                }
        }
        int bestId = -1;
        for (int partId : partVector) {
                //Compares the results for the current ID with the best one (if it exists).
                if (bestId == -1) {
                        bestId = partId;
                } else {
                        //If the new ID presents better results than the last one, update the bestId.
                        bool isNewIdBetter = comparePartitionings(getPartitioningResult(bestId),
                                                                  getPartitioningResult(partId),
                                                                  evaluationFunction);
                        if (isNewIdBetter){
                                bestId = partId;
                        }
                }
        }

        reportPartitionResult(bestId);
}

void PartClusManagerKernel::computePartitionResult(unsigned partitionId, std::string function){
        odb::dbDatabase* db = odb::dbDatabase::getDatabase(_dbId);
	odb::dbChip* chip = db->getChip();
	odb::dbBlock* block = chip->getBlock();
        std::vector<unsigned long> setSizes;
        std::vector<unsigned long> setAreas;

        PartSolutions currentResults = _results[partitionId];
        for (unsigned idx = 0; idx < currentResults.getNumOfRuns(); idx++)
        {
                std::vector<short> currentAssignment = currentResults.getAssignment(idx);
                unsigned long currentRuntime = currentResults.getRuntime(idx);
                int currentSeed = currentResults.getSeed(idx);

                unsigned long terminalCounter = 0; 
                unsigned long cutCounter = 0; 
                unsigned long edgeTotalWeigth = 0; 
                std::map<short, unsigned long> setSize; 
                std::map<short, unsigned long> setArea; 
                std::vector<unsigned> computedVertices;

                for (odb::dbNet* net : block->getNets()){
                        int nITerms = (net->getITerms()).size();
                        int nBTerms = (net->getBTerms()).size();
                        if (net->isSpecial() || (nITerms + nBTerms) < 2){
                                continue;
                        }

                        short currentInstPart = 9999;
                        bool edgeWasCut = false;
                        std::vector<short> parentPartitions;
                        std::vector<unsigned> netVertices;
                        
                        //Terminals ----------
                        for (odb::dbITerm* iterm : net->getITerms()){
                                odb::dbInst* inst = iterm->getInst();
                                if ( !(_graph.isInMap(inst->getName())) ){
                                        edgeWasCut = true;
                                        terminalCounter = terminalCounter + 1;
                                        continue;
                                }
                                unsigned idx = _graph.getMapping(inst->getName());
                                short cl = currentAssignment[idx];
                                //Got the cluster that the ITerm was part of.
                                if (currentInstPart == 9999){
                                        //If this is the first pass, set the cluster index in the currentInstPart variable.
                                        currentInstPart = cl;
                                } else if ((currentInstPart != cl) && (std::find(parentPartitions.begin(), parentPartitions.end(), cl) == parentPartitions.end())){
                                        //If the cluster changed, and it change to a cluster that wasn't previously computed in this net, add a new terminal to the old cluster.
                                        parentPartitions.push_back(currentInstPart); //Old cluster can't be used anymore for this net.
                                        terminalCounter = terminalCounter + 1;
                                        //The new cluster becomes the old one.
                                        currentInstPart = cl;
                                        //The edge was CUT! (new hypercut edge.)
                                        edgeWasCut = true;
                                }
                                netVertices.push_back(idx);
                                //Set size and area ----------
                                if (std::find(computedVertices.begin(), computedVertices.end(), idx) == computedVertices.end()){
                                        if ( setSize.find(cl) == setSize.end() ) {
                                                setSize[cl] = 1;
                                                setArea[cl] = _graph.getVertexWeight(idx);
                                        } else {
                                                setSize[cl] = setSize[cl] + 1;
                                                setArea[cl] = setArea[cl] + _graph.getVertexWeight(idx);
                                        }
                                        computedVertices.push_back(idx);
                                }
                        }
                        for (odb::dbBTerm* bterm : net->getBTerms()){
                                if ( !(_graph.isInMap(bterm->getName())) ){
                                        edgeWasCut = true;
                                        terminalCounter = terminalCounter + 1;
                                        continue;
                                }
                                unsigned idx = _graph.getMapping(bterm->getName());
                                short cl = currentAssignment[idx];
                                //Got the cluster that the BTerm was part of.
                                if (currentInstPart == 9999){
                                        //If this is the first pass, set the cluster index in the currentInstPart variable.
                                        currentInstPart = cl;
                                } else if ((currentInstPart != cl) && (std::find(parentPartitions.begin(), parentPartitions.end(), cl) == parentPartitions.end())){
                                        //If the cluster changed, and it change to a cluster that wasn't previously computed in this net, add a new terminal to the old cluster.
                                        parentPartitions.push_back(currentInstPart); //Old cluster can't be used anymore for this net.
                                        terminalCounter = terminalCounter + 1;
                                        //The new cluster becomes the old one.
                                        currentInstPart = cl;
                                        //The edge was CUT! (new hypercut edge.)
                                        edgeWasCut = true;
                                }
                                netVertices.push_back(idx);
                                //Set size and area ----------
                                if (std::find(computedVertices.begin(), computedVertices.end(), idx) == computedVertices.end()){
                                        if ( setSize.find(cl) == setSize.end() ) {
                                                setSize[cl] = 1;
                                                setArea[cl] = _graph.getVertexWeight(idx);
                                        } else {
                                                setSize[cl] = setSize[cl] + 1;
                                                setArea[cl] = setArea[cl] + _graph.getVertexWeight(idx);
                                        }
                                        computedVertices.push_back(idx);
                                }
                        }
                        if (edgeWasCut && (std::find(parentPartitions.begin(), parentPartitions.end(), currentInstPart) == parentPartitions.end()) && (currentInstPart != 9999)){
                                //If the edge was cut, and the last cluster wasn't previously computed for this new, add a new terminal to that cluster.
                                terminalCounter = terminalCounter + 1;
                        }
                        //HyperEdge Cuts ----------
                        if (edgeWasCut){
                                //If the edge was cut, a hyperedge cut is present for this net.
                                cutCounter = cutCounter + 1;
                                //Also add the total weigth for this hyperedge cut.
                                unsigned currentVertex = netVertices.back();
                                netVertices.pop_back();
                                int currentRow = _graph.getRowPtr(currentVertex);
                                unsigned vertexEnd = 0;
                                if (_graph.getRowPtr().size() == (currentVertex + 1)){
                                        vertexEnd = _graph.getColIdx().size();
                                } else {
                                        vertexEnd = _graph.getRowPtr(currentVertex + 1);
                                }
                                while (currentRow < vertexEnd)
                                {
                                        unsigned currentConnection = _graph.getColIdx(currentRow);
                                        if (std::find(netVertices.begin(), netVertices.end(), currentConnection) != netVertices.end()) {
                                                edgeTotalWeigth = edgeTotalWeigth + _graph.getEdgeWeight(currentRow);
                                        }
                                        currentRow++;
                                }
                        }
                }
                
                //Computation for the standard deviation of set size 
                double currentSum = 0;
                for (std::pair<short, unsigned long> clusterSize : setSize) {
                        currentSum = currentSum + clusterSize.second;
                }
                double currentMean = currentSum / setSize.size();
                double sizeSD = 0;
                for (std::pair<short, unsigned long> clusterSize : setSize) {
                        sizeSD = sizeSD + std::pow(clusterSize.second - currentMean, 2);
                }
                sizeSD = std::sqrt(sizeSD / setSize.size());

                //Computation for the standard deviation of set area 
                currentSum = 0;
                for (std::pair<short, unsigned long> clusterArea : setArea) {
                        currentSum = currentSum + clusterArea.second;
                }
                currentMean = currentSum / setArea.size();
                double areaSD = 0;
                for (std::pair<short, unsigned long> clusterArea : setArea) {
                        areaSD = areaSD + std::pow(clusterArea.second - currentMean, 2);
                }
                areaSD = std::sqrt(areaSD / setArea.size());

                //Check if the current assignment is better than the last one. "terminals hyperedges size area runtime hops"
                bool isBetter = false;
                if (function == "hyperedges") {
                        isBetter = ((cutCounter < currentResults.getBestNumHyperedgeCuts()) || (currentResults.getBestNumHyperedgeCuts() == 0));
                } else if (function == "terminals") {
                        isBetter = ((terminalCounter < currentResults.getBestNumTerminals()) || (currentResults.getBestNumTerminals() == 0));
                } else if (function == "size") {
                        isBetter = ((sizeSD < currentResults.getBestSetSize()) || (currentResults.getBestSetSize() == 0));
                } else if (function == "area") {
                        isBetter = ((areaSD < currentResults.getBestSetArea()) || (currentResults.getBestSetArea() == 0));
                } else if (function == "hops") {
                        isBetter = ((edgeTotalWeigth < currentResults.getBestHopWeigth()) || (currentResults.getBestHopWeigth() == 0));
                } else {
                        isBetter = ((currentRuntime < currentResults.getBestRuntime()) || (currentResults.getBestRuntime() == 0));
                }
                if (isBetter){
                        currentResults.setBestSolutionIdx(idx);
                        currentResults.setBestRuntime(currentRuntime);
                        currentResults.setBestNumHyperedgeCuts(cutCounter);
                        currentResults.setBestNumTerminals(terminalCounter);
                        currentResults.setBestHopWeigth(edgeTotalWeigth);
                        currentResults.setBestSetSize(sizeSD);
                        currentResults.setBestSetArea(areaSD);
                }
        }
        _results[partitionId] = currentResults;
}

bool PartClusManagerKernel::comparePartitionings(PartSolutions oldPartition, PartSolutions newPartition, std::string function) {
        bool isBetter = false;
        if (function == "hyperedges") {
                isBetter = newPartition.getBestNumHyperedgeCuts() < oldPartition.getBestNumHyperedgeCuts();
        } else if (function == "terminals") {
                isBetter = newPartition.getBestNumTerminals() < oldPartition.getBestNumTerminals();
        } else if (function == "size") {
                isBetter = newPartition.getBestSetSize() < oldPartition.getBestSetSize();
        } else if (function == "area") {
                isBetter = newPartition.getBestSetArea() < oldPartition.getBestSetArea();
        } else if (function == "hops") {
                isBetter = newPartition.getBestHopWeigth() < oldPartition.getBestHopWeigth();
        } else {
                isBetter = newPartition.getBestRuntime() < oldPartition.getBestRuntime();
        }
        return isBetter;
}

void PartClusManagerKernel::reportPartitionResult(unsigned partitionId){
        PartSolutions currentResults = _results[partitionId];
        std::cout << "\nPartitioning Results for ID = " << partitionId << " and Tool = " << currentResults.getToolName() << ".\n";
        unsigned bestIdx = currentResults.getBestSolutionIdx();
        int seed = currentResults.getSeed(bestIdx);
        std::cout << "Best results used seed " << seed << ".\n";
        std::cout << "Number of Hyperedge Cuts = " << currentResults.getBestNumHyperedgeCuts() << ".\n";
        std::cout << "Number of Terminals = " << currentResults.getBestNumTerminals() << ".\n";
        std::cout << "Cluster Size SD = " << currentResults.getBestSetSize() << ".\n";
        std::cout << "Cluster Area SD = " << currentResults.getBestSetArea() << ".\n";
        std::cout << "Total Hop Weigth = " << currentResults.getBestHopWeigth() << ".\n";
        std::cout << "Total Runtime = " << currentResults.getBestRuntime() << ".\n\n";
}

// Write Partitioning To DB

odb::dbBlock* PartClusManagerKernel::getDbBlock() const {
        odb::dbDatabase* db = odb::dbDatabase::getDatabase(_dbId);
        odb::dbChip* chip = db->getChip();
        odb::dbBlock* block = chip->getBlock();
        return block;
}

void PartClusManagerKernel::writePartitioningToDb(unsigned partitioningId) {
        std::cout << "[INFO] Writing partition id's to DB.\n";
        if (partitioningId >= getNumPartitioningResults()) {
                std::cout << "[ERROR] Partition id out of range ("
                          << partitioningId << ").\n";
                return;
        }

        PartSolutions &results = getPartitioningResult(partitioningId);
        unsigned bestSolutionIdx = results.getBestSolutionIdx();
        const std::vector<short>& result = results.getAssignment(bestSolutionIdx);

        odb::dbBlock* block = getDbBlock();
        for (odb::dbInst* inst: block->getInsts()) {
                std::string instName = inst->getName();
                int instIdx = _graph.getMapping(instName);
                short partitionId = result[instIdx];
                
                odb::dbIntProperty* propId = odb::dbIntProperty::find(inst, "partition_id");
                if (!propId) {
                        propId = odb::dbIntProperty::create(inst, "partition_id", partitionId);
                } else {
                        propId->setValue(partitionId);
                }
        }

        std::cout << "[INFO] Writing done.\n";
}

void PartClusManagerKernel::dumpPartIdToFile(std::string name) {
        std::ofstream file(name);

        odb::dbBlock* block = getDbBlock();
        for (odb::dbInst* inst: block->getInsts()) {
                std::string instName = inst->getName();
                odb::dbIntProperty* propId = odb::dbIntProperty::find(inst, "partition_id");
                if (!propId) {
                        std::cout << "[ERROR] Property not found for inst " << instName << "\n";
                        continue;
                }
                file << instName << " " << propId->getValue() << "\n";
        }

        file.close();
}

}
