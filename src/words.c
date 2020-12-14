#include "bdb.h"
#include "ivtable.h"
#include "thesaurus.h"
#include "parse_thesaurus.h"
#include <string.h>
#include <readargs.h>

const char *thesaurus_name="thesaurus";
const char *thesaurus_word=NULL;
int thesaurus_recid=0;

bool flag_import_thesaurus = 0;
bool flag_dump_thesaurus = 0;

int import_thesaurus(void)
{
   int retval = 1;
   FILE *f = fopen("files/mthesaur.txt", "r");
   if (f)
   {
      Result result;
      TTABS ttabs;
      
      TTB.init(&ttabs);
      if (!(result = TTB.open(&ttabs, thesaurus_name)))
      {
         read_thesaurus_file(f, save_thesaurus_word, &ttabs);
         TTB.close(&ttabs);
      }

      fclose(f);
   }

   return retval;
}

bool dump_thesaurus(void)
{
   Result result;
   IVTable ivt;
   init_IVTable(&ivt);
   if (!(result = ivt.open(&ivt, thesaurus_name)))
   {
      dump_table(&ivt.t_records, thesaurus_dumpster, NULL);

      ivt.close(&ivt);
      return 0;
   }
   else
   {
      DB *db = ivt.get_records_db(&ivt);
      db->err(db, result, "Unable to open thesaurus \"%s\".", thesaurus_name);
   }

   return 1;
}

void show_thesaurus_word_callback(RecID *list, int length, void *data)
{
   TTABS *ttabs = (TTABS*)data;
   RecID *listend = list + length;

   const char *colors[] = {
      "\x1b[31;1m",
      "\x1b[32;1m",
      "\x1b[33;1m",
      "\x1b[34;1m",
      "\x1b[35;1m",
      "\x1b[36;1m",
      "\x1b[37;1m"
   };

   char buff[64];
   TREC *trec = (TREC*)buff;

   Result result;
   int counter = 0;
   int color_count = sizeof(colors) / sizeof(colors[0]);
   const char *color;

   while (list < listend)
   {
      color = colors[counter++ % color_count];
      if (!(result = TTB.get_word_rec(ttabs, *list, trec, sizeof(buff))))
         printf(" %s%s\x1b[m", color, trec->value);
      else
         printf(" %s%u\x1b[m", color, *list);
      
      ++list;
   }

   printf("\n");
}

int show_thesaurus_word(const char *word)
{
   // Eventually, we'll show all the words, so we need to
   // use all the resources in the Thesaurus-TABleS
   Result result;
   TTABS ttabs;
      
   TTB.init(&ttabs);
   if (!(result = TTB.open(&ttabs, thesaurus_name)))
   {
      RecID id = TTB.lookup(&ttabs, word);
      if (id)
         TTB.get_words(&ttabs, id, show_thesaurus_word_callback);
      else
         fprintf(stderr, "Failed to find \"%s\" in the thesaurus.\n", word);

      TTB.close(&ttabs);
   }

   return 0;
}


int thesaurus_word_by_recid(int recid)
{
   int retval = 1;
   
   // Only showing the word for now, use
   // the small part of thesaurus data
   Result result;
   IVTable ivt;
   init_IVTable(&ivt);
   if ((result = ivt.open(&ivt, thesaurus_name)))
      goto abandon_function;

   DBT value;
   memset(&value, 0, sizeof(DBT));
   if ((result = ivt.get_record_by_recid(&ivt, recid, &value)))
   {
      if (result == DB_NOTFOUND)
         printf("RecID %d was not found.\n", recid);
      else
      {
         DB *db = ivt.get_records_db(&ivt);
         db->err(db, result, "Index search failed.");
      }
   }
   else
   {
      TREC *trec = (TREC*)value.data;
      const char *str = (const char *)value.data + sizeof(TREC);
      int lenstr = value.size - sizeof(TREC);
      printf("%.*s (x%u)\n", lenstr, str, trec->count);
      retval = 0;
   }

   ivt.close(&ivt);

  abandon_function:
   return retval;
}

raAction actions[] = {
   {'h', "help", "This help display", &ra_show_help_agent },

   {'i', "id", "Search thesaurus word by id", &ra_int_agent, &thesaurus_recid },

   {'T', "import_thesaurus", "Import thesaurus contents", &ra_flag_agent, &flag_import_thesaurus },
   {'t', "thesaurus_word", "Word to be sought in thesaurus", &ra_string_agent, &thesaurus_word },

   {'f', "thesaurus_name", "Base name of thesaurus database", &ra_string_agent, &thesaurus_name },
   {'d', "thesaurus_dump", "Dump contents of thesaurus database", &ra_flag_agent, &flag_dump_thesaurus },
   {-1, "*word", "Show thesaurus entry", &ra_string_agent, &thesaurus_word }
};

int main(int argc, const char **argv)
{
   ra_set_scene(argv, argc, actions, ACTS_COUNT(actions));

   if (ra_process_arguments())
   {
      if (flag_import_thesaurus)
         return import_thesaurus();
      else if (flag_dump_thesaurus)
         return dump_thesaurus();
      else if (thesaurus_word)
         return show_thesaurus_word(thesaurus_word);
      else if (thesaurus_recid)
         return thesaurus_word_by_recid(thesaurus_recid);
   }

   return 0;
}
