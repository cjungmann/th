#ifndef READ_FILE_LINES_H
#define READ_FILE_LINES_H

// Default value is 2048.  Adjust buffer length according to your needs
extern unsigned RFL_bufflen;

// Using *bool* type to communicate the expected return value,
// 0 for failure, non-zero for success.
/* typedef int bool; */
#include "bdb.h"

typedef bool (*line_user)(const char *start, const char *end, void *closure);

int read_handle_lines(int fh, line_user user, void *closure);
int read_file_lines(const char *filepath, line_user user, void *closure);


#endif
