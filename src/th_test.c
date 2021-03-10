#include <string.h>

#include "thesaurus.h"
#include "th.h"
#include "commaize.h"

#define __USE_MISC
#include <stdlib.h>  // for qsort, alloca unlocked with __USE_MISC for BSD

/**
 * This "closure" contains all the variables needed by
 * the recursive function *word_list_recursor*.  Using
 * a pointer to an instance of this struct should minimize
 * the stack frame penalty of the deeply-recursuve function.
 */
typedef struct recurse_closure {
   TTABS          *ttabs;
   IVTable        *ivt;
   RecID          *list;
   RecID          *ptr;
   RecID          *lend;
   const char     **wlist;
   const char     **wptr;
   int            len;
   word_list_user user;
   void           *closure;
   DBT            *value;

   // Work variables for use in word_list_recursor()
   Result         result;
   TREC           *trec;
   size_t         word_size;
   char           *buffer;
} REc;


/**
 * Using recursion and VLA instead of *alloca* to make a list
 * of related words.
 */
void word_list_recursor(REc *rec)
{
   if (rec->ptr < rec->lend)
   {
      rec->result = rec->ivt->get_record_by_recid(rec->ivt, *rec->ptr, rec->value);

      if (rec->result)
         fprintf(stderr, "Failed to retrieve word number %u.\n", *rec->ptr);
      else if (rec->value->size > 0)
      {
         rec->trec = (TREC*)rec->value->data;
         rec->word_size = rec->value->size - sizeof(TREC);

         // make copy of word
         char buffer[rec->word_size + 1];
         /* char *buffer = (char*)alloca(rec->word_size + 1); */
         memcpy(buffer, rec->trec->value, rec->word_size);
         buffer[rec->word_size] = '\0';

         // Save word
         *rec->wptr = buffer;

         rec->buffer = buffer;

         // (redundant but safe) clear closure of current stack's data:
         /* rec->result = 0; */
         /* rec->trec = NULL; */
         /* rec->word_size = 0; */
         /* rec->buffer = NULL; */

         // Update word-list pointer only if a new word was stored:
         rec->wptr++;
         rec->ptr++;
         word_list_recursor(rec);
      }
      else
      {
         rec->ptr++;
         word_list_recursor(rec);
      }
   }
   else
   {
      (*rec->user)(rec->wlist, rec->len, rec->closure);
   }
}

/**
 * Initialize the data and launch the recursive word compilation function.
 */
void build_word_list_recurse(TTABS *ttabs, RecID *list, int len, word_list_user user, void *closure)
{
   const char *wlist[len];
   DBT value;
   memset(&value, 0, sizeof(DBT));

   // This struct serves all computation needs of word_list_recursor()
   // in order to save stack space (only one parameter, no local variables).
   REc  rec = { ttabs, &ttabs->ivt, list, list, list+len, wlist, wlist, len, user, closure, &value };

   word_list_recursor(&rec);
}

/**
 * Related word-list collector, provides the list through
 * the callback function *user*.
 *
 * This function is much simpler than the recursion+VLA method,
 * and much more memory efficient, as comparison testing shows.
 */
void build_word_list_alloca(TTABS *ttabs, RecID *list, int len, word_list_user user, void *closure)
{
   IVTable *ivt = &ttabs->ivt;

   RecID *listend = list + len;

   const char *wlist[len];
   memset(wlist, 0, sizeof(wlist));
   const char **wptr = wlist;

   Result result;
   DBT value;
   memset(&value, 0, sizeof(DBT));

   while (list < listend)
   {
      if ((result = ivt->get_record_by_recid(ivt, *list, &value)))
         fprintf(stderr, "Failed to retrieve word number %u.\n", *list);
      else if (value.size > 0)
      {
         TREC *trec = (TREC*)value.data;
         int vsize = value.size - sizeof(TREC);
         char *buff = (char*)alloca(vsize + 1);
         memcpy(buff, trec->value, vsize);
         buff[vsize] = '\0';
         *wptr = buff;
         ++wptr;
      }

      ++list;
   }

   (*user)(wlist, len, closure);
}

void stack_report_final(const char **list, int length, void *closure)
{
   const char **stack_pointer = (const char **)closure;
   int last_stack;
   *stack_pointer = (const char*)&last_stack;
}

void stack_report_user(TTABS *ttabs, TRESULT *tresult, void *closure)
{
   struct stwc_closure *stwc = (struct stwc_closure*)closure;

   const char *stack_pointer_start = (const char *)&stwc;
   const char *stack_pointer_alloca = NULL;
   const char *stack_pointer_recurse = NULL;

   clock_t alloca_start = clock();

   build_word_list_alloca(ttabs,
                          tresult->entries,
                          tresult->entries_count,
                          stack_report_final,
                          &stack_pointer_alloca);

   clock_t alloca_end = clock();

   build_word_list_recurse(ttabs,
                          tresult->entries,
                          tresult->entries_count,
                          stack_report_final,
                          &stack_pointer_recurse);

   clock_t recurse_end = clock();

   long bytes_consumed_alloca = stack_pointer_start - stack_pointer_alloca;
   long bytes_consumed_recurse = stack_pointer_start - stack_pointer_recurse;

   clock_t ticks_alloca = alloca_end - alloca_start;
   if (ticks_alloca < 0)
      ticks_alloca = 0 - ticks_alloca;
   clock_t ticks_recurse = recurse_end - alloca_end;
   if (ticks_recurse < 0)
      ticks_recurse = 0 - ticks_recurse;

   printf("stack report for %s:\n", stwc->word);
   printf("  %d entries\n", tresult->entries_count);

   printf("  ");
   commaize_number(bytes_consumed_alloca);
   printf(" alloca method stack bytes in %ld ticks\n", (long)ticks_alloca);

   printf("  ");
   commaize_number(bytes_consumed_recurse);
   printf(" recurse/VLA method stack bytes in %ld ticks\n", (long)ticks_recurse);

   printf("  recurse/VLA method used %f times more stack memory than alloca method.\n", (float)bytes_consumed_recurse / (float)bytes_consumed_alloca);
   printf("  recurse/VLA method took %f times more clock ticks than alloca method.\n", (double)ticks_recurse / (double)ticks_alloca);
}

void run_stack_report(const char *word)
{
   // Eventually, we'll show all the words, so we need to
   // use all the resources in the Thesaurus-TABleS
   Result result;
   TTABS ttabs;
      
   TTB.init(&ttabs);
   if (!(result = open_existing_thesaurus(&ttabs)))
   {
      RecID id = TTB.lookup(&ttabs, word);

      if (id)
      {
         struct stwc_closure closure = { &ttabs, word, id };
         TTB.get_result(&ttabs, id, stack_report_user, &closure);
      }
      else
         fprintf(stderr, "Failed to find \"%s\" in the thesaurus.\n", word);

      TTB.close(&ttabs);
   }
}

