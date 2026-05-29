/* Clean-room stub of the GNU Readline interface -- NOT GNU Readline.
 *
 * Declares only the entry points OpenROAD's vendored abc/yosys reference so
 * the batch (non-interactive) build links without the GPL readline library
 * and its ncurses -> sed transitive chain. Authored from the public API
 * surface; no readline source was copied.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef OPENROAD_MOCK_READLINE_READLINE_H_
#define OPENROAD_MOCK_READLINE_READLINE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef char* rl_compentry_func_t(const char*, int);
typedef char** rl_completion_func_t(const char*, int, int);

extern char* rl_line_buffer;
extern const char* rl_readline_name;
extern const char* rl_basic_word_break_characters;
extern rl_completion_func_t* rl_attempted_completion_function;

char* readline(const char* prompt);
char** rl_completion_matches(const char* text, rl_compentry_func_t* entry_func);

#ifdef __cplusplus
}
#endif

#endif /* OPENROAD_MOCK_READLINE_READLINE_H_ */
