


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



namespace aak
{

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class DetailedObjective
{
public:
    DetailedObjective( void ) { m_name = "objective"; }
    virtual ~DetailedObjective( void ) {}

    virtual const std::string& getName( void ) const { return m_name; }

    virtual double curr( void ) { return 0.0; }

    // Different methods for generating moves.  We _must_ overload these.  The
    // generated move should be stored in the manager.
    virtual double  delta( int n, std::vector<Node*>& nodes,
                    std::vector<double>& curX, std::vector<double>& curY,
                    std::vector<unsigned>& curOri,
                    std::vector<double>& newX, std::vector<double>& newY,
                    std::vector<unsigned>& newOri
                )
    { 
        std::cout << "Error." << std::endl; 
        exit(-1);
        return 0.0;
    }

protected:
    std::string m_name;
};

} // namespace aak
