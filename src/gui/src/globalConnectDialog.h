/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

#include "odb/db.h"

namespace gui {

class GlobalConnectDialog : public QDialog
{
  Q_OBJECT
 public:
  GlobalConnectDialog(odb::dbBlock* block, QWidget* parent = nullptr);

 signals:
  void connectionsMade(int connections);

 private slots:
  void runRules();
  void clearRules();
  void deleteRule(odb::dbGlobalConnect* gconnect);
  void makeRule();
  void announceConnections(int connections);
  void addRegexTextChanged(const QString& text);

 private:
  odb::dbBlock* block_;

  QGridLayout* layout_;

  QPushButton* add_;
  QPushButton* clear_;
  QPushButton* run_;

  struct GlobalConnectWidgets
  {
    QLineEdit* inst_pattern;
    QLineEdit* pin_pattern;
    QLineEdit* net;
    QLineEdit* region;

    QPushButton* run;
    QPushButton* remove;
  };
  std::map<odb::dbGlobalConnect*, GlobalConnectWidgets> rules_;

  QLineEdit* inst_pattern_;
  QLineEdit* pin_pattern_;
  QComboBox* net_;
  QComboBox* region_;

  QLabel* connections_;

  void addRule(odb::dbGlobalConnect* gc);
  void runRule(odb::dbGlobalConnect* gc);
};

}  // namespace gui
