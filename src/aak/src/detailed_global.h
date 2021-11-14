

#pragma once


#include <vector>
#include <deque>
#include "architecture.h"
#include "network.h"
#include "router.h"
#include "rectangle.h"
#include "detailed_generator.h"


namespace aak
{


class DetailedSeg;
class DetailedMgr;


// CLASSES ===================================================================
class DetailedGlobalSwap : public DetailedGenerator
{
public:
    DetailedGlobalSwap( Architecture* arch, Network* network, RoutingParams* rt );
    DetailedGlobalSwap( void );
    virtual ~DetailedGlobalSwap( void );

    // Intefaces for scripting.
    void run( DetailedMgr* mgrPtr, std::string command );
    void run( DetailedMgr* mgrPtr, std::vector<std::string>& args );

    // Interface for move generation.
    virtual bool generate( DetailedMgr* mgr, std::vector<Node*>& candiates );
    virtual void stats( void );
    virtual void init( DetailedMgr* mgr );

protected:

    struct compareNodesX
    {
        inline bool operator() ( Node* p, Node* q ) const
        {
            return p->getX() < q->getX();
        }
        inline bool operator()( Node*& s, double i ) const
        {
            return s->getX() < i;
        }
        inline bool operator()( double i, Node*& s ) const
        {
            return i < s->getX();
        }
    };

protected:
    void        globalSwap( void ); // tries to avoid overlap.
    bool        calculateEdgeBB( Edge* ed, Node* nd, Rectangle& bbox );
    bool        getRange( Node*, Rectangle& );
    double      delta( Node* ndi, double new_x, double new_y );
    double      delta( Node* ndi, Node* ndj );

    bool        generate( Node* ndi );

protected:
    // Standard stuff.
    DetailedMgr*                m_mgr;
    Architecture*               m_arch;
    Network*                    m_network;
    RoutingParams*              m_rt;

    // Other.
    int                         m_skipNetsLargerThanThis;
    std::vector<int>            m_edgeMask;
    int                         m_traversal;

    std::vector<double>         m_xpts;
    std::vector<double>         m_ypts;

    // For use as a move generator.
    int                         m_attempts;
    int                         m_moves;
    int                         m_swaps;
};

} // namespace aak

