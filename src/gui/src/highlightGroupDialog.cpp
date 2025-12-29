// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "highlightGroupDialog.h"

#include <QColor>
#include <QDialog>
#include <QPalette>
#include <QWidget>
#include <vector>

#include "gui/gui.h"

namespace gui {
HighlightGroupDialog::HighlightGroupDialog(QWidget* parent) : QDialog(parent)
{
  setupUi(this);
  grp1RadioButton->setChecked(true);

  setButtonBackground(grp1RadioButton, Painter::kHighlightColors[0]);
  setButtonBackground(grp2RadioButton, Painter::kHighlightColors[1]);
  setButtonBackground(grp3RadioButton, Painter::kHighlightColors[2]);
  setButtonBackground(grp4RadioButton, Painter::kHighlightColors[3]);
  setButtonBackground(grp5RadioButton, Painter::kHighlightColors[4]);
  setButtonBackground(grp6RadioButton, Painter::kHighlightColors[5]);
  setButtonBackground(grp7RadioButton, Painter::kHighlightColors[6]);
  setButtonBackground(grp8RadioButton, Painter::kHighlightColors[7]);
  setButtonBackground(grp9RadioButton, Painter::kHighlightColors[8]);
  setButtonBackground(grp10RadioButton, Painter::kHighlightColors[9]);
  setButtonBackground(grp11RadioButton, Painter::kHighlightColors[10]);
  setButtonBackground(grp12RadioButton, Painter::kHighlightColors[11]);
  setButtonBackground(grp13RadioButton, Painter::kHighlightColors[12]);
  setButtonBackground(grp14RadioButton, Painter::kHighlightColors[13]);
  setButtonBackground(grp15RadioButton, Painter::kHighlightColors[14]);
  setButtonBackground(grp16RadioButton, Painter::kHighlightColors[15]);
}

// NOLINTNEXTLINE(readability-non-const-parameter)
void HighlightGroupDialog::setButtonBackground(QRadioButton* button,
                                               Painter::Color color)
{
  QPalette pal = button->palette();
  QColor button_color(color.r, color.g, color.b, color.a);
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
                                                     grp8RadioButton,
                                                     grp9RadioButton,
                                                     grp10RadioButton,
                                                     grp11RadioButton,
                                                     grp12RadioButton,
                                                     grp13RadioButton,
                                                     grp14RadioButton,
                                                     grp15RadioButton,
                                                     grp16RadioButton};
  for (int i = 0; i < kNumHighlightSet; ++i) {
    if (highlight_group_buttons[i]->isChecked()) {
      return i;
    }
  }
  return 0;
}

void HighlightGroupDialog::accept()
{
  QDialog::accept();
}
}  // namespace gui
