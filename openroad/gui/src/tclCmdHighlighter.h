// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <QRegularExpression>
#include <QString>
#include <QSyntaxHighlighter>
#include <QTextBlockUserData>
#include <QTextCharFormat>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace gui {

using QRegularExpressionPtr = std::unique_ptr<QRegularExpression>;
struct CommandRule
{
  QRegularExpressionPtr pattern;
  int command;  // command index from tclCmdInputWidget::commands_
};

struct ArgumentRule
{
  std::vector<QRegularExpressionPtr> rules;
  const QTextCharFormat* format;
};

using CommandRulePtr = std::unique_ptr<CommandRule>;
using ArgumentRulePtr = std::unique_ptr<ArgumentRule>;
struct CommandRuleGroup
{
  std::vector<CommandRulePtr> rules;
  const QTextCharFormat* format;
};

struct CommandArguments
{
  std::string command;              // name of command
  bool is_toplevel;                 // command is part of the toplevel namespace
  std::set<std::string> arguments;  // set of command arguments
};

struct TclCmdUserData : public QTextBlockUserData
{
  bool line_continued;
  std::set<int> commands;
};

class TclCmdHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT

 public:
  TclCmdHighlighter(QTextDocument* parent,
                    const std::vector<CommandArguments>& or_cmds,
                    const std::string& command_start,
                    const std::string& command_end);

 protected:
  void highlightBlock(const QString& text) override;

 private:
  void initFormats();

  void init(const std::vector<CommandArguments>& or_cmds,
            const std::string& start_of_command,
            const std::string& end_of_command);
  void initOpenRoad(const std::vector<CommandArguments>& or_cmds,
                    const std::string& start_of_command,
                    const std::string& end_of_command);
  void initTclKeywords(const std::string& start_of_command,
                       const std::string& end_of_command);
  void initOther();

  static std::string escape(const std::string& preregex);
  static CommandRulePtr buildKeywordRule(int command_id,
                                         const std::string& command,
                                         const std::string& start_of_command,
                                         const std::string& end_of_command,
                                         bool escape_cmd = true);
  static void buildKeywordsRule(std::vector<CommandRulePtr>& vec,
                                const std::vector<std::string>& commands,
                                const std::string& start_of_command,
                                const std::string& end_of_command);
  static CommandRulePtr buildRule(const std::string& pattern);
  static CommandRulePtr buildRule(int command_id, const std::string& pattern);
  static ArgumentRulePtr buildArgumentRule(const std::vector<std::string>& args,
                                           const QTextCharFormat* format);

  static void addRuleGroup(std::vector<CommandRuleGroup>& rule_group,
                           std::vector<CommandRulePtr>& rules,
                           const QTextCharFormat* format);

  void highlightBlockWithRules(const QString& text,
                               int start_idx,
                               const std::vector<CommandRuleGroup>& rules,
                               std::set<int>& matched_commands);
  int highlightBlockWithRule(const QString& text,
                             int start_idx,
                             const QRegularExpressionPtr& rule,
                             const QTextCharFormat* format);

  void highlightBlockWithString(const QString& text);

  std::vector<CommandRuleGroup> cmd_rules_;
  // general syntax rules
  std::vector<CommandRuleGroup> syntax_rules_;
  // string formatting, needs to be handled separately since it can span
  // multiple lines
  CommandRuleGroup string_rule_;

  std::map<int, ArgumentRulePtr> argument_rules_;

  // formatting
  QTextCharFormat openroad_cmd_format_;
  QTextCharFormat openroad_arg_format_;
  QTextCharFormat tcl_cmd_format_;
  QTextCharFormat brackets_format_;
  QTextCharFormat string_format_;
  QTextCharFormat variable_format_;
  QTextCharFormat comment_format_;
};

}  // namespace gui
