#ifndef TH_H
#define TH_H

#include "bdb.h"
#include "ivtable.h"
#include "thesaurus.h"


struct stwc_closure {
   TTABS      *ttabs;
   const char *word;
   RecID      id;
};

/**
 * Function pointer types for callbacks
 */
typedef void (*word_list_user)(const char **list, int length, void *closure);
typedef void (*trec_list_user)(const TREC **list, int length, void *closure);

void build_trec_list_alloca(TTABS *ttabs, RecID *list, int len, trec_list_user user, void *closure);

// th2.c
int new_show_thesaurus_word(const char *word);




#endif
