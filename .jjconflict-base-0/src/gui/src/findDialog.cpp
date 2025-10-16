// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "findDialog.h"

#include <QDialog>
#include <QWidget>
#include <string>

#include "gui/gui.h"

namespace gui {
FindObjectDialog::FindObjectDialog(QWidget* parent) : QDialog(parent)
{
  setupUi(this);
}

void FindObjectDialog::accept()
{
  std::string pattern_to_find = findObjEdit->text().trimmed().toStdString();
  bool match_case = false;
  if (matchCaseCheckBox->isEnabled()) {
    match_case = matchCaseCheckBox->isChecked();
  }

  if (findObjType->currentText() == "Instance") {
    Gui::get()->select("Inst",
                       pattern_to_find,
                       "",
                       0,
                       match_case,
                       addToHighlightCheckBox->isChecked() ? 0 : -1);
  } else if (findObjType->currentText() == "Net") {
    Gui::get()->select("Net",
                       pattern_to_find,
                       "",
                       0,
                       match_case,
                       addToHighlightCheckBox->isChecked() ? 0 : -1);
  } else {
    Gui::get()->select("BTerm",
                       pattern_to_find,
                       "",
                       0,
                       match_case,
                       addToHighlightCheckBox->isChecked() ? 0 : -1);
  }

  QDialog::accept();
}

void FindObjectDialog::reject()
{
  QDialog::reject();
}

int FindObjectDialog::exec()
{
  findObjEdit->setFocus();
  return QDialog::exec();
}

}  // namespace gui
