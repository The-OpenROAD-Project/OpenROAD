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

// NOTE:  in the interest of having easily managable files, HGFixed
// has been split across a few headers.
//  hgFNode.h   defines class HGFNode
//  hgFEdge.h   defines class HGFEdge
//  hgFNodeInline.h defines inline functions of HGFNode
//  hgFEdgeInline.h defines inline functions of HGFEdge
// the later two header files are included at the end of this document
// because they require that HGraphFixed be defined first

#ifndef _HGRAPHFIXED_H_
#define _HGRAPHFIXED_H_

#include "ABKCommon/abkcommon.h"
#include "newcasecmp.h"
#include "ABKCommon/sgi_hash_map.h"
#include "Ctainers/listO1size.h"
#include "hgBase.h"
#include "hgFNode.h"
#include "hgFEdge.h"

#include <utility>
#include <map>
#include <list>
#include <string>
#include <sstream>
#include <ABKCommon/uofm_alloc.h>

typedef uofm::vector<HGFNode*> ctainerHGFNodesGlobal;
typedef uofm::vector<HGFEdge*> ctainerHGFEdgesGlobal;

typedef ctainerHGFNodesGlobal::const_iterator itHGFNodeGlobal;
typedef ctainerHGFEdgesGlobal::const_iterator itHGFEdgeGlobal;

typedef ctainerHGFNodesGlobal::iterator itHGFNodeGlobalMutable;
typedef ctainerHGFEdgesGlobal::iterator itHGFEdgeGlobalMutable;

struct HGeqstr {
        bool operator()(const char* s1, const char* s2) const
            //{ return strcasecmp(s1, s2) < 0; }
        {
                return newstrcasecmp(s1, s2) == 0;
        }
};

typedef hash_map<uofm::string, unsigned, hash<uofm::string>, std::equal_to<uofm::string> >
    // typedef map<const char*, unsigned, HGeqstr>
    HGNodeNamesMap;
typedef HGNodeNamesMap HGNetNamesMap;

class HGraphFixed : public HGraphBase {
        friend class HGFNode;
        friend class HGFEdge;

       public:
        HGraphFixed(unsigned numNodes = 0, unsigned numWeights = 1, HGraphParameters param = HGraphParameters());

        // this ctor takes:
        // an aux file only
        // a netD and (optionally) and areM
        // a nodes, a nets and (optionally) an wts
        // the can be in any order.  The ctor determins the file
        // types from the filename extensions

        HGraphFixed(const char* Filename1, const char* Filename2 = 0, const char* Filename3 = 0, HGraphParameters param = HGraphParameters());

        // takes in an Hmetis-style hypergraph representation
        HGraphFixed(const int* edges, const int* conns, const double* edgeWts, const double* nodeWts, const int nodeCount, const int connCount, const int edgeCount, bool debug);

        HGraphFixed(const HGraphFixed&);

        virtual ~HGraphFixed();

        virtual HGFEdge* addEdge(HGWeight weight = HGWeight(1.0));
        virtual HGFEdge* fastAddEdge(unsigned numPins, HGWeight weight = HGWeight());
        void adviseNodeDegrees(const uofm::vector<unsigned>& nodeDegrees);

        virtual void finalize();
        // after changes are made to the graph, this method must be called
        // before any methods can be called that rely on src/snk/srcSnk
        // distinction

        unsigned getNumNodes() const;
        unsigned getNumEdges() const;

        double getAvgNodeDegree() const;
        double getAvgEdgeDegree() const;

        itHGFNodeGlobal nodesBegin() const;
        itHGFNodeGlobal nodesEnd() const;

        itHGFNodeGlobal terminalsBegin() const;
        itHGFNodeGlobal terminalsEnd() const;

        itHGFEdgeGlobal edgesBegin() const;
        itHGFEdgeGlobal edgesEnd() const;

        itHGFNodeGlobalMutable nodesBegin();
        itHGFNodeGlobalMutable nodesEnd();
        itHGFEdgeGlobalMutable edgesBegin();
        itHGFEdgeGlobalMutable edgesEnd();

        const HGFNode& getNodeByIdx(unsigned nodeIndex) const;
        bool haveSuchNode(const char* name) const;
        bool haveSuchNet(const char* name) const;
        const HGFNode& getNodeByName(const char* name) const;
        const HGFEdge& getNetByName(const char* name) const;
        HGFNode& getNodeByIdx(unsigned nodeIndex);
        HGFNode& getNodeByName(const char* name);
        const HGFEdge& getEdgeByIdx(unsigned edgeIndex) const;
        HGFEdge& getEdgeByIdx(unsigned edgeIndex);
        HGFEdge& getNetByName(const char* name);

        const uofm::string getNodeNameByIndex(unsigned nId) const;
        const uofm::string getNetNameByIndex(unsigned nId) const;
        const uofm::vector<uofm::string>& getNetNames(void) const;
        const uofm::vector<uofm::string>& getNodeNames(void) const;
        void clearNames(void);
        void clearNameMaps(void);

        unsigned maxNodeIndex() const;
        unsigned maxEdgeIndex() const;

        void temporarilyZeroOutTermWeights(void);
        void reinstateTermWeights(void);

        const Permutation& getNodesSortedByWeights() const;
        const Permutation& getNodesSortedByWeightsWShuffle() const;
        const Permutation& getNodesSortedByDegrees() const;

        virtual bool isConsistent() const;
        virtual void sortAsDB();

        void saveAsNetDAre(const char* baseName) const;
        virtual void saveAsNodesNetsWts(const char* baseName) const;

        void printEdgeSizeStats(std::ostream& str = std::cout) const;
        void printEdgeWtStats(std::ostream& str = std::cout) const;
        void printNodeWtStats(std::ostream& str = std::cout) const;
        void printNodeDegreeStats(std::ostream& str = std::cout) const;

        friend std::ostream& operator<<(std::ostream& out, const HGraphFixed& graph);

       protected:
        bool _finalized;
        bool _haveNames;
        bool _haveNameMaps;

        ctainerHGFNodesGlobal _nodes;
        ctainerHGFEdgesGlobal _edges;

#ifdef SIGNAL_DIRECTIONS
        uofm::vector<std::pair<unsigned, unsigned> > _srcs;
        uofm::vector<std::pair<unsigned, unsigned> > _snks;
#endif
        uofm::vector<std::pair<unsigned, unsigned> > _srcSnks;

        uofm::vector<double> _oldTerminalWeights;
        unsigned _timesZeroedWeights;

        void init(unsigned nNodes, unsigned nWeights);

// these can only be used before finalize is called.
#ifdef SIGNAL_DIRECTIONS
        void addSrc(const HGFNode& node, const HGFEdge& edge);
        void addSnk(const HGFNode& node, const HGFEdge& edge);
#endif
        void addSrcSnk(const HGFNode& node, const HGFEdge& edge);

        virtual void fastAddSrcSnk(HGFNode* node, HGFEdge* edge);

        void computeMaxNodeDegree() const;
        void computeMaxEdgeDegree() const;
        void computeNumPins() const;
        void computeNodesSortedByWeights() const;
        void computeNodesSortedByWeightsWShuffle() const;
        void computeNodesSortedByDegrees() const;

        virtual void sortNodes();
        virtual void sortEdges();

        void removeBigNets();

        // functions useful in populating the HGraph

        virtual void parseAux(const char* filename);
        virtual void readNetD(const char* filename);
        virtual void readAreM(const char* filename);
        virtual void readWts(const char* filename);
        virtual void readNodes(const char* filename);
        virtual void readNets(const char* filename);

        uofm::vector<uofm::string>& getNodeNames(void);
        uofm::vector<uofm::string>& getNetNames(void);
        HGNodeNamesMap& getNodeNamesMap(void);
        HGNetNamesMap& getNetNamesMap(void);

        HGraphFixed(HGraphParameters param);
        void setNetNameManual(unsigned netIdx, const uofm::string& name);

       private:
        uofm::vector<uofm::string> _nodeNames;  // vector of all node names,
        // indexed by nodeId
        uofm::vector<uofm::string> _netNames;  // vector of all net  names,
        // indexed by netId
        HGNodeNamesMap _nodeNamesMap;  // map from name to nodeId.
        // the names pointed to by this map
        // are owned by the _nodeNames vector
        HGNetNamesMap _netNamesMap;  // map from name to netId.
                                     // the names pointed to by this map
                                     // are owned by the _netNames vector
};

#ifndef INLINE_NOTHING
#include "hgFixed.inl"
#include "hgFEdge.inl"
#endif

class HGAlgo {
       public:
        unsigned connectedComponents(const HGraphFixed& graph);
        // Compute the number of connected components in given graphFixed
};

#include "hgSorting.h"

#endif
