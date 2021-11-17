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
#include "plotgnu.h"
#include "router.h"

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////


namespace dpo 
{
class DetailedSeg;
class DetailedMgr;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class LegalizeParams
{
public:
    LegalizeParams():
        m_skipLegalization( false ),
        m_targetMaxMovement( std::numeric_limits<double>::max() ), // No limit.
        m_targetUt( 1.0 ) // No limit.
    {}
public:
    bool        m_skipLegalization;
    double      m_targetMaxMovement;
    double      m_targetUt;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class Legalize 
{
public:
    Legalize( LegalizeParams& params );
    virtual ~Legalize();

    void legalize( DetailedMgr& mgr );
    //void legalize( Architecture* arch, Network* network, RoutingParams* rt );
    void plotter( PlotGnu* plotter ) { m_plotter = plotter; }


public:


    class Rect
    {
    public:
        Rect():m_l(0),m_r(0),m_b(0),m_t(0)
        {};
        Rect(double l, double r, double b, double t):m_l(l),m_r(r),m_b(b),m_t(t)
        {}
        Rect(const Rect& other):m_l(other.m_l),m_r(other.m_r),m_b(other.m_b),m_t(other.m_t)
        {}
        Rect& operator=(const Rect& other )
        {
            if( this != &other )
            {
                m_l = other.m_l;
                m_r = other.m_r;
                m_b = other.m_b;
                m_t = other.m_t;
            }
            return *this;
        }
    public:
        double m_l, m_r, m_b, m_t;
    };

    class Clump
    {
    public:
        Clump() {}
        virtual ~Clump() {}
    public:
        int m_id;
        double m_weight;
        double m_wposn;
        double m_width;
        double m_posn;
        std::vector<Node*> m_nodes;
    };

protected:
    void set_original_positions( void );
    void restore_original_positions( void );
    void force_cells_into_regions( void );
    bool inside_rectangle( Node* ndi, Rectangle& rect );
    bool fits_in_rectangle( Node* ndi, Rectangle& rect );
    double distance_to_rectangle( Node* ndi, Rectangle& rect );
    void move_into_rectangle( Node* ndi, Rectangle& rect );
    void print_dist_from_start_positions( void );
    void print_dist_from_orig_positions( void );

    void moveCellsBetweenSegments( DetailedSeg* oldPtr, int leftRightTol, double offsetTol, bool ignoreGaps = true );
    void moveCellsBetweenSegments( void );

    void doClumping( std::vector<Node*>& nodes_in, double xmin, double xmax, bool debug = false );
    void doClumping( Clump* r, std::vector<Clump>& clumps, double xmin, double xmax, bool debug = false );

protected:
    // Standard stuff.
    LegalizeParams&         m_params;

    DetailedMgr*            m_mgr;

    Network*                m_network;
    Architecture*           m_arch;
    RoutingParams*          m_rt;

    PlotGnu*                m_plotter;

    std::vector<double>     m_origX;
    std::vector<double>     m_origY;
};

} // namespace dpo
