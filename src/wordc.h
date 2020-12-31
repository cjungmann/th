#ifndef WORDC_H
#define WORDC_H

#include "bdb.h"

typedef uint32_t Rank;
typedef uint64_t Freq;

typedef struct wordc_rec{
   Freq count;
   Rank rank;
   RecID    wordid;
} WCR;

typedef struct wordc_class {
   DB *db;
   Freq static_last_count;
   int static_position;
   Rank static_rank;
} WCC;

void wcc_interpret_string_number(const char /*in*/ *start, const char /*in*/ *end,
                                 const char /*out*/ **word_end, Freq /*out*/ *number);

Result wcc_open(WCC *wcc, const char *name, int create);
bool wcc_add_word(const char *line, const char *end, void *closure);
Result wcc_get_word(WCC *wcc, const char *word, Rank *rank, Freq *count);
void wcc_close(WCC *wcc);


/* typedef bool (*line_user)(const char *line, const char *end, void *closure); */
/* void read_file_lines(int fh, line_user user, void *closure); */


#endif
