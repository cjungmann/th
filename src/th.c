#include <string.h>
#include <strings.h>  // for strcasecmp()
#include <readargs.h>
#include <limits.h>
#include <assert.h>

#define __USE_MISC
#include <stdlib.h>  // for qsort, alloca unlocked with __USE_MISC for BSD
/* #include <alloca.h> */

#include "bdb.h"
#include "ivtable.h"
#include "thesaurus.h"
#include "parse_thesaurus.h"
#include "utils.h"
#include "term.h"
#include "read_file_lines.h"
#include "wordc.h"

#include "get_keypress.h"
#include "columnize.h"

#include "th.h"

// thesaurus_name moved to thesaurus.c
const char *thesaurus_word=NULL;
int thesaurus_recid=0;

const char *freq_word = NULL;

bool flag_import_thesaurus = 0;
bool flag_dump_thesaurus = 0;
bool flag_import_frequencies = 0;
bool flag_update_freq = 0;
bool flag_stack_report = 0;
bool flag_enumerate = 0;
bool flag_verbose = 0;

const char *path_thesaurus_source = "files/mthesaur.txt";
const char *path_frequency_source = "files/count_1w.txt";

const char *get_importing_thesaurus_name(void)
{
   return thesaurus_name ? thesaurus_name : "thesaurus";
}

/**
 * Fulfills command line option -i.
 *
 * Typically, this will be called only once, but it may also
 * be used repeatedly while experimenting with how the data
 * is organized.
 */
int import_thesaurus(void)
{
   int retval = 1;
   FILE *f = fopen(path_thesaurus_source, "r");
   if (f)
   {
      Result result;
      const char *tname = get_importing_thesaurus_name();

      TTABS ttabs;
      TTB.init(&ttabs);
      if (!(result = TTB.open(&ttabs, tname, 1)))
      {
         WCC wcc;
         memset(&wcc, 0, sizeof(WCC));
         if (!(result = wcc_open(&wcc, "wordc.db", 0)))
         {
            ttabs.rankobj = &wcc;
            ttabs.ranker = wcc_ranker;
         }

         read_thesaurus_file(f, flag_verbose, save_thesaurus_word, &ttabs);
         retval = 0;

         if (ttabs.rankobj)
            wcc_close(&wcc);
         
         TTB.close(&ttabs);
      }

      fclose(f);
   }

   return retval;
}

/**
 * Fulfills command-line option -d
 *
 * This is strictly a debugging function.  It changes as
 * necessary to accomplish temporary debugging goals.
 */
bool dump_thesaurus(void)
{
   Result result;
   const char *tname = get_importing_thesaurus_name();

   IVTable ivt;
   init_IVTable(&ivt);
   if (!(result = ivt.open(&ivt, tname, 0)))
   {
      dump_table(&ivt.t_records, thesaurus_dumpster, NULL);

      ivt.close(&ivt);
      return 0;
   }
   else
   {
      DB *db = ivt.get_records_db(&ivt);
      db->err(db, result, "Unable to open thesaurus \"%s\".", tname);
   }

   return 1;
}

/**
 * intermediary callback function for verbose mode while
 * importing word frequency.
 */
bool verbose_wcc_add_word(const char *line, const char *end, void *closure)
{
   const char *end_word;
   Freq vcount;

   wcc_interpret_string_number(line, end, &end_word, &vcount);

   printf("\x1b[1G\x1b[K%.*s", (int)(end_word-line), line);

   return wcc_add_word(line, end, closure);
}

/**
 * Reads word-count data from file, building a table
 * with word frequency and rank.
 *
 * Uses wordc.c and read_file_lines.c, accessed through
 * wordc.h and read_file_lines.h.
 */
int import_frequencies(void)
{
   Result result;
   WCC wcc;
   memset(&wcc, 0, sizeof(WCC));
   if (!(result = wcc_open(&wcc, "wordc.db", 1)))
   {
      line_user_f user = flag_verbose ? verbose_wcc_add_word : wcc_add_word;
      result = read_file_lines(path_frequency_source, user, &wcc);
      wcc_close(&wcc);

      // Print newline if verbose mode repeatedly used same line for progress.
      if (flag_verbose)
         printf("\n");
   }

   if (result)
   {
      fprintf(stderr, "Thesaurus import failed: %s.\n", db_strerror(result));
      return 1;
   }
   else
      return 0;
}


void UTWF_user(TTABS *ttabs, RecID id, TREC *trec, int word_len, void *closure)
{
   /* WCC *wcc = (WCC*)closure; */

   // %* or %*. denote the length, in this case adding padding spaces.
   printf("%*s", (45-word_len), "");
   // %.* denotes the precision, which we need to print the word.
   // Using *. will yield a zero-length string.
   printf("%.*s: %u\n",
          word_len, trec->value,
          trec->count);
}


int update_thesaurus_word_frequencies(void)
{
   Result result;
   const char *tname = get_importing_thesaurus_name();

   TTABS ttabs;
   memset(&ttabs, 0, sizeof(TTABS));
   TTB.init(&ttabs);
   if ((result = TTB.open(&ttabs, tname, 0)))
   {
      fprintf(stderr, "Failed to open thesaurus database %s: %s.\n",
              tname,
              db_strerror(result));
      goto abandon_function;
   }

   WCC wcc;
   memset(&wcc, 0, sizeof(WCC));

   if ((result = wcc_open(&wcc, "wordc.db", 0)))
   {
      fprintf(stderr, "Failed to open word frequency databse %s: %s.\n",
              tname,
              db_strerror(result));
      goto abandon_ttabs;
   }

   TTB.walk_entries(&ttabs, UTWF_user, &wcc);

   wcc_close(&wcc);

  abandon_ttabs:
   TTB.close(&ttabs);

  abandon_function:
      return result;
}


void build_trec_list_alloca(TTABS *ttabs, RecID *list, int len, trec_list_user user, void *closure)
{
   IVTable *ivt = &ttabs->ivt;

   RecID *listend = list + len;

   const TREC *tlist[len];
   memset(tlist, 0, sizeof(tlist));
   const TREC **tptr = tlist;

   Result result;
   DBT value;
   memset(&value, 0, sizeof(DBT));

   while (list < listend)
   {
      if ((result = ivt->get_record_by_recid(ivt, *list, &value)))
         fprintf(stderr, "Failed to retrieve word number %u.\n", *list);
      else if (value.size > 0)
      {
         TREC *trec = (TREC*)alloca(value.size);
         memcpy(trec, value.data, value.size);

         *tptr = trec;
         ++tptr;
      }

      ++list;
   }

   (*user)(tlist, len, closure);
}


int trec_get_len(const void *el)
{
   const TREC *rec = (const TREC *)el;
   return rec->rec_length - sizeof(TREC);
}

int trec_print(FILE *f, const void *el)
{
   const TREC *rec = (const TREC *)el;
   int word_len = rec->rec_length - sizeof(TREC);
   return fprintf(f, "%*s", word_len, rec->value);
}

int trec_print_cell(FILE *f, const void *el, int width)
{
   const TREC *rec = (const TREC *)el;
   int word_len = rec->rec_length - sizeof(TREC);

   return fprintf(f, "%-*.*s", width, word_len, rec->value);
}

const CEIF ceif_trec = { trec_get_len, trec_print, trec_print_cell };



int trec_sort_alpha(const void *left, const void *right)
{
   const TREC *rec_left = *(const TREC **)left;
   int len_left = rec_left->rec_length - sizeof(TREC);
   char buff_left[len_left+1];
   memcpy(buff_left, rec_left->value, len_left);
   buff_left[len_left] = '\0';

   const TREC *rec_right = *(const TREC **)right;
   int len_right = rec_right->rec_length - sizeof(TREC);
   char buff_right[len_right+1];
   memcpy(buff_right, rec_right->value, len_right);
   buff_right[len_right] = '\0';

   return strcasecmp(buff_left, buff_right);
}

// Reverse sort because we want the most frequent at the
// top of the list.
int trec_sort_t_freq(const void *left, const void *right)
{
   const TREC *rec_left = *(const TREC **)left;
   const TREC *rec_right = *(const TREC **)right;

   // Reverse-sort, so we reverse the subtraction values
   return rec_right->count - rec_left->count;
}

int trec_sort_g_freq(const void *left, const void *right)
{
   const TREC *rec_left = *(const TREC **)left;
   const TREC *rec_right = *(const TREC **)right;

   return rec_left->frank - rec_right->frank;
}

int thesaurus_word_by_recid(int recid)
{
   int retval = 1;

   TTABS ttabs;
   Result result;
      
   TTB.init(&ttabs);

   if ((result = open_existing_thesaurus(&ttabs)))
      goto abandon_function;

   IVTable *ivt = &ttabs.ivt;
   
   DBT value;
   memset(&value, 0, sizeof(DBT));
   if ((result = ivt->get_record_by_recid(ivt, recid, &value)))
   {
      if (result == DB_NOTFOUND)
         printf("RecID %d was not found.\n", recid);
      else
      {
         DB *db = ivt->get_records_db(ivt);
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

   TTB.close(&ttabs);

  abandon_function:
   return retval;
}

typedef struct enumerator_track {
   int   entries;
   int   words;
   RecID min_syns_id;
   int   min_syns;
   RecID max_syns_id;
   int   max_syns;
} ET;

/*
 * This function, and the struct just above, is an implementation
 * of a callback function that can be called 
 */
void enumerator_callback(TTABS *ttabs, RecID id, TREC *trec, int word_len, void *closure)
{
   ET *et = (ET*)closure;

   ++et->words;

   printf("\x1b[1G");
   commaize_number(et->words);

   if (trec->is_root)
   {
      ++et->entries;

      int scount = TTB.count_synonyms(ttabs, id);

      if (scount < et->min_syns)
      {
         et->min_syns = scount;
         et->min_syns_id = id;
      }

      if (scount > et->max_syns)
      {
         et->max_syns = scount;
         et->max_syns_id = id;
      }
   }
}

/*
 * Use walk_entries() to evaluate each root word for related words count,
 * reporting the word with the least and the word with most related words.
 */
int enumerate_words(void)
{
   Result result;
   const char *tname = thesaurus_name ? thesaurus_name : "thesaurus";
      
   TTABS ttabs;
   TTB.init(&ttabs);
   if ((result = open_existing_thesaurus(&ttabs)))
   {
      printf("enumerate_words error opening %s: %s.\n", tname, db_strerror(result));
      return 0;
   }
   else
   {
      ET et;
      memset(&et, 0, sizeof(ET));
      et.min_syns = INT_MAX;

      TTB.walk_entries(&ttabs, enumerator_callback, &et);

      char buffmax[80];
      TTB.get_word_rec(&ttabs, et.max_syns_id, (TREC*)buffmax, sizeof(buffmax));
      char buffmin[80];
      TTB.get_word_rec(&ttabs, et.min_syns_id, (TREC*)buffmin, sizeof(buffmin));

      printf("\n");
      printf("\"%s\" has the fewest synonyms with %d.\n", ((TREC*)buffmin)->value, et.min_syns);
      printf("\"%s\" has the most synonyms with %d.\n", ((TREC*)buffmax)->value, et.max_syns);

      TTB.close(&ttabs);
   }

   return 0;
}



raAction actions[] = {
   {'h', "help",
    "This help display", &ra_show_help_agent },

   {-1, "*word",
    "Show thesaurus entry", &ra_string_agent, &thesaurus_word },
   {'t', "thesaurus_word",
    "Word to be sought in thesaurus", &ra_string_agent, &thesaurus_word },

   {'T', "import_thesaurus",
    "Import thesaurus contents", &ra_flag_agent, &flag_import_thesaurus },

   { 0, "thesaurus_name",
     "Change base name of thesaurus database (import and usage)", &ra_string_agent, &thesaurus_name },
   { 0, "thesaurus_source",
     "Path to thesaurus source", &ra_string_agent, &path_thesaurus_source },
   { 0, "frequency_source",
     "Path to frequency source", &ra_string_agent, &path_frequency_source },


   {'f', "freq_word",
    "Display frequency information about word", &ra_string_agent, &freq_word },
   {'F', "freq_import",
    "Import frequency data", &ra_flag_agent, &flag_import_frequencies }, 
   { 0,  "update_freq",
     "Update thesaurus word frequencies from wordc.db", &ra_flag_agent, &flag_update_freq },

   {'v', "verbose",
    "Verbose output for import", &ra_flag_agent, &flag_verbose },

   {'e', "enumerate",
    "DEBUGGING: Count synonyms per entry", &ra_flag_agent, &flag_enumerate },
   {'d', "thesaurus_dump",
    "DEBUGGING; Dump contents of thesaurus database", &ra_flag_agent, &flag_dump_thesaurus },
   {'i', "id",
    "DEBUGGING: Search thesaurus word by id", &ra_int_agent, &thesaurus_recid },
   {'s', "stack_report",
    "DEBUGGING: Show stack report for word lists", &ra_flag_agent, &flag_stack_report }
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
      else if (flag_import_frequencies)
         return import_frequencies();
      else if (flag_update_freq)
         return update_thesaurus_word_frequencies();
      else if (flag_enumerate)
         return enumerate_words();
      else if (flag_stack_report)
         run_stack_report("cut");
      else if (thesaurus_word)
         return show_thesaurus_word(thesaurus_word);
      else if (thesaurus_recid)
         return thesaurus_word_by_recid(thesaurus_recid);
   }

   return 0;
}
