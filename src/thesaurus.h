#ifndef THESAURUS_H
#define THESAURUS_H

#include "bdb.h"

typedef struct _tword_head {
   bool     is_root;
   uint32_t count;
   char     value[];
} TREC;

typedef struct _thesaurus_tables {
   IVTable ivt;
   DB      *db_r2w;
   DB      *db_w2r;
} TTABS;

typedef void (*word_list_user)(RecID *list, RecID *end, void *closure);

typedef void   (*ttabs_init)     (TTABS *ttabs);
typedef Result (*ttabs_open)     (TTABS *ttabs, const char *name);
typedef void   (*ttabs_close)    (TTABS *ttabs);
typedef Result (*ttabs_add_word) (TTABS *ttabs, const char *str, int size, bool newline);
typedef Result (*ttabs_get_word) (TTABS *ttabs, RecID id, char *buffer, DataSize size);
typedef RecID  (*ttabs_lookup)   (TTABS *ttabs, const char *str);
typedef Result (*ttabs_get_words)(TTABS *ttabs, RecID id, word_list_user user);

bool save_thesaurus_word(const char *str, int size, bool newline, void *data);
bool thesaurus_dumpster(DBT* key, DBT* value, void *data);

typedef struct ttabs_class {
   ttabs_init      init;
   ttabs_open      open;
   ttabs_close     close;
   ttabs_add_word  add_word;
   ttabs_get_word  get_word;
   ttabs_lookup    lookup;
   ttabs_get_words get_words;
} TTABS_Class;

extern TTABS_Class TTB;






#endif
