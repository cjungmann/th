#ifndef THESAURUS_H
#define THESAURUS_H

#include "bdb.h"

typedef struct _tword_head {
   bool     is_root;
   uint32_t count;
   char     value[];
} TREC;


int save_thesaurus_word(const char *str, int size, int newline, void *data);
bool thesaurus_dumpster(DBT* key, DBT* value, void *data);



#endif
