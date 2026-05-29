/* Clean-room stub of the GNU History interface -- NOT GNU Readline.
 * See readline.h for rationale. SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef OPENROAD_MOCK_READLINE_HISTORY_H_
#define OPENROAD_MOCK_READLINE_HISTORY_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void* histdata_t;

typedef struct _hist_entry
{
  char* line;
  char* timestamp;
  histdata_t data;
} HIST_ENTRY;

HIST_ENTRY** history_list(void);

void add_history(const char* line);
void clear_history(void);
int where_history(void);
int read_history(const char* filename);
int write_history(const char* filename);
int append_history(int nelements, const char* filename);
int history_truncate_file(const char* filename, int nlines);

#ifdef __cplusplus
}
#endif

#endif /* OPENROAD_MOCK_READLINE_HISTORY_H_ */
