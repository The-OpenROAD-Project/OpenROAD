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

#include <ClusteredHGraph/clustHGNet.h>
#include <ClusteredHGraph/clustHGCluster.h>
#include <ClusteredHGraph/clustHGTreeBase.h>
#include <Combi/partitionIds.h>
#include <HGraph/hgFixed.h>

using std::ostream;
using std::endl;

// constructor for leaf Clusters
ClHG_Cluster::ClHG_Cluster(const HGraphFixed& hgraph, const HGFNode& node, ClHG_ClusterTreeBase& tree, PartitionIds fixed, PartitionIds part) : _index(node.getIndex()), _area(hgraph.getWeight(node.getIndex())), _leafDesc(1), _tag(SHRT_MAX), _part(part), _fixed(fixed), _terminal(false), _clusterable(true) {
        hgraphIds.reserve(4);
        hgraphIds.push_back(node.getIndex());
        _cutNets.reserve(node.getDegree() * 2);

        itHGFEdgeLocal nItr;
        for (nItr = node.edgesBegin(); nItr != node.edgesEnd(); nItr++) {
                HGFEdge& curNet = **nItr;
                unsigned nId = curNet.getIndex();
                if (curNet.getDegree() > 1) _cutNets.push_back(ClHG_CutNet(nId, tree.getNetByIdx(nId), 1));
        }
}
// add newChild to this cluster..
void ClHG_Cluster::mergeWith(ClHG_Cluster& newChild, ClHG_ClusterTreeBase& tree) {
        abkassert(_clusterable && newChild._clusterable, "attempted to merge an unclusterable node");

        _tag = -1;
        _terminal |= newChild._terminal;
        _part &= newChild._part;
        _fixed &= newChild._fixed;
        _area += newChild._area;
        _leafDesc += newChild._leafDesc;

        abkassert(!_part.isEmpty() && !_fixed.isEmpty(), "clustered nodes which produced an empty part or terminal");

        hgraphIds.insert(hgraphIds.end(), newChild.hgraphIds.begin(), newChild.hgraphIds.end());

        ClHG_CutNetItr cutN;
        //_cutNets.reserve(_cutNets.size() + newChild.getDegree());

        for (cutN = cutNetsBegin(); cutN != cutNetsEnd(); cutN++) tree._numContained[cutN->netId] = cutN->numLeaves;

        for (cutN = newChild.cutNetsBegin(); cutN != newChild.cutNetsEnd(); cutN++) {
                unsigned nId = cutN->netId;
                if (tree._numContained[nId] == 0 && cutN->net->getWeight() > 0.0) _cutNets.push_back(*cutN);
                tree._numContained[nId] += cutN->numLeaves;
        }

        _cutNets.erase(std::remove_if(_cutNets.begin(), _cutNets.end(), RemoveContainedNets(tree._numContained)), _cutNets.end());
}

ostream& operator<<(ostream& out, const ClHG_Cluster& cl) {
        out << "{ Cluster:    " << cl.getIndex() << endl;
        out << "  Size:       " << cl.getArea() << endl;
        out << "  Tag:        " << cl.getTag() << endl;
        out << "  NumCutNets: " << cl.getDegree() << endl;
        out << "  Mappings: " << cl.hgraphIds << endl;
        out << "  Part: " << cl._part << "  Fixed: " << cl._fixed << endl;
        out << "}" << endl;
        return out;
}
