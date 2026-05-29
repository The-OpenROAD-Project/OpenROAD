/* Clean-room no-op implementations of the readline/history symbols abc and
 * yosys link against. readline() returns NULL (EOF), so any REPL loop exits
 * immediately -- correct for the batch-only CI build, which never reaches it.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <readline/history.h>
#include <readline/readline.h>
#include <stddef.h>

char* rl_line_buffer = NULL;
const char* rl_readline_name = "";
const char* rl_basic_word_break_characters = "";
rl_completion_func_t* rl_attempted_completion_function = NULL;

char* readline(const char* prompt)
{
  (void) prompt;
  return NULL;
}

char** rl_completion_matches(const char* text, rl_compentry_func_t* entry_func)
{
  (void) text;
  (void) entry_func;
  return NULL;
}

HIST_ENTRY** history_list(void)
{
  static HIST_ENTRY* empty[1] = {NULL};
  return empty;
}

void add_history(const char* line)
{
  (void) line;
}

void clear_history(void)
{
}

int where_history(void)
{
  return 0;
}

int read_history(const char* filename)
{
  (void) filename;
  return 0;
}

int write_history(const char* filename)
{
  (void) filename;
  return 0;
}

int append_history(int nelements, const char* filename)
{
  (void) nelements;
  (void) filename;
  return 0;
}

int history_truncate_file(const char* filename, int nlines)
{
  (void) filename;
  (void) nlines;
  return 0;
}
