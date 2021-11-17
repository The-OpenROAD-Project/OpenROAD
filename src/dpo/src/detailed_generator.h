

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


////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////

namespace aak
{

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class DetailedGenerator
{
public:
    DetailedGenerator( void ) {}
    virtual ~DetailedGenerator( void ) {}

    // Different methods for generating moves.  We _must_ overload these.  The
    // generated move should be stored in the manager.
    virtual bool generate( DetailedMgr* mgr, std::vector<Node*>& candiates ) 
    { 
        std::cout << "Error." << std::endl; 
        exit(-1);
        return false;
    };

    virtual void stats( void ) 
    {
        std::cout << "Error." << std::endl;
        exit(-1);
    }

    virtual void init( DetailedMgr* mgr )
    {
        std::cout << "Error." << std::endl;
    }

protected:
};

} // namespace aak

