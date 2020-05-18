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

//! author =" Max Moroz"

#ifndef _SUBHG_H_
#define _SUBHG_H_

#include "hgFixed.h"
#include "ABKCommon/sgi_hash_map.h"
#include "Combi/mapping.h"
#include <map>
#include <ABKCommon/uofm_alloc.h>

class HGraphFixed;
class HGFNode;
class HGFEdge;

typedef hash_map<unsigned, unsigned, hash<unsigned>, equal_to<unsigned> >
    // typedef  map<unsigned, unsigned, std::less<unsigned> >
    OrigToNewMap;

//: Derived from HyperGraph with constructor from nodes/edges of another HGraph.
class SubHGraph : public HGraphFixed {
       protected:
        OrigToNewMap origNodeToNew;
        // given the Idx of a node/edge in the 'original' HGraph,
        // these return the Idx of the coresponding node/edge
        // in the SubHGraph (old to new)
        OrigToNewMap origEdgeToNew;

        Mapping newNodeToOrig;
        // idx of a node/edge in the subHGraph => index of a
        // node/edge in the origonal HGraph (new to old).
        //   can use Mapping since nodes are numbered continously
        Mapping* newEdgeToOrig;

        bool _clearTerminalAreas;
        bool _ignoreSize1Nets;
        unsigned _threshold;

        inline void addTerminal(const HGraphFixed& origHGraph, const HGFNode& origNode, unsigned newId);
        inline void addNode(const HGraphFixed& origHGraph, const HGFNode& origNode, unsigned newId);
        inline void addNode(double weight, char* name, unsigned origId, unsigned newId);

        inline HGFEdge& addNewEdge(const HGFEdge& origEdge);

        inline void fastAddTerminal(const HGraphFixed& origHGraph, const HGFNode& origNode, unsigned newId);
        inline void fastAddNode(const HGraphFixed& origHGraph, const HGFNode& origNode, unsigned newId);
        inline HGFEdge& fastAddNewEdge(const HGFEdge& origEdge, unsigned numPins);

        inline void createIdentityMappings(void);

        SubHGraph(unsigned numTerminals, unsigned numNonTerminals, unsigned numEdges = UINT_MAX,  // just a bound..does not have to be exact
                  bool clearTerminalAreas = false, bool ignoreSize1Nets = true, unsigned threshold = 0)
            : HGraphFixed(numTerminals + numNonTerminals),
              newNodeToOrig(numTerminals + numNonTerminals, UINT_MAX / 2),
              // newEdgeToOrig(numEdges, UINT_MAX/2),
              newEdgeToOrig(NULL),
              _clearTerminalAreas(clearTerminalAreas),
              _ignoreSize1Nets(ignoreSize1Nets),
              _threshold(threshold) {
                if (numEdges != UINT_MAX) newEdgeToOrig = new Mapping(numEdges, UINT_MAX / 2);
                _numTerminals = numTerminals;
                clearNameMaps();
                clearNames();
        }

       public:
        SubHGraph(const HGraphFixed& origHGraph, const uofm::vector<const HGFNode*>& nonTerminals, const uofm::vector<const HGFNode*>& terminals, const uofm::vector<const HGFEdge*>& nets, bool clearTerminalAreas = false, bool ignoreSize1Nets = true, unsigned threshold = 0);
        // all terminals go first and are in the same order as in the input

        // these new constructors here just to satisfy HGraphWDims

        SubHGraph(unsigned numNodes = 0, unsigned numWeights = 1, HGraphParameters param = HGraphParameters()) : HGraphFixed(numNodes, numWeights, param), newEdgeToOrig(NULL) {
                _clearTerminalAreas = false;
                _ignoreSize1Nets = true;
                _threshold = 0;
        }

        SubHGraph(HGraphParameters param) : HGraphFixed(param), newEdgeToOrig(NULL) {
                _clearTerminalAreas = false;
                _ignoreSize1Nets = true;
                _threshold = 0;
        }

        SubHGraph(const HGraphFixed& hg) : HGraphFixed(hg), newEdgeToOrig(NULL) {
                _clearTerminalAreas = false;
                _ignoreSize1Nets = true;
                _threshold = 0;
        }

        virtual ~SubHGraph();

        void printNodeMap() const;

        const HGFNode& getNewNodeByOrigIdx(unsigned origIdx) const { return getNodeByIdx((*origNodeToNew.find(origIdx)).second); }

        const HGFEdge& getNewEdgeByOrigIdx(unsigned origIdx) const { return getEdgeByIdx((*origEdgeToNew.find(origIdx)).second); }

        unsigned origNode2NewIdx(unsigned origIdx) const { return (*origNodeToNew.find(origIdx)).second; }
        // from original node to find the new index of mapping node in subhgraph

        unsigned origEdge2NewIdx(unsigned origIdx) const { return (*origEdgeToNew.find(origIdx)).second; }
        // from original edge to find the new index of mapping edge in subhgraph

        unsigned newNode2OrigIdx(unsigned newIdx) const { return newNodeToOrig[newIdx]; }
        // from new node to find the index of original node in hgraph

        unsigned newEdge2OrigIdx(unsigned newIdx) const {
                abkfatal(newEdgeToOrig,
                         "The newEdgeToOrig pointer is disabled, please "
                         "construct with"
                         " a non-trivial edge bound to enable this capability");
                return (*newEdgeToOrig)[newIdx];
        }
        // from new edge to find the index of original edge in hgraph

        const OrigToNewMap& getOrigToNewNodeMapping() const { return origNodeToNew; }
        // get the mapping relationship form original to new
        const Mapping& getNewToOrigNodeMapping() const { return newNodeToOrig; }
        // get the mapping relationship from new to original
};

inline void SubHGraph::addTerminal(const HGraphFixed& origHGraph, const HGFNode& origNode, unsigned newId) {
        if (_clearTerminalAreas)
                setWeight(newId, 0);
        else
                setWeight(newId, origHGraph.getWeight(origNode.getIndex()));

        //    uofm::string nName =  origNode.getName() ;
        //    _nodeNames[newId] = nName;
        //    _nodeNamesMap[nName]  = newId;

        origNodeToNew[origNode.getIndex()] = newId;
        newNodeToOrig[newId] = origNode.getIndex();
}

inline void SubHGraph::addNode(const HGraphFixed& origHGraph, const HGFNode& origNode, unsigned newId) {
        setWeight(newId, origHGraph.getWeight(origNode.getIndex()));

        //    uofm::string nName = origNode.getName() ;
        //    _nodeNames[newId] = nName;
        //    _nodeNamesMap[nName]  = newId;

        origNodeToNew[origNode.getIndex()] = newId;
        newNodeToOrig[newId] = origNode.getIndex();
}

inline void SubHGraph::addNode(double weight, char* name, unsigned origId, unsigned newId) {
        setWeight(newId, weight);
        //  uofm::string nName = name ;
        //  _nodeNames[newId] = nName;
        //  _nodeNamesMap[nName]  = newId;
        origNodeToNew[origId] = newId;
        newNodeToOrig[newId] = origId;
}

inline HGFEdge& SubHGraph::addNewEdge(const HGFEdge& origEdge) {
        HGFEdge* newEdge = addEdge(origEdge.getWeight());
        abkassert(origEdge.getWeight() > 0, "constructing subgraph w/ 0 weight edge");

        origEdgeToNew[origEdge.getIndex()] = newEdge->getIndex();
        if (newEdgeToOrig) (*newEdgeToOrig)[newEdge->getIndex()] = origEdge.getIndex();

        return *newEdge;
}

inline void SubHGraph::fastAddTerminal(const HGraphFixed& origHGraph, const HGFNode& origNode, unsigned newId) {
        if (_clearTerminalAreas)
                setWeight(newId, 0);
        else
                setWeight(newId, origHGraph.getWeight(origNode.getIndex()));

        //	uofm::string nName = origNode.getName() ;

        //    _nodeNames[newId] = nName;
        //    _nodeNamesMap[nName]  = newId;

        origNodeToNew[origNode.getIndex()] = newId;
        newNodeToOrig[newId] = origNode.getIndex();
        _nodes[newId]->_edges.reserve(origNode.getDegree());
}

inline void SubHGraph::fastAddNode(const HGraphFixed& origHGraph, const HGFNode& origNode, unsigned newId) {
        setWeight(newId, origHGraph.getWeight(origNode.getIndex()));

        //	uofm::string nName = origNode.getName() ;

        //    _nodeNames[newId] = nName;
        //    _nodeNamesMap[nName]  = newId;

        origNodeToNew[origNode.getIndex()] = newId;
        newNodeToOrig[newId] = origNode.getIndex();
        _nodes[newId]->_edges.reserve(origNode.getDegree());
}

inline HGFEdge& SubHGraph::fastAddNewEdge(const HGFEdge& origEdge, unsigned numPins) {
        HGFEdge* newEdge = fastAddEdge(numPins, origEdge.getWeight());

        origEdgeToNew[origEdge.getIndex()] = newEdge->getIndex();
        if (newEdgeToOrig) (*newEdgeToOrig)[newEdge->getIndex()] = origEdge.getIndex();

        return *newEdge;
}

inline void SubHGraph::createIdentityMappings(void) {
        origNodeToNew.clear();
        newNodeToOrig = Mapping(getNumNodes(), INT_MAX);
        for (unsigned i = 0; i < getNumNodes(); ++i) {
                origNodeToNew[i] = i;
                newNodeToOrig[i] = i;
        }

        origEdgeToNew.clear();
        if (newEdgeToOrig) {
                delete newEdgeToOrig;
                newEdgeToOrig = new Mapping(getNumEdges(), INT_MAX);
        }
        for (unsigned i = 0; i < getNumEdges(); ++i) {
                origEdgeToNew[i] = i;
                if (newEdgeToOrig) (*newEdgeToOrig)[i] = i;
        }
}

#endif
