///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_CIMG_LIB
#ifndef __REPLACE_CIMG_PLOT__
#define __REPLACE_CIMG_PLOT__


#include <vector>
#include <memory>
#include <string>
#include <filesystem>
#include "point.h"
#include "utl/Logger.h"

// 
// The following structure/header will be removed.
//
// This is temporal implementation with CImg
//


namespace cimg_library {
template<typename T1>
class CImg;
}

namespace gpl {

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
  void setLogger(utl::Logger* log);

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

  static void setPlotPath(std::string);
  static bool isPlotEnabled() { return plotPath != nullptr; };

 private:
  std::shared_ptr<PlacerBase> pb_;
  std::shared_ptr<NesterovBase> nb_;
  utl::Logger* log_;

  static std::unique_ptr<std::filesystem::path> plotPath;

  const char* pathArrow = "arrow";
  const char* pathBin   = "bin";
  const char* pathCell  = "cell";

};

}
#endif
#endif
