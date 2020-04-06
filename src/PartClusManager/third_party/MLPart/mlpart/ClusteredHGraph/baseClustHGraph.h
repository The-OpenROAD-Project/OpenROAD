/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation
***  the rights to use, copy, modify, merge, publish, distribute, sublicense,
***  and/or sell copies of the Software, and to permit persons to whom the
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/

#ifndef _BASE_CLUSTEREDHGRAPH_H_
#define _BASE_CLUSTEREDHGRAPH_H_

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <ClusteredHGraph/clustHGRegistry.h>
#include <Combi/combi.h>
#include <HGraph/hgFixed.h>
#include <Partitioning/partitionData.h>

// this is the base object produced by the clustering algorithms.
// it contains some number of pointers to HGraphs, maps between
// the nodes of each HGraph, and the one following it, and
// bits indicating if this BaseClusteredHGraph owns each HGraph,
// or not (for instance..it will typically not own the bottom
// level hgraph, but will own all the rest).

class Partitioning;

class BaseClusteredHGraph {
       protected:
        uofm::vector<HGraphFixed*> _clusteredHGraphs;
        // the bottom level HGraph is at index 0. So..ML would start at
        // the highest numbered HGraph (_clusteredHGraphs.size()-1),
        // and work down to 0 (the original hgraph).

        uofm_bit_vector _ownsHGraph;

        uofm::vector<uofm::vector<unsigned> > _clusterMap;
        //_clusterMap[k] is the mapping from _clusteredHGraphs[k+1]->
        //_clusteredHGraphs[k], for k >= 0.
        // This means that _clusterMap[k].size() ==
        //			_clusteredHGraphs[k]->getNumNodes().
        // And that _clusterMap[k] is a vector of unsigned in the
        // range >= 0 and < _clHGrs[k-1]->getNumNodes.

        uofm::vector<Partitioning> _fixedConst;  // one per level

        Partitioning _topLevelPart;

        unsigned _numTerminals;
        const Partitioning* _origPart;  // for clustering relative to
                                        // a partition. Null-> there isn't
                                        // one (not built from a partititoning)

       public:
        class Parameters {
               public:
                ClHG_ClusteringType clType;
                double clusterRatio;  // target rate of decrease of
                // size of levels. Must be > 1.0.
                // 1.3 means the bottom level is
                // 1.3x the size of the one above it.
                // 1.3 is the default
                double maxNewClRatio;  // limit on the size of clusters
                // that can be created.  This is
                // a multiple of the average size
                // at the level. expressed as a
                // percent. That is, a value of 3.0 means
                // //no cluster can be made which will
                // have a size > 3x the average at the
                // current top level.
                double maxChildClRatio;  // similar to the maxClRatio,
                // but this is a limit on the area
                // the nodes being clustered can have,
                // not a limit on the size of the
                // new CL being created.
                double maxClArea;  // a HARD limit on the max area of
                // a cluster, expressed as a percentage.
                // 1 = 1% of total area.
                // the max area of a newly created
                // cluster is the
                // min(maxClArea,maxNewClRatio).
                // note that, unlike the Ratio params,
                // this one does not 'grow' with levels.

                unsigned sizeOfTop;  // stop when the top level has
                //<= sizeOfTop clusters

                double levelGrowth;  // rate at which the 'snapshot' sizes
                // decrease

                unsigned removeDup;  // 0 == none
                // 2 == only degree 2 edges
                // 3 == only degree 2 & 3 edges

                unsigned weightOption;  // 1 == divide by min
                // 2 == divide by max
                // 3 == divide by sum
                // 4 == divide by product
                // 5 -- divide by sqrt(max)
                // 6 == divide by sqrt(sum)
                // 7 == divide by sqrt(product)

                bool dontClusterTerms;  // if true, terminals may not be
                // clustered

                Verbosity verb;

                Parameters();
                Parameters(int argc, const char* argv[]);
                Parameters(Parameters const& p);
                Parameters(ClHG_ClusteringType cltype);
                ~Parameters() {}

                friend std::ostream& operator<<(std::ostream& os, const Parameters& params);
        };

       protected:
        Parameters _params;

       public:
        BaseClusteredHGraph(const HGraphFixed& leafLevel, const Parameters& params, const Partitioning* fixedConst = NULL, const Partitioning* origPart = NULL);

        virtual ~BaseClusteredHGraph();
        void saveAsPartitioning(const char* filename);
        bool wasBuiltFromPartitioning() const { return _origPart != NULL; }
        const Partitioning& getTopLevelPart() const { return _topLevelPart; }

        const HGraphFixed& getHGraph(unsigned level) const {
                abkfatal(level < _clusteredHGraphs.size(), "invalid level");
                return *(_clusteredHGraphs[level]);
        }

        void destroyHGraph(unsigned level) {
                abkfatal(level < _clusteredHGraphs.size(), "invalid level");
                if (_ownsHGraph[level]) {
                        delete _clusteredHGraphs[level];
                        _clusteredHGraphs[level] = NULL;
                }
                if (level < _clusterMap.size()) {
                        _clusterMap[level] = uofm::vector<unsigned>(0);
                }
        }

        const Partitioning& getFixedConst(unsigned level) {
                abkfatal3(level < _fixedConst.size(), "invalid level (", level, ")");
                return _fixedConst[level];
        }
        int determineTopLevelPartition(int index, int level);

        unsigned getNumLevels() const { return _clusteredHGraphs.size(); }
        unsigned getNumTerminals() const { return _clusteredHGraphs[0]->getNumTerminals(); }
        unsigned getNumLeafNodes() const { return _clusteredHGraphs[0]->getNumNodes(); }

        void mapPartitionings(const Partitioning& srcPart, Partitioning& destPart, unsigned level);  // level is the lvl of the
                                                                                                     // destPart
        // note: the top (most clustered) level is the highest numbered.
        // the bottom level is numbered 0.

        const uofm::vector<unsigned>& getMapping(unsigned level) const {
                abkassert(level >= 0 && level < _clusteredHGraphs.size() - 1, "out of range level in 'getMapping'");
                return _clusterMap[level];
        }

        // used for debug
        unsigned computeCut(const HGraphFixed& graph, const Partitioning& part);
};

typedef BaseClusteredHGraph::Parameters ClustHGraphParameters;

#endif
