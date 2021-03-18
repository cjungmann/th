#ifndef TH_H
#define TH_H

#include "bdb.h"
#include "ivtable.h"
#include "thesaurus.h"
#include "columnize.h"


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


// Columnize needs a CEIF instance for formatting and printing,
// Columnize CEIF Constituent functions:
int trec_get_len(const void *el);
int trec_print(FILE *f, const void *el);
int trec_print_cell(FILE *f, const void *el, int width);

extern const CEIF ceif_trec;

// TREC sorting functions
int trec_sort_alpha(const void *left, const void *right);   // alphabetical order
int trec_sort_t_freq(const void *left, const void *right);  // by thesaurus frequence
int trec_sort_g_freq(const void *left, const void *right);  // by google frequency


// th2.c
void resort_trec_list(void* recs, int count, int order);
void resort_params(PPARAMS *params, int order);
void result_trec_user(const TREC **list, int length, void *closure);
void thesaurus_result_user(TTABS *ttabs, TRESULT *tresult, void *closure);
int show_thesaurus_word(const char *word);

// th_test.c
void run_stack_report(const char *word);




#endif
