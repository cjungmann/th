#ifndef THESAURUS_H
#define THESAURUS_H

#include "bdb.h"

extern const char *thesaurus_name;

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

typedef void (*recid_list_user)(TTABS *ttabs, RecID *list, int len, void *closure);
typedef void (*tword_user)(TTABS *ttabs, RecID id, TREC *trec, void *closure);

typedef void   (*ttabs_init)           (TTABS *ttabs);
typedef Result (*ttabs_open)           (TTABS *ttabs, const char *name, bool create);
typedef void   (*ttabs_close)          (TTABS *ttabs);
typedef Result (*ttabs_add_word)       (TTABS *ttabs, const char *str, int size, bool newline);
typedef Result (*ttabs_get_word_rec)   (TTABS *ttabs, RecID id, TREC *buffer, DataSize size);
typedef RecID  (*ttabs_lookup)         (TTABS *ttabs, const char *str);
typedef Result (*ttabs_get_words)      (DB *db, TTABS *ttabs, RecID id,
                                        recid_list_user user, void *closure);
typedef Result (*ttabs_walk_entries)   (TTABS *ttabs, tword_user user, void *closure);
typedef int    (*ttabs_count_synonyms) (TTABS *ttabs, RecID id);

bool save_thesaurus_word(const char *str, int size, bool newline, void *data);
bool thesaurus_dumpster(DBT* key, DBT* value, void *data);

typedef struct ttabs_class {
   ttabs_init           init;
   ttabs_open           open;
   ttabs_close          close;
   ttabs_add_word       add_word;
   ttabs_get_word_rec   get_word_rec;
   ttabs_lookup         lookup;
   ttabs_get_words      get_words;
   ttabs_walk_entries   walk_entries;
   ttabs_count_synonyms count_synonyms;
} TTABS_Class;

extern TTABS_Class TTB;

Result open_existing_thesaurus(TTABS *ttabs);






#endif
