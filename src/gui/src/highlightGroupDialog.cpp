//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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

#include "highlightGroupDialog.h"

#include <QColor>
#include <QDialog>
#include <QPalette>

namespace gui {
HighlightGroupDialog::HighlightGroupDialog(QWidget* parent) : QDialog(parent)
{
  setupUi(this);
  grp1RadioButton->setChecked(true);

  setButtonBackground(grp1RadioButton, Painter::highlightColors[0]);
  setButtonBackground(grp2RadioButton, Painter::highlightColors[1]);
  setButtonBackground(grp3RadioButton, Painter::highlightColors[2]);
  setButtonBackground(grp4RadioButton, Painter::highlightColors[3]);
  setButtonBackground(grp5RadioButton, Painter::highlightColors[4]);
  setButtonBackground(grp6RadioButton, Painter::highlightColors[5]);
  setButtonBackground(grp7RadioButton, Painter::highlightColors[6]);
  setButtonBackground(grp8RadioButton, Painter::highlightColors[7]);
}

void HighlightGroupDialog::setButtonBackground(QRadioButton* button,
                                               Painter::Color color)
{
  QPalette pal = button->palette();
  QColor button_color(color.r, color.g, color.b);
  pal.setColor(QPalette::Button, button_color);
  button->setAutoFillBackground(true);
  button->setPalette(pal);
  button->update();
}

int HighlightGroupDialog::getSelectedHighlightGroup() const
{
  std::vector<QRadioButton*> highlight_group_buttons{grp1RadioButton,
                                                     grp2RadioButton,
                                                     grp3RadioButton,
                                                     grp4RadioButton,
                                                     grp5RadioButton,
                                                     grp6RadioButton,
                                                     grp7RadioButton,
                                                     grp8RadioButton};
  for (int i = 0; i < 8; ++i) {
    if (highlight_group_buttons[i]->isChecked())
      return i;
  }
  return 0;
}

void HighlightGroupDialog::accept()
{
  QDialog::accept();
}
}  // namespace gui