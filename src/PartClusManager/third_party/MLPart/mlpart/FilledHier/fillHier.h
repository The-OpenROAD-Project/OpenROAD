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

#ifndef __FILLABLE_HIERARCHY_H__
#define __FILLABLE_HIERARCHY_H__

#include "GenHier/genHier.h"
#include "HGraph/hgFixed.h"
#include "Ctainers/bitBoard.h"

class SubHGraph;
class ClusteredHGraph;

struct FillableHierarchyParameters {
        unsigned maxFanout;
        // insert clusters below
        // nodes w/ fanout > maxFanout.
        // 0 means don't cluster.
        // clusters will be insterted
        // till the fanout is < maxFanout.
        //(it may not end up == maxFanout)
        Verbosity verb;

        FillableHierarchyParameters(unsigned fanout = 10, const Verbosity& verbosity = Verbosity("1_1_1")) : maxFanout(fanout), verb(verbosity) {}
        FillableHierarchyParameters(int argc, const char* argv[]);
};

class FillableHierarchy : public GenericHierarchy {
        const HGraphFixed& _leafHGraph;
        const FillableHierarchyParameters& _params;

        // used to induce hgraphs for filling w/ HEM
        uofm::vector<uofm::vector<unsigned> > _adjacentEdges;
        uofm::vector<double> _clusterWeights;
        uofm::vector<double> _edgeWeights;

        uofm::vector<unsigned> _clusteredEdgeDegree;

        // used as a temp. data structure when inducing clustered edges
        BitBoard _edgeIsIncident;

        // computes clusters areas and makes all area-0 nodes
        // be children of root.  Normally called in ctor and passed _root.
        void setAreas(unsigned nodeId);

        void fillWithHEM();
        void fillNodeWithHEM(unsigned nodeId);

        void insertClusteringIntoHierarchy(const SubHGraph& subHG, const ClusteredHGraph& clHG, unsigned rootId);

        // set the _adjacentEdges vector for nodeId as the union of
        // its children's adjacent edges, minus those that are contained
        // within the nodId-cluster.
        void induceEdgesFromChildren(unsigned nodeId);

        // the HEM-added fill nodes don't have their adjacentEdge
        // vectors populated...this takes care of that for all
        // nodes below nodeId
        void populateRemainingAdjEdges(unsigned nodeId);

        // adds nodes in the leafHGraph which are not in the hierarchy
        // as (leaf) children of root.
        void putAllNodesInHierarchy();

       public:
        FillableHierarchy(const uofm::vector<const char*>& nodeNames, char* delimStr,  // a sequence of delimeters
                          const HGraphFixed& leafHGraph, const FillableHierarchyParameters& params)
            : GenericHierarchy(nodeNames, delimStr), _leafHGraph(leafHGraph), _params(params), _adjacentEdges(_names.size()), _clusterWeights(_names.size()), _edgeWeights(leafHGraph.getNumEdges()), _clusteredEdgeDegree(leafHGraph.getNumEdges()), _edgeIsIncident(leafHGraph.getNumEdges()) {
                putAllNodesInHierarchy();
                setAreas(_root);
                fillWithHEM();
        }

        FillableHierarchy(const char* hclFileName, const HGraphFixed& leafHGraph, const FillableHierarchyParameters& params) : GenericHierarchy(hclFileName), _leafHGraph(leafHGraph), _params(params), _adjacentEdges(_names.size()), _clusterWeights(_names.size()), _edgeWeights(leafHGraph.getNumEdges()), _clusteredEdgeDegree(leafHGraph.getNumEdges()), _edgeIsIncident(leafHGraph.getNumEdges()) {
                putAllNodesInHierarchy();
                setAreas(_root);
                fillWithHEM();
        }

        virtual ~FillableHierarchy() {}

        double getClusterArea(unsigned clId) const { return _clusterWeights[clId]; }
        const uofm::vector<double>& getClusterAreas() const { return _clusterWeights; }
        const uofm::vector<uofm::vector<unsigned> >& getAdjacentEdges() const { return _adjacentEdges; }
        const uofm::vector<double>& getEdgeWeights() const { return _edgeWeights; }
};

#endif
