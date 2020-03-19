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
namespace PartClusManager {

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
        std::cout << "Running chaco...\n";
        std::vector<int> edgeWeights = _graph.getEdgeWeight();
	std::vector<double> vertexWeights = _graph.getVertexWeight();
	std::vector<int> colIdx = _graph.getColIdx();
	std::vector<int> rowPtr = _graph.getRowPtr();
        int numVertices = vertexWeights.size();
        
        int architecture = 0;
        if (_options.getArchitecture()){
                architecture = _options.getArchTopology().size();
        }
        int* mesh_dims = (int*) malloc((unsigned) 3 * sizeof(int));
        if (_options.getArchitecture()){
                std::vector<int> archTopology = _options.getArchTopology();
                for (int i = 0; i < architecture; i++)
                {
                        mesh_dims[i] = archTopology[i];
                }
        }

        int hypercubeDims = (int) (std::sqrt( (float) (_options.getTargetPartitions()) ));

        int numVertCoar = _options.getCoarVertices();

        std::srand(time(NULL));
        long seed = std::rand();

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
        interface_wrap(numVertices,                                       /* number of vertices */
                       starts, adjacency, vweights, eweights,   /* graph definition for chaco */
                       NULL, NULL, NULL,                        /* x y z positions for the inertial method, not needed for multi-level KL */
		       NULL, NULL,                              /* output assigment name and file, isn't needed because internal methods of PartClusManager are used instead */
		       assigment,                               /* vertex assigment vector. Contains the set that each vector is present on.*/
                       architecture, hypercubeDims, mesh_dims,  /* architecture, architecture topology and the hypercube dimensions (number of 2-way divisions) */
                       NULL,                                    /* desired set sizes for each set, computed automatically, so it isn't needed */
		       1, 1,                                    /* constants that define the methods used by the partitioner -> multi-level KL, 2-way */
                       0, numVertCoar, 1,                       /* disables the eigensolver, number of vertices to coarsen down to and the number of eigenvectors (hard-coded, not used) */
		       0.001, seed);                            /* tolerance on eigenvectors (hard-coded, not used) and the seed */
        
        for (int i = 0; i < numVertices; i++)
	{
		short* currentpointer = assigment + i;
                _graph.addAssignment(*currentpointer);
	}

        free(assigment);
        free(mesh_dims);

        std::cout << "Chaco run completed. Seed used: " << seed << ".\n";
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
	graphDecomp.createGraph(_graph, _options.getGraphModel(), _options.getWeightModel(), _options.getMaxEdgeWeight(), 
					_options.getMaxVertexWeight(), _options.getCliqueThreshold());
}

}
