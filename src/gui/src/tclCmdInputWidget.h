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

#include <QCompleter>
#include <QMenu>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSettings>
#include <QStringList>
#include <QStringListModel>
#include <memory>
#include <set>
#include <string>

#include "tclCmdHighlighter.h"
#include "tclSwig.h"  // generated header

namespace gui {

class TclCmdInputWidget : public QPlainTextEdit
{
  Q_OBJECT

 public:
  TclCmdInputWidget(QWidget* parent = nullptr);
  ~TclCmdInputWidget();

  void setTclInterp(Tcl_Interp* interp,
                    bool do_init_openroad,
                    const std::function<void(void)>& post_or_init);

  void setFont(const QFont& font);

  QString text() const;
  void setText(const QString& text);

  void setMaximumHeight(int height);

  void readSettings(QSettings* settings);
  void writeSettings(QSettings* settings);

 signals:
  // tcl exit has been initiated, want the gui to handle
  // shutdown
  void tclExiting();

  void commandAboutToExecute();
  void commandFinishedExecuting(int code);

  void addResultToOutput(const QString& result, int code);
  void addCommandToOutput(const QString& cmd);

 public slots:
  void executeCommand(const QString& cmd,
                      bool echo = true,
                      bool silent = false);

 private slots:
  void updateSize();

  void updateHighlighting();
  void updateCompletion();

  void insertCompletion(const QString& text);

 protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  void keyPressEvent(QKeyEvent* e) override;
  void keyReleaseEvent(QKeyEvent* e) override;

 private:
  // back in history
  void goBackHistory();
  // forward in history
  void goForwardHistory();

  void init();
  void processTclResult(int result_code);

  static int tclExitHandler(ClientData instance_data,
                            Tcl_Interp* interp,
                            int argc,
                            const char** argv);

  bool isCommandComplete(const std::string& cmd);

  void determineLineHeight();

  void initOpenRoadCommands();
  void parseOpenRoadArguments(const char* or_args, std::set<std::string>& args);
  void collectNamespaces(std::set<std::string>& namespaces);
  void collectSWIGArguments();

  const QString wordUnderCursor();
  const swig_class* swigBeforeCursor();

  void setCompleterCommands();
  void setCompleterSWIG(const swig_class* type);
  void setCompleterArguments(const std::set<int>& cmds);
  void setCompleterVariables();

  int line_height_;
  int document_margins_;

  int max_height_;

  Tcl_Interp* interp_;
  QStringList history_;
  QString history_buffer_last_;
  int historyPosition_;

  std::unique_ptr<QMenu> context_menu_;
  std::unique_ptr<QAction> enable_highlighting_;
  std::unique_ptr<QAction> enable_completion_;

  std::unique_ptr<TclCmdHighlighter> highlighter_;

  std::unique_ptr<QCompleter> completer_;
  std::unique_ptr<QStringListModel> completer_options_;
  std::unique_ptr<QStringList> completer_commands_;
  std::unique_ptr<QRegularExpression> completer_start_of_command_;
  std::unique_ptr<QRegularExpression> completer_end_of_command_;

  // hold openroad commands and associated arguments
  std::vector<CommandArguments> commands_;
  std::map<const swig_class*, std::unique_ptr<QStringList>> swig_arguments_;

  static constexpr const char* enable_highlighting_keyword_ = "highlighting";
  static constexpr const char* enable_completion_keyword_ = "completion";
  static const int completer_mimimum_length_ = 2;
  static constexpr const char* command_rename_prefix_ = "::tcl::openroad::";
};

}  // namespace gui
