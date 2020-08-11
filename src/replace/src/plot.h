#ifdef ENABLE_CIMG_LIB
#ifndef __REPLACE_CIMG_PLOT__
#define __REPLACE_CIMG_PLOT__


#include <vector>
#include <memory>
#include "point.h"

// 
// The following structure/header will be removed.
//
// This is temporal implementation with CImg
//


namespace cimg_library {
template<typename T1>
class CImg;
}

namespace replace {

void SaveCellPlotAsJPEG(std::string imgName, bool isGCell, std::string imgPosition);
void SaveBinPlotAsJPEG(std::string imgName, std::string imgPosition);
void SaveArrowPlotAsJPEG(std::string imgName, std::string imgPosition);
void SavePlot(std::string imgName, bool isGCell);
//void ShowPlot(std::string circuitName = "");

class NesterovBase;
class PlacerBase;

enum { BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE };

struct PlotColor {
  private:
  int r_;
  int g_;
  int b_;

  public:
  PlotColor() : r_(0), g_(0), b_(0) {};
  PlotColor(int r, int g, int b) :
    r_(r), g_(g), b_(b) {};

  int r() { return r_; };
  int g() { return g_; };
  int b() { return b_; };
};


typedef cimg_library::CImg< unsigned char > CImgObj;

// for X11 drawing
class PlotEnv {
 public:
  int minLength;
  int imageWidth;
  int imageHeight;
  int xMargin;
  int yMargin;
  float origWidth;
  float origHeight;
  float unitX;
  float unitY;

  // for showPlot
  float dispWidth;
  float dispHeight;

  bool hasCellColor;

  // color information for each cells.
  // Needed for cell coloring
  std::vector<PlotColor> colors;

  // constructor
  PlotEnv();
  PlotEnv(
      std::shared_ptr<PlacerBase> pb, 
      std::shared_ptr<NesterovBase> nb);

  void setPlacerBase(std::shared_ptr<PlacerBase> pb);
  void setNesterovBase(std::shared_ptr<NesterovBase> nb);

  void Init();
  void InitCellColors(std::string colorFile);
  int GetTotalImageWidth();
  int GetTotalImageHeight();
  int GetX(FloatPoint &coord);
  int GetX(float coord);
  int GetY(FloatPoint &coord);
  int GetY(float coord);
  
  void DrawModule(CImgObj *img, const unsigned char color[], float opacity);
  void DrawTerminal(CImgObj* img, 
    const unsigned char termColor[],
    const unsigned char pinColor[], float opacity);
  void DrawBinDensity(CImgObj *img, float opacity);
  void CimgDrawArrow(CImgObj *img, int x1, int y1, int x3, int y3, int thick,
                   const unsigned char color[], float opacity); 
  void DrawGcell(CImgObj *img, const unsigned char fillerColor[],
               const unsigned char cellColor[],
               const unsigned char macroColor[], float opacity); 
  void DrawArrowDensity(CImgObj *img, float opacity);

  void SaveCellPlot(CImgObj *img, bool isGCell);
  void SaveBinPlot(CImgObj *img);
  void SaveArrowPlot(CImgObj *img);
  void SavePlot(std::string imgName, bool isGCell);
  void SaveCellPlotAsJPEG(std::string imgName, bool isGCell, std::string imgPosition);
  void SaveBinPlotAsJPEG(std::string imgName, std::string imgPosition);
  void SaveArrowPlotAsJPEG(std::string imgName, std::string imgPosition);

 private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;



 
};

}
#endif
#endif
