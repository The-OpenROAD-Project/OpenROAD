


#pragma once



#include <vector>
#include <deque>
#include "architecture.h"
#include "network.h"
#include "router.h"



namespace aak
{

class DetailedSeg;
class DetailedMgr;


// CLASSES ===================================================================
class DetailedReorderer
{
public:
    DetailedReorderer( Architecture* arch, Network* network, RoutingParams* rt );
    virtual ~DetailedReorderer( void );

    void run( DetailedMgr* mgrPtr, std::string command );
    void run( DetailedMgr* mgrPtr, std::vector<std::string>& args );

protected:
    void reorder( void );
    void reorder( std::vector<Node*>& nodes, 
        int istrt, int istop, 
        double leftLimit, double rightLimit,
        int segId, int rowId );
    double cost( std::vector<Node*>& nodes, int istrt, int istop );

protected:
    // Standard stuff.
    Architecture*               m_arch;
    Network*                    m_network;
    RoutingParams*              m_rt;

    // For segments.
    DetailedMgr*                m_mgrPtr;

    // Other.
    int                         m_skipNetsLargerThanThis;
    std::vector<int>            m_edgeMask;
    int                         m_traversal;
    int                         m_windowSize;
};

} // namespace aak
