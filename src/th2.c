#include <stdlib.h>   // for qsort function

#include "utils.h"
#include "term.h"

#include "get_keypress.h"
#include "columnize.h"
#include "prompter.h"

#include "th.h"



/*
 * Extend list of options for custom features.
 */
enum poptions {
   PR_OPTIONS = CPR_CUSTOM + 1,
   PR_NEWSPAPER,
   PR_PARALLEL,
   PR_ALPHABETIC,
   PR_GOOGLE_F,
   PR_THESAURUS_F,
   PR_RETURN
};


const PUnit main_menu_units[] = {
   { "&first",    CPR_FIRST },
   { "&previous", CPR_PREVIOUS },
   { "&next",     CPR_NEXT },
   { "&last",     CPR_LAST },
   { "&options",  PR_OPTIONS },
   { "&quit",     CPR_QUIT }
};
const PMenu main_menu = { main_menu_units, ARRLEN(main_menu_units) };

const PUnit options_menu_units[] = {
   { "&newspaper flow",      PR_NEWSPAPER },
   { "&parallel flow",       PR_PARALLEL },
   { "&alphabetic",          PR_ALPHABETIC },
   { "&google frequency",    PR_GOOGLE_F },
   { "&thesaurus frequency", PR_THESAURUS_F },
   { "&return",              PR_RETURN },
   { "&quit",                CPR_QUIT }
};
const PMenu options_menu = { options_menu_units, ARRLEN(options_menu_units) };

void resort_trec_list(PPARAMS *params, int order)
{
   int (*sorter)(const void *left, const void *right) = NULL;
   switch(order)
   {
      case PR_ALPHABETIC:
         sorter = trec_sort_alpha;
         break;
      case PR_GOOGLE_F:
         sorter = trec_sort_g_freq;
         break;
      case PR_THESAURUS_F:
         sorter = trec_sort_t_freq;
         break;
   }
   void *list = (void*)params->start;
   int length = params->end - params->start;
   qsort(list, length, sizeof(TREC*),  sorter);
}

void result_trec_user(const TREC **list, int length, void *closure)
{
   struct stwc_closure *stwc = (struct stwc_closure*)closure;

   int maxlen = columnize_get_max_len(&ceif_trec, (const void **)list, (const void**)list+length);
   PPARAMS params;
   PPARAMS_init(&params,
                (const void **)list, length,
                2,       // gutter size
                4,       // reserve lines (one at top, three below for prompt/menu)
                maxlen);
   PPARAMS_query_screen(&params);

   flow_function_f flower = display_newspaper_columns;
   const PMenu *curmenu = &main_menu;

   const void **ptr = PPARAMS_first(&params);
   const void **newptr;
   while (1)
   {
      curmenu = &main_menu;

      prompter_reuse_line();
      printf(" - - - - - Synonyms for \x1b[33;1m%s\x1b[m - - - - -\n", stwc->word);
      const void **stop = (*flower)(&ceif_trec,
                                    ptr,
                                    params.end,
                                    params.gutter,
                                    params.columns_to_show,
                                    params.lines_to_show);

      printf("Synonyms for \x1b[33;1m%s\x1b[m: ", stwc->word);
      columnize_print_progress_line(&params, stop);

     recheck_user_response:
      prompter_reuse_line();
      prompter_pmenu_print(curmenu);
      int result = prompter_pmenu_await(curmenu);
      switch(result)
      {
         case CPR_QUIT:
            goto exit_function;
         case PR_NEWSPAPER:
            flower = display_newspaper_columns;
            ptr = PPARAMS_move(&params, CPR_FIRST);
            break;
         case PR_PARALLEL:
            flower = display_parallel_columns;
            ptr = PPARAMS_move(&params, CPR_FIRST);
            break;
         case CPR_FIRST:
         case CPR_PREVIOUS:
         case CPR_NEXT:
         case CPR_LAST:
            newptr = PPARAMS_move(&params, result);
            // Don't replot for unchanged position
            if (newptr == ptr)
               goto recheck_user_response;
            else
            {
               ptr = newptr;
               break;
            }
         case PR_ALPHABETIC:
         case PR_GOOGLE_F:
         case PR_THESAURUS_F:
            resort_trec_list(&params, result);
            ptr = PPARAMS_move(&params, CPR_FIRST);
            break;
         case PR_OPTIONS:
            curmenu = &options_menu;
            goto recheck_user_response;
         case PR_RETURN:
            curmenu = &main_menu;
            goto recheck_user_response;
      }
   }

  exit_function:
   prompter_reuse_line();
}

void thesaurus_result_user(TTABS *ttabs, TRESULT *tresult, void *closure)
{
   build_trec_list_alloca(ttabs,
                          tresult->entries,
                          tresult->entries_count,
                          result_trec_user,
                          closure);
}


int show_thesaurus_word(const char *word)
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
         TTB.get_result(&ttabs, id, thesaurus_result_user, &closure);
      }
      else
         fprintf(stderr, "Failed to find \"%s\" in the thesaurus.\n", word);

      TTB.close(&ttabs);
   }

   return 0;

}
