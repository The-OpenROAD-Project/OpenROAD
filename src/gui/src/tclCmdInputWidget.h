// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <QCompleter>
#include <QMenu>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QStringListModel>
#include <QWidget>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "cmdInputWidget.h"
#include "tcl.h"
#include "tclCmdHighlighter.h"

#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION < 7) && !defined(Tcl_Size)
#define Tcl_Size int
#endif
#include "tclSwig.h"  // generated header

namespace gui {

class TclCmdInputWidget : public CmdInputWidget
{
  Q_OBJECT

 public:
  // When exiting we have to return an error to stop script execution.
  // We don't want to log the error so we use this string as a special
  // key.  Not elegant but that's TCL.
  static constexpr const char* kExitString = "_GUI EXITING_";

  TclCmdInputWidget(QWidget* parent = nullptr);
  ~TclCmdInputWidget() override;

  void setTclInterp(Tcl_Interp* interp,
                    bool do_init_openroad,
                    const std::function<void()>& post_or_init);

  void readSettings(QSettings* settings);
  void writeSettings(QSettings* settings);

  // Bring the overloads into scope.
  using CmdInputWidget::executeCommand;

 public slots:
  void executeCommand(const QString& cmd, bool echo, bool silent) override;

 private slots:
  void updateHighlighting();
  void updateCompletion();

  void insertCompletion(const QString& text);

 protected:
  void contextMenuEvent(QContextMenuEvent* event) override;
  void keyPressEvent(QKeyEvent* e) override;
  void keyReleaseEvent(QKeyEvent* e) override;

  bool isCommandComplete(const std::string& cmd) const override;

 private:
  void init();
  void processTclResult(int tcl_result);

  static int tclExitHandler(ClientData instance_data,
                            Tcl_Interp* interp,
                            int argc,
                            const char** argv);

  void initOpenRoadCommands();
  void parseOpenRoadArguments(const char* or_args, std::set<std::string>& args);
  void collectNamespaces(std::set<std::string>& namespaces);
  void collectSWIGArguments();

  QString wordUnderCursor();
  const swig_class* swigBeforeCursor();

  void setCompleterCommands();
  void setCompleterSWIG(const swig_class* type);
  void setCompleterArguments(const std::set<int>& cmds);
  void setCompleterVariables();

  Tcl_Interp* interp_;

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

  static constexpr const char* kEnableHighlightingKeyword = "highlighting";
  static constexpr const char* kEnableCompletionKeyword = "completion";
  static const int kCompleterMimimumLength = 2;
  static constexpr const char* kCommandRenamePrefix = "::tcl::openroad::";
};

}  // namespace gui
