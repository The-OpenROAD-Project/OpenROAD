///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


#pragma once



////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <deque>
#include "architecture.h"
#include "network.h"


namespace dpo 
{

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class DetailedInterleave
{
public:
    DetailedInterleave( Architecture* arch, Network* network, RoutingParams* rt );
    virtual ~DetailedInterleave( void );

    void run( DetailedMgr* mgrPtr, std::string command );
    void run( DetailedMgr* mgrPtr, std::vector<std::string>& args );

    void dp( std::vector<Node*>& nodes, double minX, double maxX );

protected:
    class EdgeAndOffset
    {
    public:
        EdgeAndOffset( int edge, double offset ):
            m_edge(edge),
            m_offset(offset)
        {}
        EdgeAndOffset( const EdgeAndOffset& other ):
            m_edge(other.m_edge),
            m_offset(other.m_offset) 
        {}
        EdgeAndOffset& operator=( const EdgeAndOffset& other )
        {
            if( this != &other )
            {
                m_edge   = other.m_edge;
                m_offset = other.m_offset;
            }
            return *this;
        }
    public:
        int         m_edge;
        double      m_offset;
    };


    class EdgeInterval
    {
    public:
        EdgeInterval():
            m_xmin( std::numeric_limits<double>::max()),
            m_xmax(-std::numeric_limits<double>::max()),
            m_empty(true) 
        {}
        EdgeInterval( double xmin, double  xmax ):
            m_xmin(xmin),
            m_xmax(xmax),
            m_empty(false) 
        {}
        EdgeInterval( const EdgeInterval& other ):
            m_xmin(other.m_xmin),
            m_xmax(other.m_xmax),
            m_empty(other.m_empty) 
        {}
        EdgeInterval& operator=( const EdgeInterval &other )
        {
            if( this != &other )
            {
                m_xmin      = other.m_xmin;
                m_xmax      = other.m_xmax;
                m_empty     = other.m_empty;
            }
            return *this;
        }

        inline double width( void ) const { return (m_empty ? 0.0 : (m_xmax-m_xmin) ); }

        void add( double x )
        {
            m_xmin = std::min( x, m_xmin );
            m_xmax = std::max( x, m_xmax );
            m_empty = false;
        }
    public:
        double      m_xmin;
        double      m_xmax;
        bool        m_empty;
    };

    class SmallProblem
    {
    public:
        SmallProblem( void )
        {
            clear();
        }
        SmallProblem( int nNodes, int nEdges )
        {
            clear(); init( nNodes, nEdges );
        }

        void init( int nNodes, int nEdges )
        {
            m_nNodes = nNodes;
            m_nEdges = nEdges;
            m_adjEdges.resize( nNodes );
            m_widths.resize( nNodes );
            m_x.resize( nNodes );
            m_netBoxes.resize( nEdges );
        }
        void clear( void )
        {
            m_xmin =  std::numeric_limits<double>::max();
            m_xmax = -std::numeric_limits<double>::min();
            m_nNodes = 0;
            m_nEdges = 0;
            m_adjEdges.clear();
            m_widths.clear();
            m_x.clear();
            m_netBoxes.clear();
        }
    public:
        int                         m_nNodes;
        int                         m_nEdges;
        double                      m_xmin;
        double                      m_xmax;
        std::vector<std::vector<EdgeAndOffset> >    
                                    m_adjEdges;
        std::vector<double>         m_widths;
        std::vector<double>         m_x;
        std::vector<EdgeInterval>   m_netBoxes;
    };

    class TableEntry
    {
    public:
        TableEntry( SmallProblem* problem ):m_problem(problem),m_parent(0)
        {
            init();
        }
        void clear()
        {
            init();
        }
        void init()
        {
            m_leftEdge = m_problem->m_xmin;
            m_cost = 0;
            m_netBoxes.resize( m_problem->m_nEdges );
            for( int i = 0; i < m_problem->m_nEdges; i++ )
            {
                m_netBoxes[i] = m_problem->m_netBoxes[i];
                m_cost += m_netBoxes[i].width();
            }
        }
        void copy( TableEntry* other )
        {
            m_leftEdge = other->m_leftEdge;
            m_cost = other->m_cost;
            m_netBoxes = other->m_netBoxes;
        }
        void add( int i )
        {
            double x = m_leftEdge + 0.5 * m_problem->m_widths[i];

            std::vector<EdgeAndOffset>& pins = m_problem->m_adjEdges[i];
            for( int p = 0; p < pins.size(); p++ )
            {
                int edge = pins[p].m_edge;
                double offset = pins[p].m_offset;

                m_cost -= m_netBoxes[edge].width();
                m_netBoxes[edge].add( x + offset );
                m_cost += m_netBoxes[edge].width();
            }
            m_leftEdge += m_problem->m_widths[i];
        }
    public:
        SmallProblem*               m_problem;
        TableEntry*                 m_parent;

        std::vector<EdgeInterval>   m_netBoxes;
        double                      m_leftEdge;
        double                      m_cost;
    };

    struct compareNodesX
    {
        inline bool operator() ( Node* p, Node* q ) const
        {
            return p->getX() < q->getX();
        }
    };

protected:
    void        dp( void );
    bool        build( SmallProblem* probPtr, 
                    double leftLimit, double rightLimit,
                    std::vector<Node*>& nodes, 
                    int istrt, int istop 
                    );
    double      solve( SmallProblem* probPtr );

protected:
    // Standard stuff.
    Architecture*               m_arch;
    Network*                    m_network;
    RoutingParams*              m_rt;

    // For segments.
    DetailedMgr*                m_mgrPtr;

    // Other.
    int                         m_skipNetsLargerThanThis;
    int                         m_traversal;
    std::vector<int>            m_edgeMask;
    std::vector<int>            m_nodeMask;
    std::vector<int>            m_edgeMap;
    std::vector<int>            m_nodeMap;
    std::vector<int>            m_edgeIds;
    std::vector<int>            m_nodeIds;

    int                         m_windowStep;
    int                         m_windowSize;
};

} // namespace dpo
