#include "bdb.h"
#include "ivtable.h"
#include "thesaurus.h"
#include "parse_thesaurus.h"
#include <readargs.h>

const char *thesaurus_name="thesaurus";
const char *thesaurus_word=NULL;

bool flag_import_thesaurus = 0;
bool flag_dump_thesaurus = 0;

int import_thesaurus(void)
{
   int retval = 1;
   FILE *f = fopen("files/mthesaur.txt", "r");
   if (f)
   {
      Result result;
      IVTable ivt;
      init_IVTable(&ivt);
      if (!(result = ivt.open(&ivt, thesaurus_name)))
      {
         read_thesaurus_file(f, save_thesaurus_word, &ivt);
         ivt.close(&ivt);
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





raAction actions[] = {
   {'h', "help", "This help display", &ra_show_help_agent },
   {'T', "import_thesaurus", "Import thesaurus contents", &ra_flag_agent, &flag_import_thesaurus },
   {'t', "thesaurus word", "Word to be sought in thesaurus", &ra_string_agent, &thesaurus_word },

   {'f', "thesaurus_name", "Base name of thesaurus database", &ra_string_agent, &thesaurus_name },
   {'d', "thesaurus_dump", "Dump contents of thesaurus database", &ra_flag_agent, &flag_dump_thesaurus }
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
   }

   return 0;
}
