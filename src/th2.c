#include "utils.h"
#include "term.h"

#include "get_keypress.h"
#include "columnize.h"

#include "th.h"

typedef struct thesaurus_result_closure {
   struct stwc_closure stwc;
   COLDIMS *dims;
} TRCLOS;



CPRD new_th_page_control(int page_current, int page_count, COLDIMS *dims)
{
   TRCLOS *trclos = (TRCLOS*)dims->closure;

   printf("\x1b[34;1m%s\x1b[m related words (page %d of %d)\n"
          "\x1b[34;1mf\x1b[m" "irst "
          "\x1b[34;1mp\x1b[m" "revious "
          "\x1b[34;1mn\x1b[m" "ext "
          "\x1b[34;1ml\x1b[m" "ast "
          "\x1b[34;1mq\x1b[m" "uit",
          trclos->stwc.word,
          page_current,
          page_count
      );

   fflush(stdout);     // Force printing without newline
   printf("\x1b[1G"); // Move cursor to column 1

   const char *keys[] = { "q", "f", "p", "n", "l" };
   int index = await_keypress(keys, sizeof(keys)/sizeof(keys[0]));

   // Erase screen unless exiting
   if (index > 0)
      printf("\x1b[2K");

   switch(index)
   {
      case 0: return CPR_QUIT;
      case 1: return CPR_FIRST;
      case 2: return CPR_PREVIOUS;
      case 3: return CPR_NEXT;
      case 4: return CPR_LAST;
      default: return CPR_NO_RESPONSE;
   }
}

void result_trec_user(const TREC **list, int length, void *closure)
{
}
   

void thesaurus_result_user(TTABS *ttabs, TRESULT *tresult, void *closure)
{
   
}


int new_show_thesaurus_word(const char *word)
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
         COLDIMS dims;
         struct stwc_closure closure = { &ttabs, word, id };
         columnize_default_dims(&dims, &closure);
         dims.reserve_lines = 3;
         dims.pcontrol = new_th_page_control;

         TTB.get_result(&ttabs, id, thesaurus_result_user, &dims);
      }
      else
         fprintf(stderr, "Failed to find \"%s\" in the thesaurus.\n", word);

      TTB.close(&ttabs);
   }

   return 0;

}
