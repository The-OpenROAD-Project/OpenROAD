


#pragma once



// Description:
// - An objective function to help with computation of change in wirelength
//   if doing some sort of moves (e.g., single, swap, sets, etc.).



////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <deque>
#include "architecture.h"
#include "network.h"
#include "router.h"
#include "detailed_objective.h"

namespace aak 
{


////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class DetailedOrient;
class DetailedMgr;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

class DetailedHPWL : public DetailedObjective
{
// For WL objective.
public:
    DetailedHPWL( Architecture* arch, Network* network, RoutingParams* rt );
    virtual ~DetailedHPWL( void );

    void        init( void );
    double      curr( void );
    double      delta( int n, std::vector<Node*>& nodes, 
                    std::vector<double>& curX, std::vector<double>& curY, 
                    std::vector<unsigned>& curOri, 
                    std::vector<double>& newX, std::vector<double>& newY, 
                    std::vector<unsigned>& newOri
                );
    void       getCandidates( std::vector<Node*>& candidates );

    // Other.
    void        init( DetailedMgr* mgrPtr, DetailedOrient* orientPtr );
    double      delta( Node* ndi, double new_x, double new_y );
    double      delta( Node* ndi, Node* ndj );
    double      delta( Node* ndi, double target_xi, double target_yi, 
                    Node* ndj, double target_xj, double target_yj );

////////////////////////////////////////////////////////////////////////////////

protected:
    Architecture*               m_arch;
    Network*                    m_network;
    RoutingParams*              m_rt;

    DetailedMgr*                m_mgrPtr;
    DetailedOrient*             m_orientPtr;

    // Other.
    int                         m_skipNetsLargerThanThis;
    int                         m_traversal;
    std::vector<int>            m_edgeMask;
};



} // namespace aak
