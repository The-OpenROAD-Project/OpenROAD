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
#include <QPushButton>
#include <QCheckBox>
#include <QSettings>

#include <memory>
#include <variant>

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace gui {


class DRCViolation {
  public:
    using DRCShape = std::variant<QLine, QRect, QPolygon>;

    DRCViolation(const std::string& name,
                 const std::string& type,
                 const std::vector<std::any>& srcs,
                 const std::vector<DRCShape>& shapes,
                 odb::dbTechLayer* layer,
                 const std::string& comment);
    DRCViolation(const std::string& name,
                 const std::string& type,
                 const std::vector<DRCShape>& shapes,
                 const std::string& comment);
    ~DRCViolation() {}

    const std::string& getName() { return name_; }
    const std::string& getType() { return type_; }
    const std::vector<std::any>& getSources() { return srcs_; }
    const std::vector<DRCShape>& getShapes() { return shapes_; }
    const odb::Rect& getBBox() { return bbox_; }
    const std::string getComment() { return comment_; }
    odb::dbTechLayer* getLayer() { return layer_; }

    void paint(Painter& painter);

  private:
    void computeBBox();

    std::string name_;
    std::string type_;
    std::vector<std::any> srcs_;
    std::vector<DRCShape> shapes_;
    odb::dbTechLayer* layer_;
    std::string comment_;
    odb::Rect bbox_;
};

class DRCDescriptor : public Descriptor
{
  public:
    std::string getName(std::any object) const override;
    std::string getTypeName(std::any object) const override;
    bool getBBox(std::any object, odb::Rect& bbox) const override;

    void highlight(std::any object,
                   Painter& painter,
                   void* additional_data) const override;

    Properties getProperties(std::any object) const override;
    Selected makeSelected(std::any object, void* additional_data) const override;
    bool lessThan(std::any l, std::any r) const override;
};

class DRCWidget : public QDockWidget, public Renderer
{
  Q_OBJECT

  public:
    DRCWidget(QWidget* parent = nullptr);
    ~DRCWidget() {}

    void setLogger(utl::Logger* logger);

    // Renderer
    void drawObjects(Painter& painter) override;
    SelectionSet select(odb::dbTechLayer* layer, const odb::Point& point) override;

  signals:
    void selectDRC(const Selected& selected);

  public slots:
    void loadReport(const QString& filename);
    void setBlock(odb::dbBlock* block);
    void clicked(const QModelIndex& index);
    void selectReport();
    void toggleRenderer(bool visible);

  protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

  private:
    void loadTRReport(const QString& filename);
    void loadASCIIReport(const QString& filename);
    void updateModel();

    utl::Logger* logger_;

    QTreeView* view_;
    QStandardItemModel* model_;

    odb::dbBlock* block_;

    QPushButton* load_;

    std::vector<std::unique_ptr<DRCViolation>> violations_;
};

}  // namespace gui
