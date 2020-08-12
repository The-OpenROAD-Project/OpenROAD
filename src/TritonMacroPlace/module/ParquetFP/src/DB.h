/**************************************************************************
***    
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
***               Saurabh N. Adya, Hayward Chan, Jarrod A. Roy
***               and Igor L. Markov
***
***  Contact author(s): sadya@umich.edu, imarkov@umich.edu
***  Original Affiliation:   University of Michigan, EECS Dept.
***                          Ann Arbor, MI 48109-2122 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining 
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation 
***  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
***  and/or sell copies of the Software, and to permit persons to whom the 
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/


#ifndef DB_H
#define DB_H

#include "ABKCommon/abkcommon.h"
#include "Nets.h"
#include "Nodes.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace parquetfp
{
   class DB
   {
       protected:
           Nodes* _nodes;
           Nets* _nets;

           Nodes* _nodesBestCopy;

           Nodes* _obstacles; // <aaronnn> 
           float _obstacleFrame[2]; // width, height

           // cached copies of sum(area of all nodes)
           mutable float _area;
           mutable bool _initArea;

           float _rowHeight;   //rowHeight to snap the soln to
           float _siteSpacing; //site spacing to snap the soln to

           BBox termBBox;
           void buildTermBBox(void);

           std::vector<Point> _scaledLocs;
           void scaleTerminals(void);

           inline float getScaledX(const Node& node) const
           {
             return _scaledLocs[node.getIndex()].x;
           }

           inline float getScaledY(const Node& node) const
           {
             return _scaledLocs[node.getIndex()].y;
           }

       public:
           bool successAR;
           DB(const std::string &baseName);
	   DB(DB* db, std::vector<int>& subBlocksIndices, Point& dbLoc, float reqdAR);
           DB();

           //makes a copy of db2. compresses multiple 2-pin nets
           //between modules regardless of pin-offset. use only when pin-offsets
           //are inconsequential
           DB(DB& db2, bool compressDB=false);

           virtual ~DB();

           DB& operator=(DB& db2);
           void clean(void);

           unsigned getNumNodes(void) const;
           Nodes* getNodes(void);
           Nets* getNets(void);

           Nodes* getObstacles(void);
           float *getObstacleFrame() { return _obstacleFrame; }
           unsigned getNumObstacles(void) const;
           void addObstacles(Nodes *obstacles, float obstacleFrame[2]);

           std::vector<float> getNodeWidths() const;
           std::vector<float> getNodeHeights() const;
           std::vector<float> getXLocs() const;
           std::vector<float> getYLocs() const;

           // hhchan: more efficient (safer) getters 
           inline float getNodeArea(unsigned index) const;
           inline float getNodeWidth(unsigned index) const;
           inline float getNodeHeight(unsigned index) const;
           inline float getXLoc(unsigned index) const;
           inline float getYLoc(unsigned index) const;
           inline ORIENT getOrient(unsigned index) const;

           inline float getNodeMaxAR(unsigned index) const;
           inline float getNodeMinAR(unsigned index) const;
           inline float getNodeAR(unsigned index) const;

           inline bool isMacro(unsigned index) const;
           inline bool isOrientFixed(unsigned index) const;
           inline bool isNodeSoft(unsigned index) const;
           inline bool isNodeHard(unsigned index) const;

           // hhchan: safer setters
           inline void setNodeWidth(unsigned index, float width);
           inline void setNodeHeight(unsigned index, float height);
           inline void setNodeAR(unsigned index, float ar);

           inline void setXLoc(unsigned index, float newX);
           inline void setYLoc(unsigned index, float newY);
           inline void setOrient(unsigned index,
                   ORIENT newOrient, bool ignoreNets = false);
           inline void setOrient(unsigned index,
                   const char* newOrient, bool ignoreNets = false);

           Point getBottomLeftCorner() const;
           Point getTopRightCorner() const;

           // pack to one of the corners
           // - use only interface fcns of SPeval, hence not efficient at all
           // - not meant to used in the critical loop
           // WARNING: assume the vectors of of size [NUM_CORNERS][getNumNodes()]
           enum Corner {BOTTOM, TOP, LEFT, RIGHT,
                        BOTTOM_LEFT, BOTTOM_RIGHT, TOP_LEFT, TOP_RIGHT,
                        LEFT_BOTTOM, RIGHT_BOTTOM, LEFT_TOP, RIGHT_TOP, NUM_CORNERS};
           inline static std::string toString(Corner corner);
           void packToCorner(std::vector< std::vector<float> >& xlocsAt,
                   std::vector< std::vector<float> >& ylocsAt) const;

           // optimize HPWL by the corner
#ifdef USEFLUTE
           void cornerOptimizeDesign(bool scaleTerms, bool minWL, bool useSteiner);
#else
           void cornerOptimizeDesign(bool scaleTerms, bool minWL);
#endif

           // get total area of ALL nodes
           float getNodesArea(void) const;
           float getRowHeight(void) const;
           float getSiteSpacing(void) const;
           void setRowHeight(float rowHeight);
           void setSiteSpacing(float siteSpacing);

           // slim:       xloc, yloc, orient, width, height (but not the orig's)
           // location:   xloc, yloc
           // dimensions: width, height (but not the orig's)
           // TRUE ~ succeed in updating, FALSE otherwise
           inline bool updateNodeSlim(int index, const Node& newNode);
           inline bool updateNodeLocation(int index,
                   float xloc, float yloc);
           inline bool updateNodeDimensions(int index,
                   float width, float height);

           void updatePlacement(const std::vector<float>& xloc,
                   const std::vector<float>& yloc);
           void initPlacement(const Point& loc);      
           void updateSlacks(const std::vector<float>& xSlack,
                   const std::vector<float>& ySlack);

           // plot in gnuplot format
           void plot(const char* fileName, float area, float whitespace, float aspectRatio,
                   float time, float HPWL,
                   bool plotSlacks, bool plotNets, bool plotNames,
                   bool fixedOutline = false, 
                   float lx =FLT_MAX, float ly = FLT_MAX, 
                   float ux = FLT_MIN, float uy = FLT_MIN) const;

           // save the data in Capo format
           void saveCapo(const char* baseFileName, const BBox &, float reqdAR=1, float reqdWS=30) const;  
           void saveCapoNets(const char* baseFileName) const;

           // save data in floorplan format
           void save(const char* baseFileName) const; 
           void saveNets(const char* baseFileName) const;
           void saveWts(const char* baseFileName) const;

           void saveBestCopyPl(char* baseFileName) const;
           void saveInBestCopy(void); // non-const since it modifies "successAR"

#ifdef USEFLUTE
           float evalHPWL(bool useWts, bool scaleTerms, bool useSteiner);  //assumes that placement is updated
           float evalSteiner(bool useWts, bool scaleTerms);
#else
           float evalHPWL(bool useWts, bool scaleTerms);  //assumes that placement is updated
#endif
           float evalArea(void) const;  //assumes that placement is updated
           float getXSize(void) const;
           float getYSize(void) const;
           float getAvgHeight(void) const;

           // optimize the BL-corner of the design
#ifdef USEFLUTE
           void shiftOptimizeDesign(float outlineWidth, float outlineHeight,
                   bool scaleTerms, bool useSteiner, Verbosity verb); // deduce BL-corner from DB
           void shiftOptimizeDesign(const Point& bottomLeft, const Point& topRight,
                   bool scaleTerms, bool useSteiner, Verbosity verb); 
#else
           void shiftOptimizeDesign(float outlineWidth,
                   float outlineHeight, bool scaleTerms, Verbosity verb); // deduce BL-corner from DB
           void shiftOptimizeDesign(const Point& bottomLeft,
                   const Point& topRight, bool scaleTerms, Verbosity verb);
#endif

           void shiftDesign(const Point& offset); //shift the entire placement by an offset
           void shiftTerminals(const Point& offset);

           void expandDesign(float maxWidth, float maxHeight);
           //expand entire design to occupy the new dimensions. only locations of blocks
           //are altered

           //marks all nodes with height > avgHeight as macros
           void markTallNodesAsMacros(float maxHeight);

           //reduce the core cells area of design excluding macros to maxWS whitespace
           //assumes macros are marked using DB::markTallNodesAsMacros()
           void reduceCoreCellsArea(float layoutArea, float maxWS);

           //this function gets the dimension of the FP only considering the macros
           float getXSizeWMacroOnly();
           float getYSizeWMacroOnly();

           float getXMax();
           float getYMax();

           float getXMaxWMacroOnly();
           float getYMaxWMacroOnly();

           void resizeHardBlocks(float ratio);

       private:
           // help function of shiftOptimizeDesign()
           float getOptimalRangeStart(bool isHorizontal);
           void _setUpPinOffsets(void) const;
   };

   // ---------------
   // IMPLEMENTATIONS
   // ---------------
   float DB::getNodeArea(unsigned index) const
   {   return getNodeWidth(index) * getNodeHeight(index); }
   
   float DB::getNodeWidth(unsigned index) const
   {   return _nodes->getNodeWidth(index); }

   float DB::getNodeHeight(unsigned index) const
   {   return _nodes->getNodeHeight(index); }

   float DB::getXLoc(unsigned index) const
   {   return (_nodes->getNode(index)).getX(); }

   float DB::getYLoc(unsigned index) const
   {   return (_nodes->getNode(index)).getY(); }

   ORIENT DB::getOrient(unsigned index) const
   {   return (_nodes->getNode(index)).getOrient(); }

   float DB::getNodeMinAR(unsigned index) const
   {   return (_nodes->getNode(index)).getminAR(); }

   float DB::getNodeMaxAR(unsigned index) const
   {   return (_nodes->getNode(index)).getmaxAR(); }

   float DB::getNodeAR(unsigned index) const
   {   return getNodeWidth(index) / getNodeHeight(index); }
   
   bool DB::isMacro(unsigned index) const
   {   return (_nodes->getNode(index)).isMacro(); }

   bool DB::isOrientFixed(unsigned index) const
   {   return (_nodes->getNode(index)).isOrientFixed(); }

   bool DB::isNodeSoft(unsigned index) const
   {   return getNodeMaxAR(index) - getNodeMinAR(index) > 1e-6; }

   bool DB::isNodeHard(unsigned index) const
   {   return !isNodeSoft(index); }
   // -----------------------------------------------------
   void DB::setNodeWidth(unsigned index, float width)
   {   (_nodes->getNode(index)).putWidth(width); }
   
   void DB::setNodeHeight(unsigned index, float height)
   {   (_nodes->getNode(index)).putHeight(height); }

   void DB::setNodeAR(unsigned index, float ar)
   {
      const float area = getNodeArea(index);
      setNodeWidth(index, sqrt(area * ar));
      setNodeHeight(index, sqrt(area / ar));
   }
   
   void DB::setXLoc(unsigned index, float newX)
   {   (_nodes->getNode(index)).putX(newX); }
   
   void DB::setYLoc(unsigned index, float newY)
   {   (_nodes->getNode(index)).putY(newY); }
   
   void DB::setOrient(unsigned index, ORIENT newOrient, bool ignoreNets)
   {
      if (ignoreNets)
         (_nodes->getNode(index)).putOrient(newOrient);
      else      
         _nodes->changeOrient(index, newOrient, *_nets);
   }
   
   void DB::setOrient(unsigned index, const char *newOrient, bool ignoreNets)
   {
      setOrient(index,
                toOrient(const_cast< char* >(newOrient)), ignoreNets);
   }
   // -----------------------------------------------------
   bool DB::updateNodeSlim(int index,
                           const Node& newNode)
   {
      if (index < int(_nodes->getNumNodes()))
      {
         Node& oldNode = _nodes->getNode(index);
         
         oldNode.putX(newNode.getX());
         oldNode.putY(newNode.getY());
         oldNode.changeOrient(newNode.getOrient(), *_nets);
         oldNode.putWidth(newNode.getWidth());
         oldNode.putHeight(newNode.getHeight());
         return true;
      }
      else
         return false;
   }
   // -----------------------------------------------------
   bool DB::updateNodeLocation(int index,
                               float xloc, float yloc)
   {
      if (index < int(_nodes->getNumNodes()))
      {
         Node& oldNode = _nodes->getNode(index);
         oldNode.putX(xloc);
         oldNode.putY(yloc);
         return true;
      }
      else
         return false;
   }
   // -----------------------------------------------------
   bool DB::updateNodeDimensions(int index,
                                 float width, float height)
   {
      if (index <int( _nodes->getNumNodes()))
      {
         Node& oldNode = _nodes->getNode(index);

         oldNode.putWidth(width);
         oldNode.putHeight(height);
         return true;
      }
      else
         return false;
   }
   // -----------------------------------------------------
   std::string DB::toString(DB::Corner corner)
   {
      switch (corner)
      {
      case BOTTOM: return "bottom";
      case TOP: return "top";
      case LEFT: return "left";
      case RIGHT: return "right";
      case BOTTOM_LEFT: return "bottom-left";
      case BOTTOM_RIGHT: return "bottom-right";
      case TOP_LEFT: return "top-left";
      case TOP_RIGHT: return "top-right";
      case LEFT_BOTTOM: return "left-bottom";
      case RIGHT_BOTTOM: return "right-bottom";
      case LEFT_TOP: return "left-top";
      case RIGHT_TOP: return "right-top";
      default: return "INVALID CORNER";
      }
   }
   // -----------------------------------------------------

}
// using namespace parquetfp;

#endif 
