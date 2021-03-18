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
   PR_FLOW_MENU,
   PR_SORT_MENU,
   PR_NEWSPAPER,
   PR_PARALLEL,
   PR_ALPHABETIC,
   PR_GOOGLE_F,
   PR_THESAURUS_F,
   PR_BRANCHES,
   PR_TRUNKS,
   PR_RETURN
};


const PUnit main_menu_units[] = {
   { "&first",    CPR_FIRST },
   { "&previous", CPR_PREVIOUS },
   { "&next",     CPR_NEXT },
   { "&last",     CPR_LAST },
   /* { "&options",  PR_OPTIONS }, */
   { "&quit",     CPR_QUIT }
};
const PMenu main_menu = { main_menu_units, ARRLEN(main_menu_units) };

const PUnit trunks_menu_units[] = {
   { "&first",    CPR_FIRST },
   { "&previous", CPR_PREVIOUS },
   { "&next",     CPR_NEXT },
   { "&last",     CPR_LAST },
   { "&branches", PR_BRANCHES },
   { "&quit",     CPR_QUIT }
};
const PMenu trunks_menu = { trunks_menu_units, ARRLEN(trunks_menu_units) };

const PUnit branches_menu_units[] = {
   { "&first",    CPR_FIRST },
   { "&previous", CPR_PREVIOUS },
   { "&next",     CPR_NEXT },
   { "&last",     CPR_LAST },
   { "&trunks",   PR_TRUNKS },
   { "&quit",     CPR_QUIT }
};
const PMenu branches_menu = { branches_menu_units, ARRLEN(branches_menu_units) };

const PUnit options_menu_units[] = {
   { "&flow",    PR_FLOW_MENU },
   { "&sort",    PR_SORT_MENU },
   { "&return",  PR_RETURN },
   { "&quit",    CPR_QUIT }
};
const PMenu options_menu = { options_menu_units, ARRLEN(options_menu_units) };

const PUnit flow_menu_units[] = {
   { "&newspaper flow", PR_NEWSPAPER },
   { "&parallel flow",  PR_PARALLEL },
   { "&return",         PR_RETURN },
   { "&quit",           CPR_QUIT }
};
const PMenu flow_menu = { flow_menu_units, ARRLEN(flow_menu_units) };

const PUnit sort_menu_units[] = {
   { "&alphabetic",     PR_ALPHABETIC },
   { "&google freq",    PR_GOOGLE_F },
   { "&thesaurus freq", PR_THESAURUS_F },
   { "&return",         PR_RETURN },
   { "&quit",           CPR_QUIT }
};
const PMenu sort_menu = { sort_menu_units, ARRLEN(sort_menu_units) };

void resort_trec_list(void* recs, int count, int order)
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
   qsort(recs, count, sizeof(TREC*),  sorter);
}

void resort_params(PPARAMS *params, int order)
{
   resort_trec_list(params->start,
                    params->end - params->start,
                    order);
}

void thesaurus_result_user(TTABS *ttabs, TRESULT *tresult, void *closure)
{
   struct stwc_closure *stwc = (struct stwc_closure*)closure;

   // Alphabetize lists outside of loop:
   resort_trec_list(tresult->entries,
                    tresult->entries_count,
                    PR_ALPHABETIC);
   
   resort_trec_list(tresult->roots,
                    tresult->roots_count,
                    PR_ALPHABETIC);


   // No longer supporting flow change.
   flow_function_f flower = display_newspaper_columns;

   enum display_modes { DM_SAME, DM_BRANCHES, DM_TRUNKS };
   bool display_mode = DM_BRANCHES;

   // display_mode-dependent variables
   PPARAMS params;
   const TREC **list;
   int list_len;
   const PMenu *curmenu = NULL;

   // page-dependent variables
   const void **ptr;
   const void **newptr;

   while (1)
   {
      if (display_mode != DM_SAME)
      {
         if (display_mode == DM_BRANCHES)
         {
            list = (const TREC**)&tresult->entries;
            list_len = tresult->entries_count;
            curmenu = &branches_menu;
         }
         else if (display_mode == DM_TRUNKS)
         {
            list = (const TREC**)&tresult->roots;
            list_len =  tresult->roots_count;
            curmenu = &trunks_menu;
         }

         // Calculate columns and lines to display base on screen dimensions,
         // max string length, and lines to reserve for legend stuff:
         ptr = PPARAMS_first(&params);

         int maxlen = columnize_get_max_len(&ceif_trec,
                                            (const void **)list,
                                            (const void**)list + list_len);

         PPARAMS_init(&params,
                      (const void **)list, list_len,
                      2,       // gutter size
                      4,       // reserve lines (one at top, three below for prompt/menu)
                      maxlen);
         PPARAMS_query_screen(&params);
      }

      
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

            /* Navigation actions */
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
            
            /* Menu-switching actions */
         case PR_OPTIONS:
            curmenu = &options_menu;
            goto recheck_user_response;
         case PR_FLOW_MENU:
            curmenu = &flow_menu;
            goto recheck_user_response;
         case PR_SORT_MENU:
            curmenu = &sort_menu;
            goto recheck_user_response;
         case PR_RETURN:
            curmenu = &main_menu;
            goto recheck_user_response;

         case PR_BRANCHES:
            display_mode = DM_BRANCHES;
            break;
         case PR_TRUNKS:
            display_mode = DM_TRUNKS;
            break;

            /* Flow menu actions */
         case PR_NEWSPAPER:
            flower = display_newspaper_columns;
            ptr = PPARAMS_move(&params, CPR_FIRST);
            break;
         case PR_PARALLEL:
            flower = display_parallel_columns;
            ptr = PPARAMS_move(&params, CPR_FIRST);
            break;

            /* Sort menu actions */
         case PR_ALPHABETIC:
         case PR_GOOGLE_F:
         case PR_THESAURUS_F:
            resort_params(&params, result);
            ptr = PPARAMS_move(&params, CPR_FIRST);
            break;
      }
   }

  exit_function:
   prompter_reuse_line();
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
