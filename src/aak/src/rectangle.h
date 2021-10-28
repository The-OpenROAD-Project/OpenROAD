
////////////////////////////////////////////////////////////////////////////////
// File: rectangle.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <limits>
#include <cmath>
#include <algorithm>


namespace aak
{

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////

class Rectangle
{
public:
    Rectangle()
    {
        m_xmin =  std::numeric_limits<double>::max();
        m_ymin =  std::numeric_limits<double>::max();
        m_xmax = -std::numeric_limits<double>::max();
        m_ymax = -std::numeric_limits<double>::max();
    }
    Rectangle( double xmin, double ymin, double xmax, double ymax ): 
        m_xmin(xmin),m_ymin(ymin),
        m_xmax(xmax),m_ymax(ymax)
    {}
    Rectangle( const Rectangle& rect ):
        m_xmin(rect.m_xmin),m_ymin(rect.m_ymin),
        m_xmax(rect.m_xmax),m_ymax(rect.m_ymax)
    {}
    Rectangle& operator=(const Rectangle& other )
    {
        if( this != &other ) 
        {
            m_xmin = other.m_xmin;
            m_xmax = other.m_xmax;
            m_ymin = other.m_ymin;
            m_ymax = other.m_ymax;
        }
        return *this;
    }
    virtual ~Rectangle()
    {}

    void reset( void )
    {
        m_xmin =  std::numeric_limits<double>::max();
        m_ymin =  std::numeric_limits<double>::max();
        m_xmax = -std::numeric_limits<double>::max();
        m_ymax = -std::numeric_limits<double>::max();
    }

    void enlarge(const Rectangle &r)
    {
        m_xmin = ( m_xmin > r.m_xmin ? r.m_xmin : m_xmin);
        m_ymin = ( m_ymin > r.m_ymin ? r.m_ymin : m_ymin);
        m_xmax = ( m_xmax < r.m_xmax ? r.m_xmax : m_xmax);
        m_ymax = ( m_ymax < r.m_ymax ? r.m_ymax : m_ymax);
    }
    bool intersects( const Rectangle &r) const 
    { 
        return !( m_xmin > r.m_xmax || m_xmax < r.m_xmin || m_ymin > r.m_ymax || m_ymax < r.m_ymin ); 
    }
    bool is_overlap( double xmin, double ymin, double xmax, double ymax )
    {
        if( xmin >= m_xmax ) return false;
        if( xmax <= m_xmin ) return false;
        if( ymin >= m_ymax ) return false;
        if( ymax <= m_ymin ) return false;
        return true;
    }

    bool contains( const Rectangle& r )
    {
        if( r.m_xmin >= m_xmin &&
            r.m_xmax <= m_xmax &&
            r.m_ymin >= m_ymin &&
            r.m_ymax <= m_ymax ) 
        {
            return true;
        }
        return false;
    }



    void addPt( double x, double y )
    {
        m_xmin = std::min( m_xmin, x );
        m_xmax = std::max( m_xmax, x );
        m_ymin = std::min( m_ymin, y );
        m_ymax = std::max( m_ymax, y );
    }
    double getCenterX() { return 0.5 * (m_xmax + m_xmin); }
    double getCenterY() { return 0.5 * (m_ymax + m_ymin); }
    double getWidth() { return m_xmax - m_xmin; }
    double getHeight() { return m_ymax - m_ymin; }
    void clear()
    {
        m_xmin =  std::numeric_limits<double>::max();
        m_ymin =  std::numeric_limits<double>::max();
        m_xmax = -std::numeric_limits<double>::max();
        m_ymax = -std::numeric_limits<double>::max();
    }

    double xmin() const { return m_xmin; }
    double xmax() const { return m_xmax; }
    double ymin() const { return m_ymin; }
    double ymax() const { return m_ymax; }

    double area() const { return (m_xmax-m_xmin)*(m_ymax-m_ymin); }

public: 
    double m_xmin;
    double m_xmax;
    double m_ymin;
    double m_ymax;
};

} // namespace aak
