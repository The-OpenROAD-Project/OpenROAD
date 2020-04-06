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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "fillHier.h"
#include "hgForHierarchy.h"
#include "ClusteredHGraph/clustHGraph.h"
#include <sstream>

using uofm::stringstream;
using std::cout;
using std::endl;
using uofm::vector;

// adds nodes in the leafHGraph which are not in the hierarchy
// as (leaf)nodes of root.
void FillableHierarchy::putAllNodesInHierarchy() {
        for (unsigned n = 0; n < _leafHGraph.getNumNodes(); n++) {
                const char* nodeName = _leafHGraph.getNodeNameByIndex(n).c_str();
                if (_namesToId.find(nodeName) == _namesToId.end())
                    // not in hierarchy
                {
                        char* localNodeName = new char[strlen(nodeName) + 2];
                        strcpy(localNodeName, nodeName);

                        _namesToId[localNodeName] = _names.size();
                        _parents.push_back(_root);
                        _children.push_back(vector<unsigned>());
                        _children[_root].push_back(_names.size());
                        _clusterWeights.push_back(_leafHGraph.getWeight(n));
                        _adjacentEdges.push_back(vector<unsigned>());
                        _names.push_back(localNodeName);
                }
        }
}

// computes each clusters areas and makes all area-0 nodes
// be children of root.
void FillableHierarchy::setAreas(unsigned nodeId) {
        if (_children[nodeId].size() == 0) {
                _clusterWeights[nodeId] = _leafHGraph.getWeight(_leafHGraph.getNodeByName(_names[nodeId]).getIndex());
        } else {
                _clusterWeights[nodeId] = 0;
                // this is an int because we change # children and in doing so
                // subtract from c at various points
                for (int c = 0; c < int(_children[nodeId].size()); c++) {
                        unsigned child = _children[nodeId][c];
                        setAreas(child);

                        if (_clusterWeights[child] == 0 && nodeId != _root) {  // make it a child of root
                                _children[nodeId][c] = _children[nodeId].back();
                                _children[nodeId].pop_back();
                                c--;  // so we'll get this on the next time
                                      // around

                                _parents[child] = _root;
                                _children[_root].push_back(child);
                        } else
                                _clusterWeights[nodeId] += _clusterWeights[child];
                }

                if (_children[nodeId].size() == 0)  // all were area 0...
                        deleteNode(nodeId);
        }
}

void FillableHierarchy::fillWithHEM() {
        // recursive process:
        abkfatal(_root != UINT_MAX, "root was not set");

        unsigned expectedNodes = _names.size() * 3;
        _names.reserve(expectedNodes);
        _clusterWeights.reserve(expectedNodes);
        _children.reserve(expectedNodes);
        _parents.reserve(expectedNodes);
        _adjacentEdges.reserve(expectedNodes);

        itHGFEdgeGlobal eIt;
        for (eIt = _leafHGraph.edgesBegin(); eIt != _leafHGraph.edgesEnd(); eIt++) {
                _edgeWeights[(*eIt)->getIndex()] = (*eIt)->getWeight();
                _clusteredEdgeDegree[(*eIt)->getIndex()] = (*eIt)->getDegree();
        }

        fillNodeWithHEM(_root);
}

// when this function returns, nodeId will
// be the root of a filled sub-tree, and
// adjacentEdges[nodeId] will be populated

// note:  leaf nodes in the hierarchy are name-associated with
// nodes in the hgraph provided only.  their id's will be
// different.  An edge in the hierarchy will have the same ID
// as the coresponding edge in the provided hgraph.
void FillableHierarchy::fillNodeWithHEM(unsigned nodeId) {
        if (_children[nodeId].size() == 0)  // leaf
        {
                const HGFNode& lNode = _leafHGraph.getNodeByName(_names[nodeId]);

                itHGFEdgeLocal e;
                for (e = lNode.edgesBegin(); e != lNode.edgesEnd(); e++) {
                        if ((*e)->getDegree() < 2) continue;
                        _adjacentEdges[nodeId].push_back((*e)->getIndex());
                }
                return;
        }
        // populate the children node's adjacentEdge vectors, and do any
        // filling in below them that is necessary
        for (unsigned c = 0; c < _children[nodeId].size(); c++) fillNodeWithHEM(_children[nodeId][c]);

        // wether we fill in below the cluster or not, its incident
        // nets will be the same
        induceEdgesFromChildren(nodeId);

        // now that all children have been 'filled' and have their
        //'adjacentEdges' vectors populated, fill the hierarchy below
        // this node.  maxFanout == 0 means don't cluster
        if (_children[nodeId].size() > _params.maxFanout && _params.maxFanout) {
                if (_params.verb.getForMajStats() > 5) cout << "Doing HEM filling under node " << _names[nodeId] << endl;

                Timer fillTimer;

                // put terminals first
                vector<unsigned>& nodeChildren = _children[nodeId];
                unsigned firstNonTerm = 0;
                for (unsigned nd = 0; nd < nodeChildren.size(); nd++) {
                        if (_clusterWeights[nodeChildren[nd]] == 0) {
                                unsigned ndIdToSwap = nodeChildren[firstNonTerm];
                                nodeChildren[firstNonTerm] = nodeChildren[nd];
                                nodeChildren[nd] = ndIdToSwap;
                                firstNonTerm++;
                        }
                }

                // construct HGraph and call ClusteredHGraph here.
                HGraphForHierarchy clusterHG(_children[nodeId], firstNonTerm, *this);

                ClustHGraphParameters clParams;
                clParams.verb = _params.verb;
                clParams.sizeOfTop = _params.maxFanout;

                PartitionIds everywhere;
                PartitionIds nowhere;
                everywhere.setToAll(32);
                Partitioning sillyPart(_leafHGraph.getNumNodes(), everywhere);
                std::fill(sillyPart.begin(), sillyPart.begin() + firstNonTerm, nowhere);

                if (clusterHG.getNumTerminals() > 0) cout << "Node has " << clusterHG.getNumTerminals() << " 0-area children" << endl;

                Timer clHGTimer;
                ClusteredHGraph clHG(clusterHG, clParams, sillyPart);
                clHGTimer.stop();
                if (_params.verb.getForMajStats() > 5) cout << "ClHGraph took " << clHGTimer.getUserTime() << " seconds" << endl;

                // put the interior nodes in the clusteredHGraph into the
                // hierarchy as 'added nodes'

                insertClusteringIntoHierarchy(clusterHG, clHG, nodeId);

                fillTimer.stop();
                if (_params.verb.getForMajStats() > 5) cout << "Filling took " << fillTimer.getUserTime() << " seconds" << endl;
        }
}

// the subHG should be the same as the bottom level of the
// clustered HGraph.  RootId is the id of the node we are
// filling below.  So, the subHG is the HGraph over its
// children.
void FillableHierarchy::insertClusteringIntoHierarchy(const SubHGraph& subHG, const ClusteredHGraph& clHG, unsigned rootId) {
        Timer insertionTimer;

        // 1) insert the bottom level. These will all be trivial nodes
        //	(only 1 child).
        // 2) do a bottom-up insertion of the clustering hierarchy into
        // 	this hierarchy.

        vector<unsigned> origChildren = _children[rootId];

        // this is the id of the first node to be added.
        unsigned newNodesOffset = _names.size();

        // insert the bottom level
        {
                const HGraphFixed& bottomHG = clHG.getHGraph(0);
                abkassert(bottomHG.getNumNodes() == subHG.getNumNodes(), "bottom HG and subHG are not the same size");
                abkassert(origChildren.size() == subHG.getNumNodes(), "subHG not the same size as number of children");

                unsigned n;
                for (n = 0; n < subHG.getNumNodes(); n++) {
                        unsigned newClusterId = newNodesOffset + n;
                        unsigned origClusterId = subHG.newNode2OrigIdx(n);
                        abkassert(_parents[origClusterId] == rootId, "origCluster is not a child of root.");

                        stringstream nNameTmp;
                        nNameTmp << "HEMNode" << rootId << "Sub" << _names.size() - newNodesOffset;
                        char* nName = new char[nNameTmp.str().size() + 1];
                        strcpy(nName, nNameTmp.str().c_str());
                        _names.push_back(nName);
                        _namesToId[nName] = newClusterId;

                        _clusterWeights.push_back(_clusterWeights[origClusterId]);

                        abkassert(_clusterWeights.back() == subHG.getWeight(n), "cluster weights do not match ");

                        _parents.push_back(UINT_MAX);
                        _children.push_back(vector<unsigned>(1, origClusterId));
                        _adjacentEdges.push_back(_adjacentEdges[origClusterId]);

                        _parents[origClusterId] = newClusterId;
                }
// check that all the children had their parent pointers reset
#ifdef ABKDEBUG
                for (n = 0; n < origChildren.size(); n++) abkfatal(_parents[origChildren[n]] != rootId, "child did not get its parent reset");
#endif
        }

        if (_params.verb.getForActions() > 7) cout << "Inserted bottom level" << endl;

        // insert the remaining levels
        {
                unsigned prevLvlSize = origChildren.size();

                for (unsigned lvl = 1; lvl < clHG.getNumLevels(); lvl++) {
                        const HGraphFixed& lvlHG = clHG.getHGraph(lvl);
                        const vector<unsigned>& lvlMap = clHG.getMapping(lvl - 1);
                        unsigned lvlOffset = _names.size();
                        unsigned prevLvlOffset = lvlOffset - prevLvlSize;

                        abkfatal3(lvlMap.size() == prevLvlSize, lvlMap.size(), " - map is not as big as prevLvlSize - ", prevLvlSize);

                        // add in the new nodes
                        unsigned n;
                        for (n = 0; n < lvlHG.getNumNodes(); n++) {
                                abkassert(_names.size() == lvlOffset + n, "names size does not match lvlOffset+n");

                                unsigned newClusterId = _names.size();

                                stringstream nNameTmp;
                                nNameTmp << "HEMNode" << rootId << "Sub" << _names.size() - newNodesOffset;
                                char* nName = new char[nNameTmp.str().size() + 1];
                                strcpy(nName, nNameTmp.str().c_str());
                                _names.push_back(nName);
                                _namesToId[nName] = newClusterId;
                                _clusterWeights.push_back(lvlHG.getWeight(n));

                                // compute adjacent edges here.
                                _adjacentEdges.push_back(vector<unsigned>());

                                _parents.push_back(GENH_DELETED_NODE);
                                _children.push_back(vector<unsigned>());
                        }

                        // connect the new nodes to the prev. level
                        for (n = 0; n < prevLvlSize; n++) {
                                unsigned child = prevLvlOffset + n;
                                unsigned parent = lvlMap[n] + lvlOffset;

                                abkassert(_parents[child] == GENH_DELETED_NODE, "child who's parent is already set");
                                _parents[child] = parent;
                                _children[parent].push_back(child);
                        }

                        // created the adjacent edges
                        for (n = lvlOffset; n < _names.size(); n++) induceEdgesFromChildren(n);

                        if (lvl == clHG.getNumLevels() - 1)  // top level
                        {                                    // make the top-level nodes children of rootId

                                double rootChildrenWt = 0;
                                _children[rootId].clear();
                                for (n = lvlOffset; n < _names.size(); n++) {
                                        rootChildrenWt += _clusterWeights[n];
                                        _children[rootId].push_back(n);
                                        _parents[n] = rootId;
                                }
                                abkfatal(rootChildrenWt == _clusterWeights[rootId], "didn't match with root weight");
                        }
                        prevLvlSize = lvlHG.getNumNodes();
                }
        }
        insertionTimer.stop();
        if (_params.verb.getForMajStats() > 5) cout << "Insertion back into the hierarchy took " << insertionTimer.getUserTime() << " seconds" << endl;
}

void FillableHierarchy::induceEdgesFromChildren(unsigned nodeId) {
        if (_children[nodeId].size() == 1) {
                _adjacentEdges[nodeId] = _adjacentEdges[_children[nodeId][0]];
                return;
        }

        _adjacentEdges[nodeId].clear();

        _edgeIsIncident.clear();

        for (unsigned c = 0; c < _children[nodeId].size(); c++) {
                unsigned child = _children[nodeId][c];

                for (unsigned e = 0; e < _adjacentEdges[child].size(); e++) {
                        unsigned edgeId = _adjacentEdges[child][e];
                        _clusteredEdgeDegree[edgeId]--;
                        _edgeIsIncident.setBit(edgeId);
                }
        }
        const vector<unsigned>& setBits = _edgeIsIncident.getIndicesOfSetBits();
        vector<unsigned>& edges = _adjacentEdges[nodeId];
        edges.reserve(setBits.size());
        for (unsigned setB = 0; setB < setBits.size(); setB++) {
                unsigned edgeId = setBits[setB];
                if (_clusteredEdgeDegree[edgeId] == 0)  // contained edge..skip it
                        continue;
                edges.push_back(edgeId);
                _clusteredEdgeDegree[edgeId]++;
        }
}
