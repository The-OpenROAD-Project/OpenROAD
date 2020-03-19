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

#include "TclInterface.h"
#include "PartClusManagerKernel.h"
#include "openroad/OpenRoad.hh"
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>
#include <time.h>

namespace PartClusManager {

void set_tool(const char* name) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setTool(name);
}

void set_target_partitions(unsigned value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setTargetPartitions(value);
}

void set_graph_model(const char* name) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setGraphModel(name);
}

void set_clique_threshold(unsigned value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setCliqueThreshold(value);
}

void set_weight_model(unsigned value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setWeightModel(value);
}

void set_max_edge_weight(unsigned value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setMaxEdgeWeight(value);
}

void set_max_vertex_weight(unsigned value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setMaxVertexWeight(value);
}

void set_num_starts(unsigned value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setNumStarts(value);
}

void set_balance_constraints(unsigned value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setBalanceConstraint(value);
}

void set_coarsening_ratio(float value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setCoarRatio(value);
}

void set_enable_term_prop(bool value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setTermProp(value);
}

void set_cut_hop_ratio(float value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->getOptions().setCutHopRatio(value);
}

void set_architecture(const char* topology) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        std::stringstream ss(topology);
        std::istream_iterator<int> begin(ss);
        std::istream_iterator<int> end;
        std::vector<int> archTopology(begin, end);
        kernel->getOptions().setArchTopology(archTopology);
}

void set_seeds(const char* seeds) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        std::stringstream ss(seeds);
        std::istream_iterator<int> begin(ss);
        std::istream_iterator<int> end;
        std::vector<int> seedVector(begin, end);
        kernel->getOptions().setSeeds(seedVector);
}

void generate_seeds(unsigned value) {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        std::vector<int> seedVector;
        for (int i = 0; i < value; i++)
        {
                int seedVar = 5;
                int seed = 0;
                do
                {
                        std::srand(time(NULL) + (i * 1000) + (seedVar * 50));
                        seed = std::rand();
                        seedVar += 5;
                } while (std::find(seedVector.begin(), seedVector.end(), seed) != seedVector.end());
                seedVector.push_back(seed);
        }
        kernel->getOptions().setSeeds(seedVector);
}

void run_partitioning() {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        PartClusManagerKernel* kernel = openroad->getPartClusManager();
        kernel->runPartitioning();
}

} // end namespace
