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

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <ClusteredHGraph/clustHGCluster.h>
#include <ClusteredHGraph/clustHGNet.h>
#include <ClusteredHGraph/hgraphFromSlice.h>

using uofm::string;
using uofm::vector;

double HGraphFromSlice::finalizeTime = 0;

HGraphFromSlice::HGraphFromSlice(const vector<ClHG_Cluster*>& terminals, unsigned actualNumTerms, const vector<ClHG_Cluster*>& nonTerms, const vector<ClHG_ClNet>& edges, vector<unsigned>& clMap, Partitioning& fixedConst) : HGraphFixed(terminals.size() + nonTerms.size(), 1) {
        _param.makeAllSrcSnk = true;
        _edges.reserve(edges.size());

        //    _nodeNames = vector<string>(getNumNodes(),string());

        abkfatal(clMap.size() >= terminals.size() + nonTerms.size(), "clMap is way too small");

        _numTerminals = actualNumTerms;
        unsigned i = 0;
        vector<ClHG_Cluster*>::const_iterator cl;
        for (cl = terminals.begin(); cl != terminals.end(); cl++, i++) {
                setWeight(i, (*cl)->getArea());
                _nodes[i]->_edges.reserve((*cl)->getDegree());
#ifdef SIGNAL_DIRECTIONS
                _nodes[i]->_snksBegin = _nodes[i]->_srcSnksBegin = _nodes[i]->_edges.begin();
#endif
                clMap[i] = i;  // terminals get mapped to the same place at each level
                // because they aren't clustered
                fixedConst[i] = (*cl)->getFixed();

                /*	char* tmpName = new char[9];
                        sprintf(tmpName, "p%d", i);
                        _nodeNames[i] = tmpName;
                        _nodeNamesMap[tmpName] = i;
                */
        }

        for (cl = nonTerms.begin(); cl != nonTerms.end(); cl++, i++) {
                setWeight(i, (*cl)->getArea());
                _nodes[i]->_edges.reserve((*cl)->getDegree());
                vector<unsigned>& hgraphIds = (*cl)->hgraphIds;
                for (unsigned d = 0; d < hgraphIds.size(); d++) clMap[hgraphIds[d]] = i;

                hgraphIds.erase(hgraphIds.begin(), hgraphIds.end());
                hgraphIds.push_back(i);
                fixedConst[i] = (*cl)->getFixed();
                /*
                        char* tmpName = new char[9];
                        sprintf(tmpName, "a%d", i);
                        _nodeNames[i] = tmpName;
                        _nodeNamesMap[tmpName] = i;
                */
        }

        // build the HG's collection of edges.
        // skip any nets with a weight of 0

        // Timer finalTime;

        vector<ClHG_ClNet>::const_iterator n;
        for (n = edges.begin(); n != edges.end(); n++) {
                if (n->getNumClusters() <= 1 || n->getWeight() == 0) continue;

                HGFEdge* newEdge = fastAddEdge(n->getNumClusters(), n->getWeight());

                ClHG_NetClConstItr c;
                for (c = n->constClustersBegin(); c != n->constClustersEnd(); c++) {
                        unsigned hgNodeIdx = (*c)->hgraphIds[0];
                        fastAddSrcSnk(&getNodeByIdx(hgNodeIdx), newEdge);
                }
        }
        //    _netNames = vector<string>(_edges.size());
        //    finalize();
        clearNameMaps();
        clearNames();
}
