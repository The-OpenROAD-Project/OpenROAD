/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
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

#include "gui/gui.h"
#include <QDockWidget>
#include <QTreeView>
#include <QStandardItemModel>

#include <array>
#include <memory>

#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

namespace gui {

class BrowserWidget : public QDockWidget, public odb::dbBlockCallBackObj
{
  Q_OBJECT

  public:
    BrowserWidget(QWidget* parent = nullptr);

    // dbBlockCallBackObj
    virtual void inDbInstCreate(odb::dbInst*);
    virtual void inDbInstCreate(odb::dbInst*, odb::dbRegion*);
    virtual void inDbInstDestroy(odb::dbInst*);
    virtual void inDbInstSwapMasterAfter(odb::dbInst*);

  signals:
    void select(const Selected& selected);

  public slots:
    void setBlock(odb::dbBlock* block);
    void clicked(const QModelIndex& index);

    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

  protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

  private:
    void updateModel();
    void clearModel();

    odb::dbBlock* block_;

    QTreeView* view_;
    QStandardItemModel* model_;

    int64_t populateModule(odb::dbModule* module, QStandardItem* parent);

    int64_t addInstanceItem(odb::dbInst* inst, QStandardItem* parent);
    int64_t addModuleItem(odb::dbModule* module, QStandardItem* parent);

    QStandardItem* makeSizeItem(int64_t area) const;
    QStandardItem* makeMasterItem(const std::string& name) const;
};

}  // namespace gui
