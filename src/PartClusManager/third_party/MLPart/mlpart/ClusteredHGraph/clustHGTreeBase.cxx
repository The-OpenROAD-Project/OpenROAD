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

#include <ABKCommon/abkassert.h>
#include <ABKCommon/uofm_alloc.h>
#include <ClusteredHGraph/clustHGTreeBase.h>
#include <ClusteredHGraph/hgraphFromSlice.h>
#include <ClusteredHGraph/clustHGNet.h>

using std::cout;
using std::endl;
using std::setw;
using std::max;
using uofm::vector;

unsigned ClHG_ClusterTreeBase::deg2Removed = 0;
unsigned ClHG_ClusterTreeBase::deg3Removed = 0;

RandomRawUnsigned ClHG_ClusterTreeBase::_rng;

ClHG_ClusterTreeBase::ClHG_ClusterTreeBase(const HGraphFixed& hgraph, const Parameters& params, const Partitioning* fixed, const Partitioning* origPart) : BaseClusteredHGraph(hgraph, params, fixed, origPart), _nextId(0), _numLeafClusters(hgraph.getNumNodes()), _numLeafTerminals(hgraph.getNumTerminals()), _totalArea(0), _minArea(DBL_MAX), _numContained(hgraph.getNumEdges(), 0) {
        _nets.reserve(hgraph.getNumEdges());
        _toCluster.reserve(hgraph.getNumNodes());

        // build all the ClNets
        for (itHGFEdgeGlobal e = hgraph.edgesBegin(); e != hgraph.edgesEnd(); e++) _nets.push_back(ClHG_ClNet(**e));

        // build all the leaf clusters
        PartitionIds allParts;
        allParts.setToAll(32);

        const Partitioning& leafConst = _fixedConst[0];

        if (fixed == NULL || _params.dontClusterTerms)
            // no fixed constraints were passed, so we can't
            // know where the terminals are allowed to go, or
            // clustering to terminals has been dis-allowed by
            // the parameters..so, just leave them alone.
            // To do this, we'll mark them as unclusterable.
        {
                // cout<<"skipping clustering to terminals"<<endl;
                _terminals.reserve(hgraph.getNumTerminals());

                for (unsigned idx = 0; idx < hgraph.getNumTerminals(); idx++) {
                        const HGFNode& node = hgraph.getNodeByIdx(idx);
                        double weight = hgraph.getWeight(node.getIndex());
                        PartitionIds nodesPart = allParts;
                        if (_origPart != NULL) nodesPart = (*_origPart)[idx];

                        ClHG_Cluster& newCl = *addClust(hgraph, node, leafConst[idx], nodesPart);

                        _toCluster.push_back(&newCl);

                        _totalArea += weight;
                        _minArea = (weight > 0 && weight < _minArea) ? weight : _minArea;
                        newCl._terminal = true;
                        newCl._clusterable = false;

                        newCl.induceNets();
                }
        } else  // allow clustering to terminals
        {
                // cout<<"allowing clustering to terminals"<<endl;

                for (unsigned idx = 0; idx < hgraph.getNumTerminals(); idx++) {
                        const HGFNode& node = hgraph.getNodeByIdx(idx);
                        double weight = hgraph.getWeight(node.getIndex());
                        PartitionIds nodesPart = allParts;
                        if (_origPart != NULL) nodesPart = (*_origPart)[idx];

                        ClHG_Cluster& newCl = *addClust(hgraph, node, leafConst[idx], nodesPart);

                        _toCluster.push_back(&newCl);

                        _totalArea += weight;
                        _minArea = (weight > 0 && weight < _minArea) ? weight : _minArea;
                        newCl._terminal = true;
                        newCl._clusterable = true;

                        newCl.induceNets();
                }
        }

        for (unsigned idx = hgraph.getNumTerminals(); idx < hgraph.getNumNodes(); idx++) {
                const HGFNode& node = hgraph.getNodeByIdx(idx);
                double weight = hgraph.getWeight(node.getIndex());
                PartitionIds nodesPart = allParts;
                if (_origPart != NULL) nodesPart = (*_origPart)[idx];

                ClHG_Cluster& newCl = *addClust(hgraph, node, leafConst[idx], nodesPart);

                _toCluster.push_back(&newCl);

                _totalArea += weight;
                _minArea = (weight > 0 && weight < _minArea) ? weight : _minArea;

                newCl.induceNets();
        }

        _numClusterableTerminals = hgraph.getNumTerminals() - _terminals.size();

        if (_params.verb.getForMajStats() > 2) {
                cout << "Total Area: " << _totalArea << endl;
                cout << "Min Non-Zero Area: " << _minArea << endl;
                cout << "Clusterable Nodes: " << _toCluster.size() << endl;
                cout << " of those, " << _numClusterableTerminals << " are terminals" << endl;
                cout << "Unclusterable Terminals: " << _terminals.size() << endl;
        }
}

ClHG_ClusterTreeBase::~ClHG_ClusterTreeBase() {
        cleanupOldNodes(_toCluster);
        cleanupOldNodes(_terminals);
}

void ClHG_ClusterTreeBase::addHGraph() {
        // make a snapshot hgraph of the clusters and nets
        // add it as the next HGraph in the _clusteredHGraphs
        // note that this one is owned by the ClusteredHGraph, as
        // it will have to delete it

        unsigned prevNumNodes = _clusteredHGraphs.back()->getNumNodes();

        _clusterMap.push_back(vector<unsigned>(prevNumNodes, UINT_MAX));
        _fixedConst.push_back(Partitioning(_terminals.size() + _toCluster.size()));

        _clusteredHGraphs.push_back(new HGraphFromSlice(_terminals, _terminals.size() + _numClusterableTerminals, _toCluster, _nets, _clusterMap.back(), _fixedConst.back()));

        _ownsHGraph.push_back(true);

        // cout<<"New HGraph: "<<endl;
        // cout<<"mapping "<<_clusterMap.back()<<endl;
        // cout<<"FixedConst"<<_fixedConst.back()<<endl;
}

void ClHG_ClusterTreeBase::addTopLevelPartitioning() {
        if (_terminals.size() + _toCluster.size() != _clusteredHGraphs.back()->getNumNodes()) {
                cout << "Size mis-match in addTopLevelPartitioning()" << endl;
                cout << " Previous HGraph had " << _clusteredHGraphs.back()->getNumNodes() << "nodes" << endl;
                cout << " There are " << _terminals.size() << " terminals "
                     << "and " << _toCluster.size() << " non-terminals" << endl;

                abkfatal(0, "size mis-match in addTopLevelPartitioning()");
        }

        _topLevelPart = Partitioning(_terminals.size() + _toCluster.size());

        unsigned t;
        for (t = 0; t < _terminals.size(); t++) _topLevelPart[t] = _terminals[t]->_part;

        for (unsigned m = 0; m < _toCluster.size(); m++) _topLevelPart[t + m] = _toCluster[m]->_part;
}

void ClHG_ClusterTreeBase::removeDuplicateEdges() {  // remove duplicate edges,
                                                     // and weight the remaining
                                                     // one

        // for now..remove only 2-pin edges
        vector<ClHG_ClNet>::iterator n;
        for (n = _nets.begin(); n != _nets.end(); n++) {
                if (n->getWeight() == 0) continue;

                if (n->getDegree() == 2)  // if we remove, we always remove degree 2 edges
                {
                        ClHG_Cluster* cl1 = n->_clusters.front();
                        ClHG_Cluster* cl2 = n->_clusters.back();
                        unsigned nId = n->getIndex();

                        ClHG_CutNetItr cutN;
                        for (cutN = cl1->cutNetsBegin(); cutN != cl1->cutNetsEnd(); cutN++) {
                                ClHG_ClNet& adjN = *(cutN->net);

                                if (adjN.getWeight() == 0 || adjN.getIndex() <= nId || adjN.getDegree() != 2) continue;

                                vector<ClHG_Cluster*>& adj = adjN._clusters;
                                if ((adj.front() == cl1 && adj.back() == cl2) || (adj.front() == cl2 && adj.back() == cl1)) {
                                        n->incWeight(adjN.getWeight());
                                        adjN.setWeight(0);
                                        deg2Removed++;
                                }
                        }
                } else if (_params.removeDup >= 3 && n->getDegree() == 3) {
                        ClHG_ClNet& net1 = *n;
                        unsigned nId = net1.getIndex();
                        ClHG_Cluster* cl_1 = *(net1.clustersBegin());
                        ClHG_Cluster* cl_2 = *(net1.clustersBegin() + 1);
                        ClHG_Cluster* cl_3 = *(net1.clustersBegin() + 2);

                        ClHG_CutNetItr cutN;
                        for (cutN = cl_1->cutNetsBegin(); cutN != cl_1->cutNetsEnd(); cutN++) {
                                ClHG_ClNet& net2 = *(cutN->net);
                                if (net2.getWeight() == 0 || net2.getIndex() <= nId || net2.getDegree() != 3) continue;

                                ClHG_NetClItr net2Itr;
                                bool checkFurther = true;
                                for (net2Itr = net2.clustersBegin(); net2Itr != net2.clustersEnd() && checkFurther; net2Itr++) {
                                        if (*net2Itr != cl_1 && *net2Itr != cl_2 && *net2Itr != cl_3) checkFurther = false;
                                }
                                if (checkFurther)  // net1 and net2 are
                                                   // duplicate edges
                                {
                                        net1.incWeight(net2.getWeight());
                                        net2.setWeight(0);
                                        deg3Removed++;
                                }
                        }
                }
        }
        // cout<<"So Far  2:"<<deg2Removed<<"  3:"<<deg3Removed<<endl;
}

void ClHG_ClusterTreeBase::printStatsHeader() {
        if (_params.verb.getForMajStats() > 5)
                cout << " #Clusters   #DiffHEdges  AveClDeg  AveEdgDeg  "
                        "TotalHEdges Eval"
                     << " MaxClArea" << endl;
        else if (_params.verb.getForMajStats() > 0)
                cout << " #Clusters" << endl;
}

void ClHG_ClusterTreeBase::printStats() {
        if (_params.verb.getForMajStats() > 5) {

                unsigned totalClDegree = 0;
                unsigned numDiffEdges = 0;  // only non-contained edges of wt >= 1
                unsigned numCutEdges = 0;   // only non-contained edges.
                unsigned totalEdgeDegree = 0;
                double largestClArea = 0;
                double totalClArea = 0;
                double evalCost = 0;

                unsigned ctr;
                for (ctr = 0; ctr < _toCluster.size(); ctr++) {
                        ClHG_CutNetItr cutN;
                        for (cutN = _toCluster[ctr]->cutNetsBegin(); cutN != _toCluster[ctr]->cutNetsEnd(); cutN++) {
                                if (cutN->net->getWeight() != 0) totalClDegree++;
                        }
                        largestClArea = max(largestClArea, _toCluster[ctr]->getArea());
                        totalClArea += _toCluster[ctr]->getArea();
                }
                for (ctr = 0; ctr < _terminals.size(); ctr++) {
                        ClHG_CutNetItr cutN;
                        for (cutN = _terminals[ctr]->cutNetsBegin(); cutN != _terminals[ctr]->cutNetsEnd(); cutN++) {
                                if (cutN->net->getWeight() != 0) totalClDegree++;
                        }
                        largestClArea = max(largestClArea, _terminals[ctr]->getArea());
                        totalClArea += _terminals[ctr]->getArea();
                }

                for (ctr = 0; ctr < _nets.size(); ctr++) {
                        if (_nets[ctr].getDegree() > 1) {
                                numCutEdges += static_cast<unsigned>(floor(_nets[ctr].getWeight()));
                                if (_nets[ctr].getWeight() > 0.0) {
                                        totalEdgeDegree += _nets[ctr].getDegree();
                                        numDiffEdges++;
                                }
                        }
                }

                double maxClPerct = (largestClArea / totalClArea) * 1000.0;
                maxClPerct = ((double)(ceil(maxClPerct)) / 10.0);

                evalCost = evalCurrentClustering();

                cout << " " << setw(9) << _toCluster.size() + _terminals.size() << " " << setw(13) << numDiffEdges << " " << setw(9) << (double)totalClDegree / (double)(_toCluster.size() + _terminals.size()) << " " << setw(10) << (double)totalEdgeDegree / (double)numDiffEdges << " " << setw(12) << numCutEdges << " " << setw(10) << maxClPerct << " " << setw(10) << evalCost << endl;
        } else if (_params.verb.getForMajStats() > 0)
                cout << " " << _toCluster.size() << endl;
}

void ClHG_ClusterTreeBase::clusterDegree1Nodes() {
        // set all tags to 0
        vector<ClHG_Cluster*>::iterator cl;
        for (cl = _toCluster.begin(); cl != _toCluster.end(); cl++) (*cl)->setTag(0);

        double maxClArea = _totalArea * 0.01;  // 1%
        bool mergedCl = true;

        while (mergedCl)  // cluster up all degree 1 nodes
        {
                mergedCl = false;
                for (unsigned i = 0; i < _toCluster.size(); i++) {
                        if (_toCluster[i]->getDegree() != 1 || _toCluster[i]->getTag() != 0) continue;

                        ClHG_Cluster* deg1Cl = _toCluster[i];
                        ClHG_Cluster* adjCl = NULL;

                        ClHG_ClNet& net = *(deg1Cl->cutNetsBegin()->net);
                        // cluster to the smallest other thing on this net that
                        // meets
                        // the area limits

                        for (cl = net.clustersBegin(); cl != net.clustersEnd(); cl++) {
                                PartitionIds unionPIds = deg1Cl->getAllowableParts();
                                unionPIds &= (*cl)->getAllowableParts();

                                if (*cl == deg1Cl || (*cl)->getTag() != 0 || unionPIds.isEmpty()) continue;

                                if ((*cl)->getArea() + deg1Cl->getArea() <= maxClArea || deg1Cl->getArea() == 0 || (*cl)->getArea() == 0) {  // match  -there may be more than
                                                                                                                                             // 1, though..we want
                                        //	the smallest
                                        if (adjCl == NULL || (*cl)->getArea() < adjCl->getArea()) adjCl = *cl;
                                }
                        }

                        if (adjCl == NULL)  // no matches
                                continue;
                        else {
                                deg1Cl->removeIndNets();
                                adjCl->removeIndNets();

                                if (deg1Cl->getIndex() < adjCl->getIndex()) {
                                        deg1Cl->mergeWith(*adjCl, *this);
                                        adjCl->setTag(1);  // to be deleted
                                        deg1Cl->induceNets();
                                        deg1Cl->setTag(0);  // clusterable
                                } else {
                                        adjCl->mergeWith(*deg1Cl, *this);
                                        deg1Cl->setTag(1);  // to be deleted
                                        adjCl->induceNets();
                                        adjCl->setTag(0);  // clusterable
                                }
                                mergedCl = true;
                        }
                }
        }
        // remove each cl we deleted from _toCluster
        _toCluster.erase(std::remove_if(_toCluster.begin(), _toCluster.end(), RemoveClusteredNodes()), _toCluster.end());
}

void ClHG_ClusterTreeBase::clusterDegree2Nodes() {
        // set all tags to 0
        vector<ClHG_Cluster*>::iterator cl;
        for (cl = _toCluster.begin(); cl != _toCluster.end(); cl++) (*cl)->setTag(0);

        double maxClArea = _totalArea * 0.01;  // 1%
        bool mergedCl = true;

        while (mergedCl)  // cluster all degree 2 nodes
        {
                mergedCl = false;

                for (unsigned i = 0; i < _toCluster.size(); i++) {
                        if (_toCluster[i]->getDegree() != 2 || _toCluster[i]->getTag() != 0) continue;

                        ClHG_Cluster* deg2Cl = _toCluster[i];
                        ClHG_Cluster* adjCl = NULL;

                        ClHG_ClNet& net0 = *(deg2Cl->cutNetsBegin()->net);
                        ClHG_ClNet& net1 = *((deg2Cl->cutNetsBegin() + 1)->net);

                        // cluster to the smallest other thing on these nets
                        // that meets
                        // the area limits

                        if (net0.getDegree() == 2)
                                for (cl = net0.clustersBegin(); cl != net0.clustersEnd(); cl++) {
                                        if (*cl == deg2Cl || (*cl)->getTag() != 0) continue;
                                        if ((*cl)->getArea() + deg2Cl->getArea() <= maxClArea || deg2Cl->getArea() == 0 || (*cl)->getArea() == 0) {  // possible match
                                                if (adjCl == NULL || (*cl)->getArea() < adjCl->getArea()) adjCl = *cl;
                                        }
                                }

                        if (net1.getDegree() == 2)
                                for (cl = net1.clustersBegin(); cl != net1.clustersEnd(); cl++) {
                                        if (*cl == deg2Cl || (*cl)->getTag() != 0) continue;
                                        if ((*cl)->getArea() + deg2Cl->getArea() <= maxClArea || deg2Cl->getArea() == 0 || (*cl)->getArea() == 0) {  // possible match
                                                if (adjCl == NULL || (*cl)->getArea() < adjCl->getArea()) adjCl = *cl;
                                        }
                                }

                        if (adjCl == NULL)  // no matches
                                continue;
                        else {
                                deg2Cl->removeIndNets();
                                adjCl->removeIndNets();

                                if (deg2Cl->getIndex() < adjCl->getIndex()) {
                                        deg2Cl->mergeWith(*adjCl, *this);
                                        adjCl->setTag(1);  // to be deleted
                                        deg2Cl->induceNets();
                                        deg2Cl->setTag(0);
                                } else {
                                        adjCl->mergeWith(*deg2Cl, *this);
                                        deg2Cl->setTag(1);  // to be deleted
                                        adjCl->induceNets();
                                        adjCl->setTag(0);
                                }
                                mergedCl = true;
                        }
                }
        }

        // remove each cl we deleted from _toCluster
        _toCluster.erase(std::remove_if(_toCluster.begin(), _toCluster.end(), RemoveClusteredNodes()), _toCluster.end());
}

double ClHG_ClusterTreeBase::evalCurrentClustering() const {
        double cost = 0;
        for (unsigned n = 0; n < _nets.size(); n++) {
                const ClHG_ClNet& net = _nets[n];
                if (net.getDegree() > 1) cost += ((double)net.getDegree() - 1.0) / ((double)net.getMaxDegree() - 1.0);
        }

        return cost;
}

void ClHG_ClusterTreeBase::spinRandomNumber(unsigned numToWaste)
    /*
     * <aaronnn> spin the random number generator a few times
     * to 'inject randomness'
     */
{
        uofm::string warningMessage =
            "DP says: "
            "\"Since every random bit is just as random as every other random "
            "bit,\n"
            "the maximum number of bits to spin should be no more than the "
            "total number of\n"
            "different starts desired. If you spin more than 100 times, then "
            "you should be\n"
            "running more than 100 starts.\"";

        if (numToWaste > 100) {
                abkwarn(0, warningMessage.c_str());
        }

        unsigned unused;
        for (unsigned i = 0; i < numToWaste; i++) unused = _rng;
}
