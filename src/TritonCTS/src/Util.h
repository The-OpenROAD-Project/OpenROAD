////////////////////////////////////////////////////////////////////////////////////
// Authors: Mateus Fogaca
//          (Ph.D. advisor: Ricardo Reis)
//          Jiajia Li
//          Andrew Kahng
// Based on:
//          K. Han, A. B. Kahng and J. Li, "Optimal Generalized H-Tree Topology and 
//          Buffering for High-Performance and Low-Power Clock Distribution", 
//          IEEE Trans. on CAD (2018), doi:10.1109/TCAD.2018.2889756.
//
//
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////

#ifndef CTS_UTIL_H
#define CTS_UTIL_H

#include <ostream> 

namespace TritonCTS {

typedef long long DBU;

template<class T>
class Point {
public:
        Point(T x, T y) : _x(x), _y(y) {}

        T getX() const { return _x; }
        T getY() const { return _y; }

        T computeDist(const Point<T>& other) const {
                T dx = (getX() > other.getX()) ? (getX() - other.getX()) : (other.getX() - getX());
                T dy = (getY() > other.getY()) ? (getY() - other.getY()) : (other.getY() - getY());
                
                return dx + dy; 
        }
       
        T computeDistX(const Point<T>& other) const {
                T dx = (getX() > other.getX()) ? (getX() - other.getX()) : (other.getX() - getX());
                return dx; 
        }

        T computeDistY(const Point<T>& other) const {
                T dy = (getY() > other.getY()) ? (getY() - other.getY()) : (other.getY() - getY());
                return dy;
        }

        bool operator < (const Point<T>& other) const {
                if (getX() != other.getX()) {
                        return getX() < other.getX();
                } else { 
                        return getY() < other.getY();
                } 
        }

        friend std::ostream& operator << (std::ostream& out, const Point<T>& point) {
                out << "[(" << point.getX() << ", " << point.getY() << ")]";
                return out;
        }

private: 
        T _x;
        T _y;
};

template <class T>
class Box {        
public:
        T getMinX() const { return _xMin; }
        T getMinY() const { return _yMin; }
        T getMaxX() const { return _xMin + _width; }
        T getMaxY() const { return _yMin + _height; }
        T getWidth() const { return _width; }
        T getHeight() const { return _height; }

        Point<T> computeCenter() const {
                return Point<T>(_xMin + _width / 2, _yMin + _height / 2);
        }

        Box<double> normalize(double factor) { 
                return Box<double>( getMinX() * factor, getMinY() * factor,
                                    getMaxX() * factor, getMaxY() * factor); 
        } 
        
        Box() : _xMin(0), _yMin(0), _width(0), _height(0) {}
        Box(T xMin, T yMin, T xMax, T yMax) : 
                _xMin(xMin), _yMin(yMin), _width(xMax-xMin), _height(yMax-yMin) {}

        friend std::ostream& operator << (std::ostream& out, const Box& box) {
                out << "[(" << box.getMinX() << ", " << box.getMinY() << "), (" 
                    << box.getMaxX() << ", " << box.getMaxY() << ")]";
                return out;
        }
        
protected:
        T _xMin;
        T _yMin;
        T _width;
        T _height;
};

}

#endif
