///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
#include <memory>

#include <QMenu>
#include <QPlainTextEdit>
#include <QSettings>

#include "tclCmdHighlighter.h"

namespace gui {

class TclCmdInputWidget: public QPlainTextEdit {
    Q_OBJECT

  public:
    TclCmdInputWidget(QWidget* parent = nullptr);
    ~TclCmdInputWidget();

    void init(Tcl_Interp* interp);

    void setFont(const QFont& font);

    QString text();
    void setText(const QString& text);

    void setMaximumHeight(int height);

    void readSettings(QSettings* settings);
    void writeSettings(QSettings* settings);

  public slots:
    void commandExecuted(int return_code);

  signals:
    // complete TCL command available
    void completeCommand();

    // back in history
    void historyGoBack();

    // forward in history
    void historyGoForward();

  private slots:
    void updateSize();

    void updateHighlighting();

  protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

  private:
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

    bool isCommandComplete(const std::string& cmd);

    void determineLineHeight();

    int line_height_;
    int document_margins_;

    int max_height_;

    std::unique_ptr<QMenu> context_menu_;
    std::unique_ptr<QAction> enable_highlighting_;

    static constexpr const char* enable_highlighting_keyword_ = "highlighting";

    std::unique_ptr<TclCmdHighlighter> highlighter_;
};

}  // namespace gui
