


#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <deque>
#include "architecture.h"
#include "network.h"
#include "router.h"
#include "detailed_segment.h"
#include "detailed_manager.h"
#include "detailed_generator.h"
#include "detailed_objective.h"


namespace aak
{

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class DetailedRandom
{
public:
    enum DrcMode
    {
        DrcMode_NoPenalty           = 0,
        DrcMode_NormalPenalty,
        DrcMode_Eliminate,
        DrcMode_Unknown
    };
    enum MoveMode
    {
        MoveMode_Median             = 0,
        MoveMode_CellDensity1,
        MoveMode_RandomWindow,
        MoveMode_Unknown
    };
    enum MoveSource
    {
        MoveSource_All              = 0,
        MoveSource_Wirelength,
        MoveSource_DrcViolators,
        MoveSource_Unknown
    };

public:
    DetailedRandom( Architecture* arch, Network* network, RoutingParams* rt );
    virtual ~DetailedRandom( void );

    void run( DetailedMgr* mgrPtr, std::string command );
    void run( DetailedMgr* mgrPtr, std::vector<std::string>& args );

protected:
    double go( void );

    void collectCandidates( void );

protected:
    // Standard stuff.
    DetailedMgr*                m_mgrPtr;

    Architecture*               m_arch;
    Network*                    m_network;
    RoutingParams*              m_rt;

    // Candidate cells.
    std::vector<Node*>          m_candidates;

    // For generating move lists.
    std::vector<DetailedGenerator*> m_generators;

    // For evaluating objectives.
    std::vector<DetailedObjective*> m_objectives;

    // Parameters controlling the moves.
    double                      m_movesPerCandidate;

    // For costing.
    std::vector<double>         m_initCost;
    std::vector<double>         m_currCost;
    std::vector<double>         m_nextCost;
    std::vector<double>         m_deltaCost;

    // For obj evaluation.
    std::vector<std::string>    m_expr;
};


class RandomGenerator : public DetailedGenerator
{
public:
    RandomGenerator( void );
    virtual ~RandomGenerator( void );
public:
   virtual bool generate( DetailedMgr* mgr, std::vector<Node*>& candiates );
   virtual void stats( void );
   virtual void init( DetailedMgr* mgr ) { (void)mgr; }
protected:
    DetailedMgr*        m_mgr;
    Architecture*       m_arch;
    Network*            m_network;
    RoutingParams*      m_rt;

    int                 m_attempts;
    int                 m_moves;
    int                 m_swaps;
};

class DisplacementGenerator : public DetailedGenerator
{
public:
    DisplacementGenerator( void );
    virtual ~DisplacementGenerator( void );
public:
   virtual bool generate( DetailedMgr* mgr, std::vector<Node*>& candiates );
   virtual void stats( void );
   virtual void init( DetailedMgr* mgr ) { (void)mgr; }
protected:
    DetailedMgr*        m_mgr;
    Architecture*       m_arch;
    Network*            m_network;
    RoutingParams*      m_rt;

    int                 m_attempts;
    int                 m_moves;
    int                 m_swaps;
};


} // namespace aak
