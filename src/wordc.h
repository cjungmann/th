#ifndef WORDC_H
#define WORDC_H

#include "bdb.h"

typedef struct wordc_rec{
   uint64_t count;
} WCR;

typedef struct wordc_class {
   DB *db;
   
} WCC;


Result wcc_open(WCC *wcc, const char *name, int create);
Result wcc_add_word(WCC *wcc, const char *line, const char *end);

/* typedef bool (*line_user)(const char *line, const char *end, void *closure); */
/* void read_file_lines(int fh, line_user user, void *closure); */


#endif
