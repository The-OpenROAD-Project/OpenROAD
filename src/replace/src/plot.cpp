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
#include "CImg.h"

#include "plot.h"
#include "placerBase.h"
#include "nesterovBase.h"
#include <unordered_map>



// 
// The following structure/header will be removed.
//
// This is temporal implementation with CImg
//
namespace gpl {

using namespace cimg_library;
using namespace std;
using utl::GPL;

// to save snapshot of the circuit.
static vector< CImg< unsigned char > > imgStor;

// possible color settings
static const unsigned char yellow[] = {255, 255, 0}, white[] = {255, 255, 255},
                           green[] = {0, 255, 0}, blue[] = {120, 200, 255},
                           darkblue[] = {69, 66, 244},
                           purple[] = {255, 100, 255}, black[] = {0, 0, 0},
                           red[] = {255, 0, 0}, pink[] = {255, 192, 203},
                           orange[] = {255, 165, 0};

std::unique_ptr<std::filesystem::path> PlotEnv::plotPath = nullptr;

////////////////////////////////////////////////////////////////////////
//
// below is for the CImg drawing
//

PlotEnv::PlotEnv()
: pb_(nullptr), nb_(nullptr),
  minLength(0), 
  imageWidth(0), imageHeight(0), 
  xMargin(0), yMargin(0), 
  origWidth(0), 
  origHeight(0),
  unitX(0), unitY(0), 
  dispWidth(0), dispHeight(0), 
  hasCellColor(false), log_(nullptr) {}

PlotEnv::PlotEnv(
    std::shared_ptr<PlacerBase> pb,
    std::shared_ptr<NesterovBase> nb) 
: pb_(pb), nb_(nb),
  minLength(1000), 
  imageWidth(0), imageHeight(0), // init later
  xMargin(30), yMargin(30), 
  origWidth(pb_->die().dieUx()-pb_->die().dieLx()), 
  origHeight(pb_->die().dieUy()-pb_->die().dieLy()),
  unitX(0), unitY(0), // init later
  dispWidth(0), dispHeight(0), // init later
  hasCellColor(false), log_(nullptr)
{
  Init();
}

void PlotEnv::Init() {
  minLength = 1000;
  xMargin = yMargin = 30;
  origWidth 
    = pb_->die().dieUx()-pb_->die().dieLx();

  origHeight
    = pb_->die().dieUy()-pb_->die().dieLy();

  hasCellColor = false;

  // imageWidth & height setting
  // Set minimum length of picture as minLength
  if(origWidth < origHeight) {
    imageHeight 
      = 1.0 * origHeight / (origWidth / minLength);
    imageWidth 
      = minLength;
  }
  else {
    imageWidth = 1.0 * origWidth / (origHeight / minLength);
    imageHeight = minLength;
  }
  //        imageWidth = origWidth*0.5;
  //        imageHeight = origHeight*0.5;
  //        imageWidth = origWidth;
  //        imageHeight = origHeight;
  //        imageWidth = origWidth*1.3;
  //        imageHeight = origHeight*1.3;

  // scaling
  unitX = imageWidth / origWidth;
  unitY = imageHeight / origHeight;

  dispWidth = imageWidth * 0.2;
  dispHeight = imageHeight * 0.2;

  // make folders for output
  if (isPlotEnabled()) {
    std::array<std::filesystem::path, 3> paths = {
      *plotPath / pathArrow,
      *plotPath / pathCell,
      *plotPath / pathBin
    };
    for (auto path : paths)
      std::filesystem::create_directories(path.c_str());
  }

//  if( plotColorFile != "" ) {
//    hasCellColor = true;
//    InitCellColors(plotColorFile) nb_ = nb;;
//  }
}

void
PlotEnv::setPlacerBase(std::shared_ptr<PlacerBase> pb){
  pb_ = pb;
}

void
PlotEnv::setNesterovBase(std::shared_ptr<NesterovBase> nb) {
  nb_ = nb;
}

void
PlotEnv::setLogger(utl::Logger* log) {
  log_ = log;
}


// Resize and initialize the vectors
// takes colorSet files
void PlotEnv::InitCellColors(string cFileName) {
//  colors.resize(moduleCNT);

//  std::unordered_map<string, int> colorMap;
//  for(int i=0; i<moduleCNT; i++) {
//    new (&colors[i]) PlotColor();

//    MODULE* curModule = &moduleInstance[i]; 

    // Fill in the colorMap
//    colorMap[curModule ->Name()] = i;
//  }
  
//  std::ifstream cFile(cFileName);
//  if( !cFile.good() ) {
//    cout << "** ERROR : Cannot Open Colorset File : " << cFileName << endl;
//    exit(1);
//  }
  
//  string cellName = "";
//  int r = 0, g = 0, b = 0;

  // keep fill in the cellName/r/g/b variable in the colorset file
//  while( cFile >> cellName >> r >> g >> b ) {
//    auto gPtr = colorMap.find(cellName);
//    if( gPtr == colorMap.end() ) {
//      cout << "ERROR: Cannot find cell : " << cellName << endl;
//    } 

    // save into colors vectors    
//    colors[gPtr->second] = PlotColor(r, g, b);
//  }
}

int PlotEnv::GetTotalImageWidth() {
  return imageWidth + 2 * xMargin;
}

int PlotEnv::GetTotalImageHeight() {
  return imageHeight + 2 * yMargin;
}

int PlotEnv::GetX(FloatPoint &coord) {
  return (coord.x - pb_->die().dieLx()) * unitX + xMargin;
}

int PlotEnv::GetX(float coord) {
  return (coord - pb_->die().dieLx()) * unitX + xMargin;
}

int PlotEnv::GetY(FloatPoint &coord) {
  return (origHeight - (coord.y - pb_->die().dieLy())) * unitY + yMargin;
}

int PlotEnv::GetY(float coord) {
  return (origHeight - (coord - pb_->die().dieLy())) * unitY + yMargin;
}


void 
PlotEnv::DrawTerminal(CImgObj *img, 
    const unsigned char termColor[],
    const unsigned char pinColor[], float opacity) {
  int pinWidth = 300;

  // FIXED CELL
  for(auto& npInst : pb_->nonPlaceInsts()) {
    int x1 = GetX(npInst->lx());
    int y1 = GetY(npInst->ly());
    int x3 = GetX(npInst->ux());
    int y3 = GetY(npInst->uy());
    img->draw_rectangle(x1, y1, x3, y3, termColor, opacity);

//    for(auto& npPin : npInst->pins()) {
//      int x1 = pe.GetX((curTerminal->center.x + curTerminal->pof[j].x) - pinWidth / 2.0);
//      int y1 = pe.GetY((curTerminal->center.y + curTerminal->pof[j].y) - pinWidth / 2.0);
//
//      int x3 = pe.GetX((curTerminal->center.x + curTerminal->pof[j].x) + pinWidth / 2.0);
//      int y3 = pe.GetY((curTerminal->center.y + curTerminal->pof[j].y) + pinWidth / 2.0);
//
//      img.draw_rectangle( x1, y1, x3, y3, pinColor, opacity );
//    }
  }
  // BTerms or ITerms from insts outside the core
  for(auto pin : pb_->pins()) {
    if (pin->isBTerm() && pin->instance() != nullptr) {
      continue;
    }
    int cx = pin->cx();
    int cy = pin->cy();

    int x1 = GetX(cx - pinWidth / 2.0);
    int y1 = GetY(cy - pinWidth / 2.0);

    int x2 = GetX(cx + pinWidth / 2.0);
    int y2 = GetY(cy + pinWidth / 2.0);

    img->draw_rectangle( x1, y1, x2, y2, orange, opacity );
  }
}

void 
PlotEnv::DrawGcell(CImgObj *img, const unsigned char fillerColor[],
               const unsigned char cellColor[],
               const unsigned char macroColor[], float opacity) {
  for(auto& gCell : nb_->gCells()) {

    // skip drawing for FillerCell
//    if(gCell->isFiller()) {
//      continue;
//    }

    const int gcx = gCell->dCx();
    const int gcy = gCell->dCy();

    int x1 = GetX(gcx - gCell->dx()/2);
    int y1 = GetY(gcy - gCell->dy()/2);
    int x3 = GetX(gcx + gCell->dx()/2);
    int y3 = GetY(gcy + gCell->dy()/2);


    // Color settings for Macro / StdCells
    unsigned char color[3] = {0, };
//    if( curGCell->flg == Macro ) {
//      for(int j=0; j<3; j++) {
//        color[j] = macroColor[j];
//      }
//    }
      
    if( hasCellColor ) {
//      color[0] = colors[i].r();
//      color[1] = colors[i].g();
//      color[2] = colors[i].b();
    }
    else {
      if( gCell->isInstance() ) {
        for(int j=0; j<3; j++) {
          color[j] = cellColor[j];
        }
      }
      else if( gCell->isFiller()) {
        for(int j=0; j<3; j++) {
          color[j] = fillerColor[j];
        }
      }
    } 

    if( gCell->isInstance() ) {
      img->draw_rectangle(x1, y1, x3, y3, color, opacity);
    }
//    img->draw_text((x1+x3)/2, (y1+y3)/2, 
//        string(to_string(gCell->dCx()) + " " + to_string(gCell->dCy())).c_str(), black, NULL, 1, 20);
  }
}

// for initPlace
void PlotEnv::DrawModule(CImgObj *img, const unsigned char color[], float opacity) {

  for(auto& inst: pb_->placeInsts()) {
    int x1 = GetX(inst->lx());
    int y1 = GetY(inst->ly());
    int x3 = GetX(inst->ux());
    int y3 = GetY(inst->uy());

    unsigned char cColor[3] = {0, };
//    if( pe.hasCellColor ) {
//      cColor[0] = pe.colors[i].r();
//      cColor[1] = pe.colors[i].g();
//      cColor[2] = pe.colors[i].b();
//    }
//    else {
      for(int j=0; j<3; j++) {
        cColor[j] = color[j];
      }
//    }
    img->draw_rectangle(x1, y1, x3, y3, cColor, opacity);
  }
}

void PlotEnv::DrawBinDensity(CImgObj *img, float opacity) {
  for(auto& bin : nb_->bins()) {
    int x1 = GetX(bin->lx());
    int y1 = GetY(bin->ly());
    int x3 = GetX(bin->ux());
    int y3 = GetY(bin->uy());
    
    int color = bin->density()* 50 + 20;

    color = (color > 255) ? 255 : (color < 20) ? 20 : color;
    color = 255 - color;

    char denColor[3] = {(char)color, (char)color, (char)color};
    img->draw_rectangle(x1, y1, x3, y3, denColor, opacity);
    //        img.draw_text((x1+x3)/2-5, (y1+y3)/2,
    //        to_string(curBin->den).c_str(), black, NULL, 1, 15);
  }
}

void 
PlotEnv::CimgDrawArrow(CImgObj *img, int x1, int y1, int x3, int y3, int thick,
                   const unsigned char color[], float opacity) {
  // ARROW HEAD DRAWING
  float arrowHeadSize = thick;
  float theta = atan(1.0 * (y3 - y1) / (x3 - x1));
  float invTheta = atan(-1.0 * (x3 - x1) / (y3 - y1));

  // ARROW RECT DRAWING
  int ldX = x1 - 1.0 * thick / 4 * cos(invTheta);
  int ldY = y1 - 1.0 * thick / 4 * sin(invTheta);
  int rdX = x1 + 1.0 * thick / 4 * cos(invTheta);
  int rdY = y1 + 1.0 * thick / 4 * sin(invTheta);

  int luX = x3 - 1.0 * thick / 4 * cos(invTheta);
  int luY = y3 - 1.0 * thick / 4 * sin(invTheta);
  int ruX = x3 + 1.0 * thick / 4 * cos(invTheta);
  int ruY = y3 + 1.0 * thick / 4 * sin(invTheta);

  cimg_library::CImg< int > rectPoints(4, 2);
  rectPoints(0, 0) = ldX;
  rectPoints(0, 1) = ldY;
  rectPoints(1, 0) = rdX;
  rectPoints(1, 1) = rdY;
  rectPoints(2, 0) = ruX;
  rectPoints(2, 1) = ruY;
  rectPoints(3, 0) = luX;
  rectPoints(3, 1) = luY;

  img->draw_polygon(rectPoints, color);

  cimg_library::CImg< int > headPoints(3, 2);
  int lPointX = x3 - 1.0 * thick / 2 * cos(invTheta);
  int lPointY = y3 - 1.0 * thick / 2 * sin(invTheta);
  int rPointX = x3 + 1.0 * thick / 2 * cos(invTheta);
  int rPointY = y3 + 1.0 * thick / 2 * sin(invTheta);

  int uPointX = (1.0 * (x3 - x1) >= 0) ? x3 + 1.0 * thick * cos(theta)
                                       : x3 - 1.0 * thick * cos(theta);
  int uPointY = (1.0 * (x3 - x1) >= 0) ? y3 + 1.0 * thick * sin(theta)
                                       : y3 - 1.0 * thick * sin(theta);

  //    int uPointX = x3 + 1.0*thick*cos(theta);
  //    int uPointY = y3 + 1.0*thick*sin(theta);

  headPoints(0, 0) = lPointX;
  headPoints(0, 1) = lPointY;
  headPoints(1, 0) = rPointX;
  headPoints(1, 1) = rPointY;
  headPoints(2, 0) = uPointX;
  headPoints(2, 1) = uPointY;

  //    cout << x3 << " " << y3 << endl;
  //    cout << lPointX << " " << lPointY << endl;
  //    cout << rPointX << " " << rPointY << endl;
  //    cout << uPointX << " " << uPointY << endl << endl;

  img->draw_polygon(headPoints, color);
  //    img.draw_arrow( x1, y1, x3, y3, color, opacity );
}

void 
PlotEnv::DrawArrowDensity(CImgObj *img, float opacity) {
  int binMaxX = nb_->binCntX();
  int binMaxY = nb_->binCntY();
  int arrowSpacing = (binMaxX / 16 <= 0) ? 1 : binMaxX / 16;

  // below is essential for extracting e?Max
  float exMax = -1e30, eyMax = -1e30, ezMax = -1e30;
  for(int i = 0; i < binMaxX; i += arrowSpacing) {
    for(int j = 0; j < binMaxY; j += arrowSpacing) {
      Bin* bin = nb_->bins()[binMaxX * j + i];
      float newEx = fabs(bin->electroForceX());
      float newEy = fabs(bin->electroForceY());

      exMax = (exMax < newEx) ? newEx : exMax;
      eyMax = (eyMax < newEy) ? newEy : eyMax;
    }
  }

  for(int i = 0; i < binMaxX; i += arrowSpacing) {
    for(int j = 0; j < binMaxY; j += arrowSpacing) {
      Bin* bin = nb_->bins()[binMaxX * j + i];
      int signX = (bin->electroForceX() > 0) ? 1 : -1;
      int signY = (bin->electroForceY() > 0) ? 1 : -1;

      float newVx = fabs(bin->electroForceX());
      float newVy = fabs(bin->electroForceY());

      int x1 = bin->cx();
      int y1 = bin->cy();

      float dx = signX * newVx / exMax;
      float dy = signY * newVy / eyMax;

      //        float theta = atan(dy / dx);
      float length = sqrt(pow(nb_->binSizeX(), 2.0) +
                         pow(nb_->binSizeY(), 2.0)) *
                    5;

      int x3 = x1 + dx * length;
      int y3 = y1 + dy * length;

      int drawX1 = GetX(x1);
      int drawY1 = GetY(y1);

      int drawX3 = GetX(x3);
      int drawY3 = GetY(y3);

      //            img.draw_arrow( drawX1, drawY1, drawX3, drawY3, black,
      //            opacity );
      CimgDrawArrow(img, drawX1, drawY1, drawX3, drawY3, 20, red, opacity);
      //        cout << "sign: " <<signX << endl;
      //        cout << "newVx: " << newVx << endl;
      //        cout << "exMax: " << exMax << endl;
      //        cout << "bin_stp: " << bin_stp.x << endl;
      //        cout << binCoordi.x << " " << binCoordi.y << " "
      //            << curBin->center.x << " " << curBin->center.y << " "
      //            << signX*newVx/exMax * bin_stp.x * 10 << " "
      //            << signY*newVy/eyMax * bin_stp.y * 10 << endl;
    }
  }
  //    cout << "theta: " << theta << endl;
}

void PlotEnv::SaveCellPlot(CImgObj *img, bool isGCell) {
  auto& die = pb_->die();

  // draw die bounds
  img->draw_line(GetX(die.dieLx()), GetY(die.dieLy()),
                 GetX(die.dieUx()), GetY(die.dieLy()), pink);
  img->draw_line(GetX(die.dieUx()), GetY(die.dieLy()),
                 GetX(die.dieUx()), GetY(die.dieUy()), pink);
  img->draw_line(GetX(die.dieUx()), GetY(die.dieUy()),
                 GetX(die.dieLx()), GetY(die.dieUy()), pink);
  img->draw_line(GetX(die.dieLx()), GetY(die.dieUy()),
                 GetX(die.dieLx()), GetY(die.dieLy()), pink);

  // draw core bounds
  img->draw_line(GetX(die.coreLx()), GetY(die.coreLy()),
                 GetX(die.coreUx()), GetY(die.coreLy()), yellow);
  img->draw_line(GetX(die.coreUx()), GetY(die.coreLy()),
                 GetX(die.coreUx()), GetY(die.coreUy()), yellow);
  img->draw_line(GetX(die.coreUx()), GetY(die.coreUy()),
                 GetX(die.coreLx()), GetY(die.coreUy()), yellow);
  img->draw_line(GetX(die.coreLx()), GetY(die.coreUy()),
                 GetX(die.coreLx()), GetY(die.coreLy()), yellow);

  float opacity = 0.7;
  // FIXED CELL
  DrawTerminal(img, blue, black, opacity);

  // STD CELL
  if(!isGCell) {
    DrawModule(img, red, opacity);
  }
  else {
    DrawGcell(img, purple, darkblue, red, opacity);
  }
}

void PlotEnv::SaveBinPlot(CImgObj *img) {
  float opacity = 1;
  DrawBinDensity(img, opacity);
}

void PlotEnv::SaveArrowPlot(CImgObj *img) {
  float opacity = 1;
  DrawBinDensity(img, opacity);
  DrawArrowDensity(img, opacity);
}

//
// using X11
// isGCell : GCell drawing mode. true->gcell_st, false->moduleInstance
//
void PlotEnv::SavePlot(string imgName, bool isGCell) {
  // if gcell is exist, then update module's information
  // before drawing
  //    if( gcell_st ) {
  //        GCellPinCoordiUpdate();
  //    }
  CImg< unsigned char > img(GetTotalImageWidth(), GetTotalImageHeight(),
                            1, 3, 255);

  SaveCellPlot(&img, isGCell);
  //    SaveBinPlot(img);

  // Finally draw image info
  img.draw_text(50, 50, imgName.c_str(), black, NULL, 1, 100);
  imgStor.push_back(img);
}

//
// save current circuit's as BMP file in imgPosition & iternumber
// isGCell : GCell drawing mode. true->gcell_st, false->moduleInstance
//
void PlotEnv::SaveCellPlotAsJPEG(string imgName, bool isGCell, string imgPosition) {
  // if gcell is exist, then update module's information
  // before drawing
  //    if( gcell_st ) {
  //        GCellPinCoordiUpdate();
  //    }

  float opacity = 0.7;
  CImg< unsigned char > img(GetTotalImageWidth(), GetTotalImageHeight(),
                            1, 3, 255);

  //    int cnt = (lab == 0)? moduleCNT : gcell_cnt;

  /*
  for(int i=0; i<row_cnt; i++) {
      ROW* curRow = &row_st[i];
      int x1 = GetX( curRow->pmin.x, unitX ) + xMargin;
      int x3 = GetX( curRow->pmax.x, unitX ) + xMargin;
      int y1 = GetY( curRow->pmin.y, unitY, origHeight ) + yMargin;
      int y3 = GetY( curRow->pmax.y, unitY, origHeight ) + yMargin;
      img.draw_rectangle( x1, y1, x3, y3, black, 0.025 );
  }*/

  SaveCellPlot(&img, isGCell);

  /*
  for(int i=0; i<tier_st[0].tot_bin_cnt; i++) {
      BIN* curBin = &tier_st[0].bin_mat[i];
      int x1 = GetX( curBin->pmin.x, unitX ) + xMargin;
      int x3 = GetX( curBin->pmax.x, unitX ) + xMargin;
      int y1 = GetY( curBin->pmin.y, unitY, origHeight ) + yMargin;
      int y3 = GetY( curBin->pmax.y, unitY, origHeight ) + yMargin;
//        img.draw_rectangle( x1, y1, x3, y3, purple, opacity );
      img.draw_text( x1, (y1 + y3)/2 , to_string(int(100*curBin->den)).c_str(),
black, NULL, 1, 25);
      img.draw_text( x1 + 35, (y1 + y3)/2 ,
to_string(int(100*curBin->den2)).c_str(), black, NULL, 1, 25);


//        if( i > 5)
//            break;
  }
  */

  //    cout << "current imgName: " << imgName << endl;
  // Finally draw image info
  std::filesystem::path saveName = *plotPath / pathCell;
  saveName /= imgPosition + ".jpg";

  img.draw_text(50, 50, imgName.c_str(), black, NULL, 1, 30);
  img.save_jpeg(saveName.c_str(), 70);
  //  img.save_bmp( string(imgPosition + string(".bmp")).c_str() );
  log_->info(GPL, 200, "{} image has been saved", saveName.c_str());
}

//
// save current circuit's as BMP file in imgPosition & iternumber
void PlotEnv::SaveBinPlotAsJPEG(string imgName, string imgPosition) {
  CImg< unsigned char > img(GetTotalImageWidth(), GetTotalImageHeight(),
                            1, 3, 255);

  SaveBinPlot(&img);

  //    cout << "current imgName: " << imgName << endl;
  // Finally draw image info
  std::filesystem::path saveName = *plotPath / pathBin;
  saveName /= imgPosition + ".jpg";

  img.draw_text(50, 50, imgName.c_str(), black, NULL, 1, 30);
  img.save_jpeg(saveName.c_str(), 70);
  //  img.save_bmp( string(imgPosition + string(".bmp")).c_str() );
  log_->info(GPL, 201, "{} image has been saved", saveName.c_str());
}
//
// save current circuit's as BMP file in imgPosition & iternumber
void PlotEnv::SaveArrowPlotAsJPEG(string imgName, string imgPosition) {
  CImg< unsigned char > img(GetTotalImageWidth(), GetTotalImageHeight(),
                            1, 3, 255);

  SaveArrowPlot(&img);

  //    cout << "current imgName: " << imgName << endl;
  // Finally draw image info
  std::filesystem::path saveName = *plotPath / pathArrow;
  saveName /= imgPosition + ".jpg";

  img.draw_text(50, 50, imgName.c_str(), black, NULL, 1, 30);
  img.save_jpeg(saveName.c_str(), 70);
  //  img.save_bmp( string(imgPosition + string(".bmp")).c_str() );
  log_->info(GPL, 202, "{} image has been saved", saveName.c_str());
}

void PlotEnv::setPlotPath(std::string path) {
	if (path.empty()) PlotEnv::plotPath = nullptr;
	else PlotEnv::plotPath = std::make_unique<std::filesystem::path>(path);
}


}
#endif
