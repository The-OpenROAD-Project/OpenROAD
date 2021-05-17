///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, OpenROAD
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

#pragma once

#include <QDockWidget>
#include <QTreeView>
#include <QStandardItemModel>

#include "gui/gui.h"

namespace gui {

// The inspector is to allow a single object to have it properties displayed.
// It is generic and builds on the Selected and Descriptor classes.
// The inspector knows how to handle SelectionSet and Selected objects
// and will create links for them in the tree widget.  Simple properties,
// like strings, are handled as well.
class Inspector : public QDockWidget
{
  Q_OBJECT

 public:
  Inspector(const SelectionSet& selected, QWidget* parent = nullptr);

 signals:
  void selected(const Selected& selected, bool showConnectivity = false);

 public slots:
  void inspect(const Selected& object);
  void clicked(const QModelIndex& index);
  void update();

 private:
  // The columns in the tree view
  enum Column
  {
    Name,
    Value
  };

  QTreeView* view_;
  QStandardItemModel* model_;
  const SelectionSet& selected_;
};

}  // namespace gui
