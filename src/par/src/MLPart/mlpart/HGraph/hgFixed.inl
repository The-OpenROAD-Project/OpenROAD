
#ifdef SIGNAL_DIRECTIONS
inline void HGraphFixed::addSrc(const HGFNode& node, const HGFEdge& edge)
{ _srcs.push_back(std::make_pair(node.getIndex(), edge.getIndex())); }

inline void HGraphFixed::addSnk(const HGFNode& node, const HGFEdge& edge)
{ _snks.push_back(std::make_pair(node.getIndex(), edge.getIndex())); }
#endif

inline void HGraphFixed::addSrcSnk(const HGFNode& node, const HGFEdge& edge)
{ _srcSnks.push_back(std::make_pair(node.getIndex(), edge.getIndex())); }

inline void HGraphFixed::fastAddSrcSnk(HGFNode* node, HGFEdge* edge)
{
    edge->_nodes.push_back(node);
    node->_edges.push_back(edge);
}

inline HGFEdge* HGraphFixed::fastAddEdge(unsigned numPins, HGWeight weight)
{
    abkassert(_addEdgeStyle != SLOW_ADDEDGE_USED,
        "cannot mix fast and slow addEdge in one HGraph object");

    abkassert2(_param.makeAllSrcSnk,
        "cannot use fast addEdge if graph has directionality",
        " (i.e., allSrcSnks is false)");

#ifdef DABKDEBUG
    _addEdgeStyle=FAST_ADDEDGE_USED;

    if (_param.netThreshold<numPins)
    {
        abkfatal(_param.removeBigNets,
            "fast addEdge requires that removeBigNets is set to true");
        return &mDummyEdge;
    }
#endif

    HGFEdge* edge=new HGFEdge(weight);
    _edges.push_back(edge);
    edge->_index=_edges.size()-1;
    edge->_nodes.reserve(numPins);
#ifdef SIGNAL_DIRECTIONS
    edge->_snksBegin = edge->_srcSnksBegin = edge->_nodes.begin();
#endif
    return edge;
}

inline std::vector<std::string>& HGraphFixed::getNodeNames(void)
{
    abkfatal(_haveNames,"Names are gone, check if clearNames called");
    return _nodeNames;
}

inline std::vector<std::string>& HGraphFixed::getNetNames(void)
{
    abkfatal(_haveNames,"Names are gone, check if clearNames called");
    return _netNames;
}

inline HGNodeNamesMap& HGraphFixed::getNodeNamesMap(void)
{
    abkfatal(_haveNameMaps,"Names are gone, check if clearNames called");
    return _nodeNamesMap;
}

inline HGNetNamesMap& HGraphFixed::getNetNamesMap(void)
{
    abkfatal(_haveNameMaps,"Names are gone, check if clearNames called");
    return _netNamesMap;
}

inline unsigned HGraphFixed::maxNodeIndex() const { return _nodes.size()-1; }

inline unsigned HGraphFixed::maxEdgeIndex() const { return _edges.size()-1; }

inline const HGFNode& HGraphFixed::getNodeByIdx(unsigned nodeIndex) const
{
    abkassert3(nodeIndex<getNumNodes(), nodeIndex,
        " is an out-of-range index, max=", getNumNodes());
    return *_nodes[nodeIndex];
}

inline bool HGraphFixed::haveSuchNode(const char* name) const
{
    abkfatal(_haveNameMaps, "Name maps no longer exist. Check when clearNameMaps() was called.");
    HGNodeNamesMap::const_iterator nameItr = _nodeNamesMap.find(name);
    return nameItr != _nodeNamesMap.end();
}

inline bool HGraphFixed::haveSuchNet(const char* name) const
{
    abkfatal(_haveNameMaps, "Name maps no longer exist. Check when clearNameMaps() was called.");
    HGNetNamesMap::const_iterator nameItr = _netNamesMap.find(name);
    return nameItr != _netNamesMap.end();
}

inline const HGFNode& HGraphFixed::getNodeByName(const char* name) const
{
    abkfatal(_haveNameMaps, "Name maps no longer exist. Check when clearNameMaps() was called.");
    HGNodeNamesMap::const_iterator nameItr = _nodeNamesMap.find(name);
    abkassert2(nameItr != _nodeNamesMap.end(),
        "name not found in getNodeByName: ", name);
    return *_nodes[(*nameItr).second];
}

inline const HGFEdge& HGraphFixed::getNetByName(const char* name) const
{
    abkfatal(_haveNameMaps, "Name maps no longer exist. Check when clearNameMaps() was called.");
    HGNetNamesMap::const_iterator nameItr = _netNamesMap.find(name);
    abkassert2(nameItr != _netNamesMap.end(),
        "name not found in getNetByName: ", name);
    return *_edges[(*nameItr).second];
}

inline HGFNode& HGraphFixed::getNodeByIdx(unsigned nodeIndex)
{
    abkassert3(nodeIndex<getNumNodes(), nodeIndex,
        " is an out-of-range index, max=", getNumNodes());
    return *_nodes[nodeIndex];
}

inline HGFNode& HGraphFixed::getNodeByName(const char* name)
{
    abkfatal(_haveNameMaps, "Name maps no longer exist. Check when clearNameMaps() was called.");
    HGNodeNamesMap::const_iterator nameItr = _nodeNamesMap.find(name);
    abkassert2(nameItr != _nodeNamesMap.end(),
        "name not found in getNodeByName: ", name);
    return *_nodes[(*nameItr).second];
}

inline const HGFEdge& HGraphFixed::getEdgeByIdx(unsigned edgeIndex) const
{
    abkassert(edgeIndex<getNumEdges(), "edge index too big");
    return *_edges[edgeIndex];
}

inline HGFEdge& HGraphFixed::getEdgeByIdx(unsigned edgeIndex)
{
    abkassert(edgeIndex<getNumEdges(), "edge index too big");
    return *_edges[edgeIndex];
}

inline HGFEdge& HGraphFixed::getNetByName(const char* name)
{
    abkfatal(_haveNameMaps, "Name maps no longer exist. Check when clearNameMaps() was called.");
    HGNetNamesMap::const_iterator nameItr = _netNamesMap.find(name);
    abkassert2(nameItr != _netNamesMap.end(),
        "name not found in getNetByName: ", name);
    return *_edges[(*nameItr).second];
}

inline const Permutation& HGraphFixed::getNodesSortedByWeights() const
{
    if (_weightSort.getSize()==0)
        computeNodesSortedByWeights();
    return _weightSort;
}

inline const Permutation& HGraphFixed::getNodesSortedByWeightsWShuffle() const
{
    if (_weightSort.getSize()==0)
        computeNodesSortedByWeightsWShuffle();
    return _weightSort;
}

inline const Permutation& HGraphFixed::getNodesSortedByDegrees() const
{
    if (_degreeSort.getSize()==0)
        computeNodesSortedByDegrees();
    return _degreeSort;
}

inline unsigned HGraphFixed::getNumNodes() const { return _nodes.size(); }

inline unsigned HGraphFixed::getNumEdges() const { return _edges.size(); }

inline double HGraphFixed::getAvgNodeDegree() const { return 1.0*getNumPins()/getNumNodes(); }

inline double HGraphFixed::getAvgEdgeDegree() const { return 1.0*getNumPins()/getNumEdges(); }

inline itHGFNodeGlobal HGraphFixed::nodesBegin() const { return _nodes.begin(); }

inline itHGFNodeGlobal HGraphFixed::nodesEnd()   const { return _nodes.end();   }

inline itHGFNodeGlobal HGraphFixed::terminalsBegin() const { return _nodes.begin(); }

inline itHGFNodeGlobal HGraphFixed::terminalsEnd()   const { return _nodes.begin()+_numTerminals; }

inline itHGFEdgeGlobal HGraphFixed::edgesBegin() const { return _edges.begin(); }

inline itHGFEdgeGlobal HGraphFixed::edgesEnd()   const { return _edges.end();   }

inline itHGFNodeGlobalMutable HGraphFixed::nodesBegin(){ return _nodes.begin(); }

inline itHGFNodeGlobalMutable HGraphFixed::nodesEnd()  { return _nodes.end();   }

inline itHGFEdgeGlobalMutable HGraphFixed::edgesBegin(){ return _edges.begin(); }

inline itHGFEdgeGlobalMutable HGraphFixed::edgesEnd()  { return _edges.end();   }

inline const std::string HGraphFixed::getNodeNameByIndex(unsigned nId) const
{
    //abkfatal(_haveNames,"Names are gone, check if clearNames called");
    if(_haveNames)
        return _nodeNames[nId];
    else
    {
        std::string rval;
        if(isTerminal(nId))
        {
            rval+="p";
            std::stringstream ss;
            ss<<(nId+1);
            std::string numstr;
            ss>>numstr;
            rval+=numstr;
        }
        else
        {
            rval+="a";
            abkfatal(nId>=getNumTerminals(),"BOOOM!");
            std::stringstream ss;
            ss<<nId-getNumTerminals();
            std::string numstr;
            ss>>numstr;
            rval+=numstr;
        }
        return rval;
    }
}

inline const std::string HGraphFixed::getNetNameByIndex(unsigned nId) const
{
    //abkfatal(_haveNames,"Names are gone, check if clearNames called");
    if(_haveNames)
        return _netNames[nId];
    else
    {
        std::string rval("N");
        std::stringstream ss;
        ss<<nId;
        std::string numstr;
        ss>>numstr;
        rval+=numstr;
        return rval;
    }
}

inline const std::vector<std::string>& HGraphFixed::getNetNames(void) const
{
    abkfatal(_haveNames,"Names are gone, check if clearNames called");
    return _netNames;
}

inline const std::vector<std::string>& HGraphFixed::getNodeNames(void) const
{
    abkfatal(_haveNames,"Names are gone, check if clearNames called");
    return _nodeNames;
}
