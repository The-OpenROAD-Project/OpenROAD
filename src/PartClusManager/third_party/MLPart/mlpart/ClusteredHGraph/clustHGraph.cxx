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

// created on 09/09/98 by Andrew Caldwell (caldwell@cs.ucla.edu)

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "clustHGraph.h"
#include "FilledHier/fillHier.h"
#include "FilledHier/hgForHierarchy.h"
#include <set>

using std::set;
using std::cout;
using std::endl;
using uofm::vector;

ClusteredHGraph::ClusteredHGraph(const HGraphFixed& graph, const Parameters& params) : ClHG_ClusterTreeBase(graph, params, NULL), HEMClusteredHGraph(graph, params), PinHEMClusteredHGraph(graph, params), BestHEMClusteredHGraph(graph, params), GreedyHEMClusteredHGraph(graph, params), CutOptClusteredHGraph(graph, params), RandomClusteredHGraph(graph, params) { setupTree(); }

ClusteredHGraph::ClusteredHGraph(const HGraphFixed& graph, const Parameters& params, const Partitioning& fixed) : ClHG_ClusterTreeBase(graph, params, &fixed, NULL), HEMClusteredHGraph(graph, params, &fixed, NULL), PinHEMClusteredHGraph(graph, params, &fixed, NULL), BestHEMClusteredHGraph(graph, params, &fixed, NULL), GreedyHEMClusteredHGraph(graph, params, &fixed, NULL), CutOptClusteredHGraph(graph, params), RandomClusteredHGraph(graph, params, &fixed, NULL) { setupTree(); }

ClusteredHGraph::ClusteredHGraph(const HGraphFixed& graph, const Parameters& params, const Partitioning& fixed, const Partitioning& curPart, PlacementWOrient* placement) : ClHG_ClusterTreeBase(graph, params, &fixed, &curPart), HEMClusteredHGraph(graph, params, &fixed, &curPart, placement), PinHEMClusteredHGraph(graph, params, &fixed, &curPart), BestHEMClusteredHGraph(graph, params, &fixed, &curPart), GreedyHEMClusteredHGraph(graph, params, &fixed, &curPart), CutOptClusteredHGraph(graph, params), RandomClusteredHGraph(graph, params, &fixed, &curPart) { setupTree(); }

ClusteredHGraph::ClusteredHGraph(FillableHierarchy& fillH, const HGraphFixed& leafHG, const Parameters& params) : ClHG_ClusterTreeBase(leafHG, params, NULL), HEMClusteredHGraph(leafHG, params), PinHEMClusteredHGraph(leafHG, params), BestHEMClusteredHGraph(leafHG, params), GreedyHEMClusteredHGraph(leafHG, params), CutOptClusteredHGraph(leafHG, params), RandomClusteredHGraph(leafHG, params) { createLevels(fillH); }

// this is a total HACK.  But, set is templated by the type
// of the comparitor, not parameterized by it. So, you can't
// pass the FillableHierarchy to the set used in the function below.
// This means that a global pointer to the FillableHierarchy
// is the only option.  Don't try this at home, kids...

FillableHierarchy* _ClHG_FillableHier_HACK_;

class CompareNodesBySize {
       public:
        CompareNodesBySize() {}

        // note that terminal clusters have zero area, and hence
        // will sort to the end.
        bool operator()(unsigned n1, unsigned n2) const { return _ClHG_FillableHier_HACK_->getClusterArea(n1) >= _ClHG_FillableHier_HACK_->getClusterArea(n2); }
};

void ClusteredHGraph::createLevels(const FillableHierarchy& fillH) {
        //_toCluster already contains nodes that can be
        // combined.
        // this function creates clustered netlists from the
        // generic hierarchy.  fillH is assumed to have induced
        // clustered nets for all nodes in it, so that we
        // may use the HGraph ctor to create the level hgraphs.
        // All we need to maintain is the mapping from one level to the
        // next.

        abkfatal(_clusteredHGraphs.size() == 1, "createLevels expects that the base hgraph is at the bottom");
        abkfatal(_ownsHGraph.size() == _clusteredHGraphs.size(), "ownsHGraph.size does not match number of level hgraphs");

        _ClHG_FillableHier_HACK_ = const_cast<FillableHierarchy*>(&fillH);

        unsigned sizeOfBottom = _clusteredHGraphs[0]->getNumNodes();

        // terminals are not included in the level size targets
        // specified in the parameters (sizeOfTop), nor included in
        // the level growth computations
        unsigned numTerminals = _clusteredHGraphs[0]->getNumTerminals();
        unsigned numZeroArea = 0;
        unsigned curLevelTargetSize = _params.sizeOfTop;

        // tracks the node's ancestors that were in the previously
        // saved slice...this is used for creating the mappings.
        // it is indexed by node (globally unique) id's, and for each
        // node in the slice, gives the (HGraph)id of its ancestor in
        // the most recently saved level hgraph.
        vector<unsigned> ancestorInSlice(fillH.getNumNodes(), UINT_MAX);

        if (curLevelTargetSize >= sizeOfBottom) {
                PartitionIds everywhere;
                everywhere.setToAll(32);
                _topLevelPart = Partitioning(sizeOfBottom, everywhere);
                return;
        }

        vector<unsigned> leafNodes;
        leafNodes.reserve(sizeOfBottom);
        set<unsigned, CompareNodesBySize> slice;
        unsigned sizeOfSlice = 0;

        // take care of the zero-area nodes first
        const vector<unsigned>& rootChildren = fillH.getChildren(fillH.getRootId());

        unsigned z;
        for (z = 0; z < rootChildren.size(); z++) {
                if (fillH.getClusterArea(rootChildren[z]) == 0) slice.insert(rootChildren[z]);
        }

        while (!slice.empty()) {
                unsigned clToSmash = *(slice.begin());
                slice.erase(slice.begin());

                abkfatal(fillH.getClusterArea(clToSmash) == 0, "non-zero area cluster with zero area parent");

                const vector<unsigned>& childrenCl = fillH.getChildren(clToSmash);

                if (childrenCl.size() == 0)  // leaf terminal
                        leafNodes.push_back(clToSmash);
                else {
                        for (unsigned c = 0; c < childrenCl.size(); c++) slice.insert(childrenCl[c]);
                }
        }

        sizeOfSlice += leafNodes.size();
        numZeroArea = sizeOfSlice;
        if (_params.verb.getForMajStats() > 4) cout << "Num zeroArea leaves/terminals: " << numZeroArea << endl;
        curLevelTargetSize += numZeroArea;

        // this is required because we need for the zero area nodes
        //(which are assumed to be all the terminals) to be in the
        // same order as in the original hgraph.  This is so we
        // can mark them as being fixed if necessary.  As there isn't
        // a mapping UP the hierarchy, it's best if we can assume that
        // the same terminal (which will never be clustered to anything)
        // has the same ID at all levels.  The key observation is that
        // zero-area nodes would be added into the hierarchy in the
        // same order as they appear in the original HGraph, so this
        // works.  If that assumption is violated then the terminal
        // fixed constraints will be permuted.
        std::sort(leafNodes.begin(), leafNodes.end(), std::less<unsigned>());

        // now insert the root's non-terminal children
        for (z = 0; z < rootChildren.size(); z++) {
                if (fillH.getClusterArea(rootChildren[z]) > 0) slice.insert(rootChildren[z]);
        }
        sizeOfSlice += slice.size();  // sizeOfSlice is the TOTAL
        // size of the slice, including
        // terminals and leaves

        // while(sizeOfSlice < sizeOfBottom)	//can't use this, as there may
        // be degree 1 clusters
        while (!slice.empty()) {
                if (_params.verb.getForMajStats() > 3) {
                        cout << "Current slice is of size " << sizeOfSlice << endl;
                        cout << "Creating a level of size " << curLevelTargetSize << endl;
                }

                // create another level.
                while (sizeOfSlice < curLevelTargetSize && !slice.empty()) {
                        // smash the largest cluster in the slice.
                        unsigned clToSmash = *(slice.begin());
                        slice.erase(slice.begin());

                        abkassert(fillH.getClusterArea(clToSmash) > 0, "zero area cluster in main smashing section");

                        const vector<unsigned>& childrenCl = fillH.getChildren(clToSmash);

                        if (childrenCl.size() == 0)
                                leafNodes.push_back(clToSmash);
                        else {
                                for (unsigned c = 0; c < childrenCl.size(); c++) {
                                        ancestorInSlice[childrenCl[c]] = ancestorInSlice[clToSmash];

                                        if (fillH.getChildren(childrenCl[c]).size() == 0)
                                                leafNodes.push_back(childrenCl[c]);
                                        else
                                                slice.insert(childrenCl[c]);
                                }
                                sizeOfSlice += childrenCl.size() - 1;
                        }
                }

                // create an HGraph of the slice, unless this is the bottom.
                // the bottom Level ClHG is just the original hgraph.
                if (sizeOfSlice < sizeOfBottom) {
                        // start with the leaves, and add in the clusters
                        vector<unsigned> lvlNodes(leafNodes);

                        std::set<unsigned, CompareNodesBySize>::iterator sItr;
                        for (sItr = slice.begin(); sItr != slice.end(); sItr++) lvlNodes.push_back(*sItr);

                        HGraphFixed* newLvlHG = new HGraphForHierarchy(lvlNodes, numTerminals, fillH);

                        abkfatal(newLvlHG->getNumNodes() == sizeOfSlice, "sizeOfSlice is wrong");

                        // hgraphs are stored bottom-up. That is, _clHGrs[0] is
                        // the bottom level (the orig. hgraph), and the highest
                        // indexed hgraph is the top.
                        // Because they are created top-down in this flow,
                        // new hgraphs are inserted just above the bottom level.
                        // This isn't really a big deal (i.e. not slow), as it's
                        // just a vector of pointers, so a really quick memcpy
                        // operation on a small (~ < size 10) vector.
                        _clusteredHGraphs.insert(_clusteredHGraphs.begin() + 1, newLvlHG);
                        _ownsHGraph.insert(_ownsHGraph.begin() + 1, true);
                        PartitionIds everywhere;
                        everywhere.setToAll(2);
                        _fixedConst.insert(_fixedConst.begin() + 1, Partitioning(sizeOfSlice, everywhere));

                        abkfatal(_fixedConst.size() == _clusteredHGraphs.size(), "fixed and clustered sizes don't match");

                        if (_clusteredHGraphs.size() == 2)  // just added the top level.
                        {                                   // There is not mapping TO the top level, so
                                // this is an empty mapping
                                _clusterMap.push_back(vector<unsigned>());
                        } else  // added a level that wasn't the top
                        {
                                // these are the first items in the vector
                                // because
                                // the clusterMap to the bottom level (final
                                // index == 0)
                                // hasn't been added yet.  It get's added last.
                                _clusterMap.insert(_clusterMap.begin(), vector<unsigned>(sizeOfSlice, UINT_MAX));
                                vector<unsigned>& newMap = _clusterMap.front();

                                for (unsigned newId = 0; newId < sizeOfSlice; newId++) {
                                        // nodes in the hgraph will be indexed
                                        // consecutively
                                        // from 0, in the same order as in the
                                        // vector
                                        // of id's given to the subHGraph ctor.
                                        newMap[newId] = ancestorInSlice[lvlNodes[newId]];
                                }
                        }
                        // reset the values in ancestorInSlice, since we saved
                        // a new lvl hgraph
                        for (unsigned newId = 0; newId < sizeOfSlice; newId++) ancestorInSlice[lvlNodes[newId]] = newId;
                }

                unsigned numNonTerms = sizeOfSlice - numZeroArea;
                unsigned nextLevelTargetSize = static_cast<unsigned>(ceil(numNonTerms * _params.levelGrowth + numZeroArea));
                unsigned levelBeyondThat = static_cast<unsigned>(ceil(numNonTerms * _params.levelGrowth * _params.levelGrowth + numZeroArea));
                unsigned maxRounding = (levelBeyondThat - nextLevelTargetSize) / 4;

                if (nextLevelTargetSize + maxRounding >= sizeOfBottom)  // maybe round up
                        nextLevelTargetSize = sizeOfBottom + 1;
                // the +1 takes care of degree one clusters

                curLevelTargetSize = nextLevelTargetSize;
        }

        abkfatal(sizeOfSlice == leafNodes.size(), "leafNodes.size does not match sizeOfSlice");

        // add the mapping TO the bottom level
        // note that nodeIds between the hierarchy and the
        // bottom level hgraph do not match!! They are name-
        // associated only.
        _clusterMap.insert(_clusterMap.begin(), vector<unsigned>(sizeOfSlice, UINT_MAX));
        vector<unsigned>& newMap = _clusterMap.front();

        for (unsigned newId = 0; newId < sizeOfSlice; newId++) {
                const char* hierNodeName = fillH.getNodeName(leafNodes[newId]);

                unsigned bottomHGNodeId = _clusteredHGraphs[0]->getNodeByName(hierNodeName).getIndex();
                newMap[bottomHGNodeId] = ancestorInSlice[leafNodes[newId]];
        }

        PartitionIds everywhere;
        everywhere.setToAll(32);
        _topLevelPart = Partitioning(sizeOfBottom, everywhere);
}
