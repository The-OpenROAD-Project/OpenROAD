///////////////////////////////////////////////////////////////////////////////
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

#pragma once

#include <tcl.h>

#include <QDockWidget>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QStringList>
#include <QTextEdit>
#include <QTreeView>

#include "options.h"

namespace odb {
class dbDatabase;
class dbBlock;
}  // namespace odb

namespace gui {

struct Callback
{
  std::function<void(bool)> action;
};

// This class shows the user the set of layers & objects that
// they can control the visibility and selectablity of.  The
// controls are show in a tree view to provide grouping of
// related options.
//
// It also implements the Options interface so that other clients can
// access the data.
class DisplayControls : public QDockWidget, public Options
{
  Q_OBJECT

 public:
  DisplayControls(QWidget* parent = nullptr);

  void setDb(odb::dbDatabase* db);

  // From the Options API
  QColor color(const odb::dbTechLayer* layer) override;
  bool   isVisible(const odb::dbTechLayer* layer) override;
  bool   isSelectable(const odb::dbTechLayer* layer) override;
  bool   arePrefTracksVisible() override;
  bool   areNonPrefTracksVisible() override;

 signals:
  // The display options have changed and clients need to update
  void changed();

 public slots:
  // Tells this widget that a new design is loaded and the
  // options displayed need to match
  void designLoaded(odb::dbBlock* block);

  // This is called by the check boxes to update the state
  void itemChanged(QStandardItem* item);

 private:
  // The columns in the tree view
  enum Column
  {
    Name,
    Swatch,
    Visible,
    Selectable
  };

  void techInit();

  template <typename T>
  QStandardItem* makeItem(const QString&                   text,
                          T*                               parent,
                          Qt::CheckState                   checked,
                          const std::function<void(bool)>& visibility_action,
                          const std::function<void(bool)>& select_action
                          = std::function<void(bool)>(),
                          const QColor& color = Qt::transparent);

  void toggleAllChildren(bool checked, QStandardItem* parent, Column column);

  QTreeView*          view_;
  QStandardItemModel* model_;

  // Categories in the model
  QStandardItem* layers_;
  QStandardItem* routing_;
  QStandardItem* tracks_;

  // Object controls
  QStandardItem* tracks_pref_;
  QStandardItem* tracks_non_pref_;

  odb::dbDatabase*                          db_;
  bool                                      tech_inited_;
  bool                                      tracks_visible_pref_;
  bool                                      tracks_visible_non_pref_;
  std::map<const odb::dbTechLayer*, QColor> layer_color_;
  std::map<const odb::dbTechLayer*, bool>   layer_visible_;
  std::map<const odb::dbTechLayer*, bool>   layer_selectable_;
};

}  // namespace gui

Q_DECLARE_METATYPE(gui::Callback);
