///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "plotgnu.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>
#include "rectangle.h"
#include "utility.h"

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// PlotGnu::PlotGnu:
////////////////////////////////////////////////////////////////////////////////
PlotGnu::PlotGnu()
    : m_network(0),
      m_counter(0),
      m_drawArchitecture(false),
      m_drawNodes(true),
      m_skipStandardCells(false),
      m_drawDisp(false) {}

////////////////////////////////////////////////////////////////////////////////
// PlotGnu::Draw:
////////////////////////////////////////////////////////////////////////////////
void PlotGnu::Draw(Network* network, Architecture* arch, char* msg) {
  m_network = network;
  m_arch = arch;
  m_rt = 0;
  m_skipStandardCells = false;

  char buf[1024];
  if (msg == 0)
    buf[0] = '\0';
  else
    sprintf(&buf[0], "%s", msg);

  if (m_drawNodes) drawNodes(buf);
  ++m_counter;
}

////////////////////////////////////////////////////////////////////////////////
// PlotGnu::drawNodes:
////////////////////////////////////////////////////////////////////////////////
void PlotGnu::drawNodes(char* buf) {

  double rowHeight = m_arch->getRow(0)->getHeight();
  double siteWidth = m_arch->getRow(0)->getSiteWidth();


  FILE* fpMain = 0;
  FILE* fpArch = 0;
  FILE* fpCell = 0;
  FILE* fpIO = 0;
  FILE* fpFixed = 0;
  FILE* fpShapes = 0;
  FILE* fpShreds = 0;
  FILE* fpOutlines = 0;
  FILE* fpPeanut = 0;
  char counter[128];
  sprintf(&counter[0], "%05d", m_counter);

  // Open different files to put object info into different .dat files.  This
  // allows plotting with different colors.
  snprintf(m_filename, BUFFER_SIZE, "placement.boxes.%05d.gp", m_counter);

  std::string mainName = "placement." + std::string(counter) + ".gp";
  if ((fpMain = fopen(mainName.c_str(), "w")) == 0) {
    cout << "Error opening file '" << mainName << "'." << endl;
    return;
  }
  std::cout << "Drawing " << mainName << std::endl;

  std::string archName = "placement." + std::string(counter) + "arch.dat";
  if ((fpArch = fopen(archName.c_str(), "w")) == 0) {
    cout << "Error opening file '" << archName << "'." << endl;
    return;
  }
  std::string cellName = "placement." + std::string(counter) + "cell.dat";
  if ((fpCell = fopen(cellName.c_str(), "w")) == 0) {
    cout << "Error opening file '" << cellName << "'." << endl;
    return;
  }
  std::string ioName = "placement." + std::string(counter) + "io.dat";
  if ((fpIO = fopen(ioName.c_str(), "w")) == 0) {
    cout << "Error opening file '" << ioName << "'." << endl;
    return;
  }
  std::string fixedName = "placement." + std::string(counter) + "fixed.dat";
  if ((fpFixed = fopen(fixedName.c_str(), "w")) == 0) {
    cout << "Error opening file '" << fixedName << "'." << endl;
    return;
  }
  std::string shapesName = "placement." + std::string(counter) + "shapes.dat";
  if ((fpShapes = fopen(shapesName.c_str(), "w")) == 0) {
    cout << "Error opening file '" << shapesName << "'." << endl;
    return;
  }
  std::string shredsName = "placement." + std::string(counter) + "shreds.dat";
  if ((fpShreds = fopen(shredsName.c_str(), "w")) == 0) {
    cout << "Error opening file '" << shredsName << "'." << endl;
    return;
  }
  std::string outlinesName =
      "placement." + std::string(counter) + "outlines.dat";
  if ((fpOutlines = fopen(outlinesName.c_str(), "w")) == 0) {
    cout << "Error opening file '" << outlinesName << "'." << endl;
    return;
  }
  std::string peanutName = "placement." + std::string(counter) + "peanut.dat";
  if ((fpPeanut = fopen(peanutName.c_str(), "w")) == 0) {
    cout << "Error opening file '" << peanutName << "'." << endl;
    return;
  }

  if (fpMain == 0 || fpArch == 0 || fpCell == 0 || fpIO == 0 || fpFixed == 0 ||
      fpShapes == 0 || fpShreds == 0 || fpOutlines == 0 || fpPeanut == 0) {
    cout << "Error attempting to write gnuplot files." << endl;
    return;
  }

  // Find rectangle around everything we have to draw.
  Rectangle range;
  range.addPt(m_arch->getMinX(), m_arch->getMinY());
  range.addPt(m_arch->getMaxX(), m_arch->getMaxY());
  for (int i = 0; i < m_network->getNumNodes(); i++) {
    Node* ndi = m_network->getNode(i);
    range.addPt(ndi->getLeft(), ndi->getBottom());
    range.addPt(ndi->getRight(), ndi->getTop());
  }

  // Output the architecture, which, at the moment is simply a box...
  {
    int lx = m_arch->getMinX();
    int rx = m_arch->getMaxX();
    int yb = m_arch->getMinY();
    int yt = m_arch->getMaxY();
    fprintf(fpArch,
          "\n"
          "%d %d\n"
          "%d %d\n"
          "%d %d\n"
          "%d %d\n"
          "%d %d\n"
          "\n",
          lx, yb, lx, yt, rx, yt, rx, yb, lx, yb);
  }

  // Draw regions with the exception of the default region.
  for (int r = 1; r < m_arch->getNumRegions(); r++) {
    Architecture::Region* regPtr = m_arch->getRegion(r);
    for (const Rectangle_i& rect : regPtr->getRects()) {
      fprintf(fpArch,
              "\n"
              "%d %d\n"
              "%d %d\n"
              "%d %d\n"
              "%d %d\n"
              "%d %d\n"
              "\n",
              rect.xmin(), rect.ymin(), rect.xmin(), rect.ymax(), rect.xmax(),
              rect.ymax(), rect.xmax(), rect.ymin(), rect.xmin(), rect.ymin());
    }
  }

  // Output different data files to display nodes in different colors...
  bool didPrintCell = true;
  bool didPrintIO = false;
  bool didPrintFixed = false;
  bool didPrintShreds = false;
  bool didPrintOutlines = false;
  bool didPrintShapes = false;
  bool didPrintPeanut = false;
  for (int i = 0; i < m_network->getNumNodes(); i++) {
    Node* ndi = m_network->getNode(i);

    FILE* fpCurr = fpCell;
    bool draw = true;
    if (ndi->isFiller()) {
      draw = false; // Don't draw filler.
    }
    else if (ndi->isTerminal() || ndi->isTerminalNI()) {
      didPrintIO = true;
      fpCurr = fpIO;
    }
    else if (ndi->isFixed()) {
      didPrintFixed = true;
      fpCurr = fpFixed;
    }
    else if (ndi->isShape()) {
      didPrintShapes = true;
      fpCurr = fpShapes;
    }
    else {
      // Should be a cell.
      ;
    }

    if (draw) {
      double lx = ndi->getLeft();
      double rx = ndi->getRight();
      double yb = ndi->getBottom();
      double yt = ndi->getTop();
      if (rx-lx>=siteWidth) {
        lx += 0.0250*siteWidth;
        rx -= 0.0250*siteWidth;
      }
      if (yt-yb>=rowHeight) {
        yb += 0.0250*rowHeight;
        yt -= 0.0250*rowHeight;
      }

      fprintf(fpCurr,
        "\n"
        "%lf %lf\n"
        "%lf %lf\n"
        "%lf %lf\n"
        "%lf %lf\n"
        "%lf %lf\n"
        "\n",
        lx, yb, lx, yt,
        rx, yt, rx, yb,
        lx, yb);
    }
  }

  if (m_drawDisp) {
    // Draw displacement.

    didPrintPeanut = true;
    for (unsigned i = 0; i < m_network->getNumNodes(); i++) {
      Node* nd = m_network->getNode(i);

      double x1 = nd->getLeft();
      double y1 = nd->getBottom();

      double x2 = nd->getOrigLeft();
      double y2 = nd->getOrigBottom();

      FILE* fpCurr = fpPeanut;

      fprintf(fpCurr,
              "\n"
              "%lf %lf\n"
              "%lf %lf\n"
              "\n",
              x1, y1, x2, y2);
    }
  }

  // Output the top level script file...

  double hpwl = Utility::hpwl(m_network);
  fprintf(fpMain,
          "# Use this file as a script for gnuplot\n"
          //"set title \"Plot: %d, Hpwl: %.3e, #Cells: %d, #Nets: %d, %s\" font
          //\"Times, 12\"\n"
          "set title \"Plot: %d, Hpwl: %.3e, %s\" font \"Times, 12\"\n"
          "set nokey\n"
          "set noxtics\n"
          "set noytics\n"
          "set nokey\n"
          "set terminal x11\n"
          "#Uncomment these two lines starting with \"set\"\n"
          "#to save an EPS file for inclusion into a latex document\n"
          "#set terminal postscript eps color solid 20\n"
          "#set output \"%s\"\n\n",
          m_counter, hpwl,
          buf, m_filename);

  // Constrain the x and y ranges.
  fprintf(fpMain, "set xrange[%lf:%lf]\n", range.xmin()-1.0, range.xmax()+1.0);
  fprintf(fpMain, "set yrange[%lf:%lf]\n", range.ymin()-1.0, range.ymax()+1.0);

  // Setup some line types.
  fprintf(fpMain, "set style line 1 lt 1 lw 3 lc rgb \"purple\"\n");
  fprintf(fpMain, "set style line 2 lt 1 lw 1 lc rgb \"black\"\n");
  fprintf(fpMain, "set style line 3 lt 1 lw 1 lc rgb \"green\"\n");
  fprintf(fpMain, "set style line 4 lt 1 lw 1 lc rgb \"red\"\n");
  fprintf(fpMain, "set style line 5 lt 2 lw 1 lc rgb \"orange\"\n");
  fprintf(fpMain, "set style line 6 lt 2 lw 1 lc rgb \"blue\"\n");
  fprintf(fpMain, "set style line 7 lt 2 lw 1 lc rgb \"brown\"\n");

  // Output the plot command.
  fprintf(fpMain, "plot [:][:] \"%s\" with lines ls 2", archName.c_str());
  if (didPrintCell) {
    fprintf(fpMain, ",\\\n    \"%s\" title \"Cell\" with lines ls 4",
            cellName.c_str());
  }
  if (didPrintIO) {
    fprintf(fpMain, ",\\\n    \"%s\" title \"IO\" with lines ls 3",
            ioName.c_str());
  }
  if (didPrintFixed) {
    fprintf(fpMain, ",\\\n    \"%s\" title \"Fixed\" with lines ls 2",
            fixedName.c_str());
  }
  if (didPrintShapes) {
    fprintf(fpMain, ",\\\n    \"%s\" title \"Shapes\" with lines ls 5",
            shapesName.c_str());
  }
  if (didPrintShreds) {
    fprintf(fpMain, ",\\\n    \"%s\" title \"Shreds\" with lines ls 6",
            shredsName.c_str());
  }
  if (didPrintOutlines) {
    fprintf(fpMain, ",\\\n    \"%s\" title \"Outlines\" with lines ls 1",
            outlinesName.c_str());
  }
  if (didPrintPeanut) {
    fprintf(fpMain, ",\\\n    \"%s\" title \"Peanuts\" with lines ls 7",
            peanutName.c_str());
  }
  fprintf(fpMain, "\n");

  // Cleanup.
  fclose(fpMain);
  fclose(fpArch);
  fclose(fpCell);
  fclose(fpIO);
  fclose(fpFixed);
  fclose(fpShapes);
  fclose(fpShreds);
  fclose(fpOutlines);
  fclose(fpPeanut);
}

}  // namespace dpo
