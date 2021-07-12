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

#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextBlockUserData>
#include <QTextCharFormat>

namespace gui {

using QRegularExpressionPtr = std::unique_ptr<QRegularExpression>;
struct CommandRule
{
  QRegularExpressionPtr pattern;
  int command; // command index
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

using CommandArguments = std::tuple<std::string, bool, std::set<std::string>>;

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
    ~TclCmdHighlighter();

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

    const std::string escape(const std::string& preregex);
    CommandRulePtr buildKeywordRule(const int command_id,
                                    const std::string& command,
                                    const std::string& start_of_command,
                                    const std::string& end_of_command);
    CommandRulePtr buildRule(const std::string& pattern);
    CommandRulePtr buildRule(const int command_id,
                             const std::string& pattern);
    ArgumentRulePtr buildArgumentRule(const std::vector<std::string>& args,
                                      const QTextCharFormat* format);

    void addRuleGroup(std::vector<CommandRuleGroup>& rule_group,
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
    // string formatting, needs to be handled separately since it can span multiple lines
    CommandRuleGroup string_rule;

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
