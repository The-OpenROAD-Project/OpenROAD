

#include <stdio.h>
#include <iostream>
#include "mer.h"
#include "architecture.h"
#include "network.h"


namespace aak
{

SpaceManager::SpaceManager( void )
{
    ;
}
SpaceManager::SpaceManager( double xmin, double xmax, double ymin, double ymax )
{
    _freeSpaceRectangles.push_back( getRectangle( xmin, xmax, ymin, ymax ) );
}
SpaceManager::~SpaceManager( void )
{
    cleanup0();
}

Rectangle* SpaceManager::AddFullRectangle( double xmin, double xmax, double ymin, double ymax )
{
    bool                    didDelete;
    Rectangle*              f = getRectangle( xmin, xmax, ymin, ymax );
    int                     i, j, direction;
    Rectangle*              o;
    Rectangle*              tmp;
    std::vector<Rectangle*> clist;
    std::deque<Rectangle*>  adjacentLES[4]; // L, R, T, B.
    std::deque<Rectangle*>  possibleLES[4]; // L, R, T, B.
    bool                    contained;

    // Add the rectangle to the list of full rectangles (occupied space).
    _fullSpaceRectangles.push_back( f );

    // Find adjacent/overlapping free rectangles.
    determineNeighbors( f, clist );

    //std::cout << "Adding rect [" << f->xmin() << "," << f->ymin() << "]-[" << f->xmax() << "," << f->ymax() << "]" << ", "
    //    << "neighbours is " << clist.size() << std::endl;

    // Examine each free rectangle.
    for( i = 0; i < clist.size(); i++ ) 
    {
        o = clist[i];

        //std::cout << "=> neighbour rect [" << o->xmin() << "," << o->ymin() << "]-[" << o->xmax() << "," << o->ymax() << "]" 
        //    << std::endl;


        // Is there a bug here???
        if     ( o->xmin() < f->xmin() && o->xmax() == f->xmin() ) 
        //if     ( o->xmin() < f->xmin() && o->xmax() == f->xmax() ) 
        // I think the line commented out above is an error.  I think we are
        // looking to see if the neighbour touches on the left so we should
        // compare the neighbour max to the box min (for comparison, see to
        // "bottom check" below.
        {
            adjacentLES[0].push_back( o );
        } 
        else if( o->xmax() > f->xmax() && o->xmin() == f->xmax() ) 
        {
            // Clearly adjacent on the right.
            adjacentLES[1].push_back( o );
        } 
        else if( o->ymin() < f->ymin() && o->ymax() == f->ymin() ) 
        {
            // Clearly adjacent on the bottom.
            adjacentLES[2].push_back( o );
        } 
        else if( o->ymax() > f->ymax() && o->ymin() == f->ymax() ) 
        {
            // Clearly adjacent on the top.
            adjacentLES[3].push_back( o );
        }
        else
        {
            if( o->xmin() < f->xmin() ) 
            {
                possibleLES[0].push_back( getRectangle( o->xmin(), f->xmin(), o->ymin(), o->ymax() ) );
            }
            if( o->xmax() > f->xmax() ) 
            {
                possibleLES[1].push_back( getRectangle( f->xmax(), o->xmax(), o->ymin(), o->ymax() ) );
            }
            if( o->ymin() < f->ymin() ) 
            {
                possibleLES[2].push_back( getRectangle( o->xmin(), o->xmax(), o->ymin(), f->ymin() ) );
            }
            if( o->ymax() > f->ymax() ) 
            {
                possibleLES[3].push_back( getRectangle( o->xmin(), o->xmax(), f->ymax(), o->ymax() ) );
            }

            // We need to remove the current rectangle from the free space list since 
            //is about to get cut into pieces.
            didDelete = false;
            for( j = _freeSpaceRectangles.size(); j > 0; ) 
            {
                --j;
                if( _freeSpaceRectangles[j] == o ) 
                {
                    if( j < _freeSpaceRectangles.size() - 1 ) 
                    {
                        std::swap( _freeSpaceRectangles[j], _freeSpaceRectangles[_freeSpaceRectangles.size() - 1] );
                    }
                    _freeSpaceRectangles.pop_back();
                    didDelete = true;
                    break;
                }
            }
            if( !didDelete )
            {
                std::cout << "Error." << std::endl;
                exit(-1);
            }
            delRectangle( o );
        }
    }

    for( direction = 0; direction < 4; direction++ ) 
    {
        for( i = 0; i < possibleLES[direction].size(); i++ ) 
        {
            o = possibleLES[direction][i];

            contained = false;

            // The following code determines containment.  Hence,
            // once we see containment, we can immediately stop
            // looking.
            for( j = 0; j < possibleLES[direction].size() && !contained; j++ ) 
            {
                tmp = possibleLES[direction][j];
                if( o != tmp && tmp->contains( *o ) ) 
                {
                    contained = true;
                }
            }

            for( j = 0; j < adjacentLES[direction].size() && !contained; j++ ) 
            {
                tmp = adjacentLES[direction][j];
                if( o != tmp && tmp->contains( *o ) ) 
                {
                    contained = true;
                }
            }

            // If not contained, then it is a free rectangle so we
            // save it.  Otherwise, we can toss the rectangle (done
            // later to avoid messing up the array).
            if( !contained ) 
            {
                _freeSpaceRectangles.push_back( getRectangle( o->xmin(), o->xmax(), o->ymin(), o->ymax() ) );
            }
        }

        for( i = 0; i < possibleLES[direction].size(); i++ ) 
        {
            delRectangle( possibleLES[direction][i] );
        }
        possibleLES[direction].clear();
    }
    return f;
}

void SpaceManager::RemFullRectangle( Rectangle* ptr )
{
    // XXX: Not strictly the correct way to remove a full space rectangle.  This is because
    // releasing this rectangle might mean some other rectangles could be larger...
    for( size_t i = _fullSpaceRectangles.size(); i > 0; ) 
    {
        --i;
        if( _fullSpaceRectangles[i] == ptr ) 
        {
            if( i < _fullSpaceRectangles.size() - 1 ) 
            {
                std::swap( _fullSpaceRectangles[i], _fullSpaceRectangles[ _fullSpaceRectangles.size() - 1] );
            }
            _fullSpaceRectangles.pop_back();
            _freeSpaceRectangles.push_back( ptr );
            return;
        }
    }
}

void SpaceManager::GetRectanglesFit( double xmin, double xmax, double ymin, double ymax, 
    std::vector<Rectangle*>& possibleLEs, double radius )
{
    // Get the available rectangles into which the specified rectangle will fit.
    int             i;
    double          xwid = xmax-xmin;
    double          ywid = ymax-ymin;
    Rectangle*      ptri;
    double          distX, distY;
    double          w, h;

    possibleLEs.clear();
    possibleLEs.reserve( _freeSpaceRectangles.size() );

    for( i = 0; i < _freeSpaceRectangles.size(); i++ ) 
    {
        ptri = _freeSpaceRectangles[i];

        w = ptri->xmax()-ptri->xmin();
        h = ptri->ymax()-ptri->ymin();

        //if( w*h < xwid*ywid ) continue;
        if( w < xwid ) continue; // too narrow.
        if( h < ywid ) continue; // too high.

        // Determine amount of shift required to get
        // into the whitespace; i.e., the distance.
        distX = 0.;
        if     ( xmin < ptri->xmin() ) distX = ptri->xmin()-xmin;
        else if( xmax > ptri->xmax() ) distX = xmax-ptri->xmax();
        distY = 0.;
        if     ( ymin < ptri->ymin() ) distY = ptri->ymin()-ymin;
        else if( ymax > ptri->ymax() ) distY = ymax-ptri->ymax();

        double dist = std::sqrt( distX*distX + distY*distY );
        if( dist > radius )
            continue;
        //ptri->_dist = sqrt( distX*distX+distY*distY );
        //if( ptri->_dist > radius ) continue;

        possibleLEs.push_back( ptri );
    }
    // XXX: Don't bother sorting.
}

void SpaceManager::Reset( double xmin, double xmax, double ymin, double ymax )
{
    cleanup1();
    _freeSpaceRectangles.push_back( getRectangle( xmin, xmax, ymin, ymax ) );
}

void SpaceManager::single( double xmin, double xmax, double ymin, double ymax, Architecture* arch )
{
    // Uses rectangle for each row rather than for the entire space.
    cleanup1();

    _freeSpaceRectangles.reserve( arch->m_rows.size() );
    for( size_t r = 0; r < arch->m_rows.size(); r++ )
    {
        double yb = arch->m_rows[r]->getY();
        double yt = yb + arch->m_rows[r]->getH();
        if( !(yb >= ymin-1.0e-3 && yt <= ymax+1.0e-3) )
        {
            continue;
        }

        _freeSpaceRectangles.push_back( getRectangle( xmin, xmax, yb, yt ) );
    }
}

bool SpaceManager::GetLargestFreeSpace( double& xmin, double& xmax, double& ymin, double& ymax )
{
    // Finds the largest rectangle of free space.  Returns false if none.
    Rectangle           *ptri = NULL, *ptrj = NULL;
    double              area_i = 0., area_j = 0.;
    int                 i;

    if( _freeSpaceRectangles.empty() ) 
    {
        return false;
    }

    for( i = 0; i < _freeSpaceRectangles.size(); i++ ) 
    {
        ptrj   = _freeSpaceRectangles[i];
        area_j = (ptrj->xmax()-ptrj->xmin())*(ptrj->ymax()-ptrj->ymin());
        if( ptri == NULL || area_j > area_i ) 
        {
            ptri   = ptrj;
            area_i = area_j;
        }
    }
    if( ptri == NULL ) 
    {
        return false;
    }
    xmin = ptri->xmin();
    xmax = ptri->xmax();
    ymin = ptri->ymin();
    ymax = ptri->ymax();

    return true;
}

void SpaceManager::cleanup0( void )
{
    // Clears everything.
    for( size_t i = 0; i < _freeSpaceRectangles.size(); i++ ) 
    {
        delete _freeSpaceRectangles[i];
    }
    for( size_t i = 0; i < _fullSpaceRectangles.size(); i++ ) 
    {
        delete _fullSpaceRectangles[i];
    }
    for( size_t i = 0; i < _availableRectangles.size(); i++ ) 
    {
        delete _availableRectangles[i];
    }
    _freeSpaceRectangles.clear();
    _fullSpaceRectangles.clear();
    _availableRectangles.clear();
}

void SpaceManager::cleanup1( void )
{
    // Clears everything; justs saves the rectangles to avoid reallocation.
    for( size_t i = 0; i < _freeSpaceRectangles.size(); i++ ) 
    {
        _availableRectangles.push_back( _freeSpaceRectangles[i] );
    }
    for( size_t i = 0; i < _fullSpaceRectangles.size(); i++ ) 
    {
        _availableRectangles.push_back( _fullSpaceRectangles[i] );
    }
    _freeSpaceRectangles.clear();
    _fullSpaceRectangles.clear();
}

void SpaceManager::determineNeighbors( Rectangle* f, std::vector<Rectangle*>& neighbors )
{
    // Determine existing rectangles in the free list that are adjacent to f or
    // intersect f.
    Rectangle           *other;
    double              xx, yy;
    double              ww, hh;

    neighbors.clear(); 
    neighbors.reserve( _freeSpaceRectangles.size() );

    for( size_t i = 0; i < _freeSpaceRectangles.size(); i++ ) 
    {
        other = _freeSpaceRectangles[i];

        xx = std::max( f->xmin(), other->xmin() );
        yy = std::max( f->ymin(), other->ymin() );
        ww = std::min( f->xmax(), other->xmax() ) - xx;
        hh = std::min( f->ymax(), other->ymax() ) - yy;

        if( ww >= 0. && hh >= 0. )
        {
            neighbors.push_back( other );
        }
    }
}

void SpaceManager::delRectangle( Rectangle*& rect )
{
    _availableRectangles.push_back( rect );
    rect = NULL;
}

Rectangle* SpaceManager::getRectangle( double xmin, double xmax, double ymin, double ymax )
{
    Rectangle       *ptr = NULL;
    if( !_availableRectangles.empty() ) 
    {
        ptr = _availableRectangles.front();
        _availableRectangles.pop_front();
        ptr->reset();
        ptr->addPt( xmin, ymin );
        ptr->addPt( xmax, ymax );
    } 
    else 
    {
        ptr = new Rectangle( xmin, ymin, xmax, ymax );
    }
    return ptr;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpaceManager::GetClosestRectanglesLarge( bool onlyNeedOne,
        double xmin, double xmax, double ymin, double ymax,
        std::vector<Rectangle*>& possibleLES, 
        double radius )
{
    // Gathers rectangles within a radius capable of holding the cell.  If 
    //'onlyNeedOne' is true, then we only return a single rectangle.  Also,
    // the rectangles _must_ be within a certain radius/distance of the 
    // provided rectangle.

    const double            eps = 1e-3;

    double              bestDist = std::numeric_limits<double>::max();
    Rectangle           *bestPtr = NULL;
    double              xwid = xmax-xmin;
    double              ywid = ymax-ymin;
    Rectangle           *ptri;
    double              distX, distY, dist;

    possibleLES.clear();

    if( onlyNeedOne ) 
    {
        possibleLES.reserve( 1 );

        for( size_t i = 0; i < _freeSpaceRectangles.size(); i++ ) 
        {
            ptri = _freeSpaceRectangles[i];
            if( ptri->xmax() - ptri->xmin() < xwid-eps )
            {
                continue;
            }
            if( ptri->ymax() - ptri->ymin() < ywid-eps )
            {
                continue;
            }

            // Determine amount of shift required to get
            // into the whitespace; i.e., the distance.
            distX = 0.;
            if     ( xmin <= ptri->xmin()-eps ) distX = ptri->xmin()-xmin;
            else if( xmax >= ptri->xmax()+eps ) distX = xmax-ptri->xmax();
            distY = 0.;
            if     ( ymin <= ptri->ymin()-eps ) distY = ptri->ymin()-ymin;
            else if( ymax >= ptri->ymax()+eps ) distY = ymax-ptri->ymax();

            dist = std::sqrt( distX*distX+distY*distY );
            if( dist < bestDist ) 
            {
                bestDist = dist;
                bestPtr = ptri;
            }
        }
        if( bestPtr != 0 )
        {
            possibleLES.push_back( bestPtr );
        }
    } 
    else 
    {
       possibleLES.reserve( _freeSpaceRectangles.size() );

        for( size_t i = 0; i < _freeSpaceRectangles.size(); i++ ) 
        {
            ptri = _freeSpaceRectangles[i];
            if( ptri->xmax() - ptri->xmin() < xwid-eps ) 
            {
                continue;
            }
            if( ptri->ymax() - ptri->ymin() < ywid-eps )
            {
                continue;
            }

            // Determine amount of shift required to get
            // into the whitespace; i.e., the distance.
            distX = 0.;
            if     ( xmin <= ptri->xmin()-eps ) distX = ptri->xmin()-xmin;
            else if( xmax >= ptri->xmax()+eps ) distX = xmax-ptri->xmax();
            distY = 0.;
            if     ( ymin <= ptri->ymin()-eps ) distY = ptri->ymin()-ymin;
            else if( ymax >= ptri->ymax()+eps ) distY = ymax-ptri->ymax();

            dist = std::sqrt( distX*distX+distY*distY );
            if( dist > radius )
            {
                continue;
            }

            possibleLES.push_back( ptri );
        }
        // Sort the rectangles so the closest ones are first.
        RectangleSorter2 sorter( xmin, xmax, ymin, ymax );
        std::sort( possibleLES.begin(), possibleLES.end(), sorter );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpaceManager::GetClosestRectanglesLarge( bool onlyNeedOne,
        double xmin, double xmax, double ymin, double ymax,
        std::vector<Rectangle*>& possibleLES, 
        Architecture* arch, Node* ndi,
        double radius )
{
    // Gathers rectangles within the specified range capable of holding
    // the cell.  Takes into account row alignment/power requirements.

    const double            eps = 1e-3;

    double              bestDist = std::numeric_limits<double>::max();
    Rectangle           *bestPtr = NULL;
    double              xwid = xmax-xmin;
    double              ywid = ymax-ymin;
    Rectangle           *ptri;
    double              distX, distY, dist;

    possibleLES.clear();

    int rowHeight = arch->m_rows[0]->getH();
    int spanned = (int)(ndi->getHeight() / rowHeight + 0.5);
    for( size_t i = 0; i < _freeSpaceRectangles.size(); i++ )
    {
        ptri = _freeSpaceRectangles[i];
        if( ptri->xmax() - ptri->xmin() < xwid-eps )
        {
            // No fit.
            continue;
        }
        if( ptri->ymax() - ptri->ymin() < ywid-eps )
        {
            // No fit.
            continue;
        }

        // Determine amount of shift required to get into the whitespace.
        distX = 0.;
        if     ( xmin <= ptri->xmin()-eps ) distX = ptri->xmin()-xmin;
        else if( xmax >= ptri->xmax()+eps ) distX = xmax-ptri->xmax();
        distY = 0.;
        if     ( ymin <= ptri->ymin()-eps ) distY = ptri->ymin()-ymin;
        else if( ymax >= ptri->ymax()+eps ) distY = ymax-ptri->ymax();

        dist = std::sqrt( distX*distX+distY*distY );

        if( dist > radius )
        {
            // Too far.
            continue;
        }

        // Row and power.
        int r = arch->find_closest_row( ptri->ymin() );
        if( r > 0 && arch->m_rows[r]->getY() >= ptri->ymin()-1.0e-3 )
        {
            --r;
        }
        bool compat = false;
        while( r < arch->m_rows.size() && arch->m_rows[r]->getY()+ndi->getHeight() <= ptri->ymax()+1.0e-3 )
        {
            if( arch->m_rows[r]->getY() >= ptri->ymin()-1.0e-3 )
            {
                bool flip = false;
                if( arch->power_compatible( ndi, arch->m_rows[r], flip ) )
                {
                    compat = true;
                    break;
                }
            }
            ++r;
        }
        if( !compat )
        {
            // No alignment.
            continue;
        }

        possibleLES.push_back( ptri );
    }
    RectangleSorter2 sorter( xmin, xmax, ymin, ymax );
    std::sort( possibleLES.begin(), possibleLES.end(), sorter );
    if( onlyNeedOne )
    {
        possibleLES.erase( possibleLES.begin()+1, possibleLES.end() );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpaceManager::GetClosestRectanglesRange( bool onlyNeedOne,
        double xmin, double xmax, double ymin, double ymax,
        std::vector<Rectangle*>& possibleLES, double radius )
{
    // Get all rectangles in range.  Sort by amount of possible overlap.  If
    // 'onlyNeedOne' is true, then we return the single, lowest-overlapping
    // rectangle capable of holding the cell. 
    double              bestDist = -std::numeric_limits<double>::max();
    Rectangle           *bestPtr = NULL;
    Rectangle           *ptri;
    double              distX, distY;
    double              minW, minH;

    possibleLES.clear();

    if( onlyNeedOne ) 
    {
        possibleLES.reserve( 1 );

        for( size_t i = 0; i < _freeSpaceRectangles.size(); i++ ) 
        {
            ptri = _freeSpaceRectangles[i];

            distX = 0.;
            if     ( xmin < ptri->xmin() ) distX = ptri->xmin() - xmin;
            else if( xmax > ptri->xmax() ) distX = xmax - ptri->xmax();
            distY = 0.;
            if     ( ymin < ptri->ymin() ) distY = ptri->ymin() - ymin;
            else if( ymax > ptri->ymax() ) distY = ymax - ptri->ymax();

            double dist =  std::sqrt( distX*distX+distY*distY );
            if( dist > radius )
            {
                continue;
            }

            // Assume rectangles centered and compute overlap.  Not really
            // sure about this measure of overlap...
            minW = std::min( (xmax - xmin), (ptri->xmax() - ptri->xmin()) );
            minH = std::min( (ymax - ymin), (ptri->ymax() - ptri->ymin()) );

            double overlap = (minW * minH)/((xmax-xmin)*(ymax-ymin));

            if( overlap > bestDist ) 
            {
                bestDist = overlap;
                bestPtr = ptri;
            }
        }
        if( bestPtr != NULL ) 
        {
            possibleLES.push_back( bestPtr );
        }
   } 
   else 
   {
        possibleLES.reserve( _freeSpaceRectangles.size() );

        for( size_t i = 0; i < _freeSpaceRectangles.size(); i++ ) 
        {
            ptri = _freeSpaceRectangles[i];

            distX = 0.;
            if     ( xmin < ptri->xmin() ) distX = ptri->xmin() - xmin;
            else if( xmax > ptri->xmax() ) distX = xmax - ptri->xmax();
            distY = 0.;
            if     ( ymin < ptri->ymin() ) distY = ptri->ymin() - ymin;
            else if( ymax > ptri->ymax() ) distY = ymax - ptri->ymax();

            double dist = std::sqrt( distX*distX+distY*distY );
            if( dist > radius )
            {
                continue;
            }

            // Assume rectangles centered and compute overlap.
            minW = std::min( (xmax - xmin), (ptri->xmax() - ptri->xmin()) );
            minH = std::min( (ymax - ymin), (ptri->ymax() - ptri->ymin()) );

            double overlap = (minW * minH)/((xmax-xmin)*(ymax-ymin));

            possibleLES.push_back( ptri );
        }
        // Sort and then reverse so the largest overlap is first.
        RectangleSorter3 sorter( xmin, xmax, ymin, ymax );
        std::sort( possibleLES.begin(), possibleLES.end(), sorter );
        std::reverse( possibleLES.begin(), possibleLES.end() );
    }
}

} // namespace aak
