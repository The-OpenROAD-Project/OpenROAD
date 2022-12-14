//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#include "colorGenerator.h"

namespace gui {

// https://mokole.com/palette.html
const std::array<QColor, 31> ColorGenerator::colors_{
    QColor{255, 0, 0},     QColor{255, 140, 0},   QColor{255, 215, 0},
    QColor{0, 255, 0},     QColor{148, 0, 211},   QColor{0, 250, 154},
    QColor{220, 20, 60},   QColor{0, 255, 255},   QColor{0, 191, 255},
    QColor{0, 0, 255},     QColor{173, 255, 47},  QColor{218, 112, 214},
    QColor{255, 0, 255},   QColor{30, 144, 255},  QColor{250, 128, 114},
    QColor{176, 224, 230}, QColor{255, 20, 147},  QColor{123, 104, 238},
    QColor{255, 250, 205}, QColor{255, 182, 193}, QColor{85, 107, 47},
    QColor{139, 69, 19},   QColor{72, 61, 139},   QColor{0, 128, 0},
    QColor{60, 179, 113},  QColor{184, 134, 11},  QColor{0, 139, 139},
    QColor{0, 0, 139},     QColor{50, 205, 50},   QColor{128, 0, 128},
    QColor{176, 48, 96}};

ColorGenerator::ColorGenerator() : index_(0)
{
}

QColor ColorGenerator::getQColor()
{
  QColor color = colors_[index_++];
  if (index_ == getColorCount()) {
    index_ = 0;
  }
  return color;
}

Painter::Color ColorGenerator::getColor()
{
  const QColor color = getQColor();
  return {color.red(), color.green(), color.blue(), color.alpha()};
}

}  // namespace gui
