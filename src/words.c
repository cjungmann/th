#include <string.h>
#include <readargs.h>
#include <alloca.h>
#include <limits.h>

#include "bdb.h"
#include "ivtable.h"
#include "thesaurus.h"
#include "parse_thesaurus.h"
#include "utils.h"
#include "term.h"

const char *thesaurus_name="thesaurus";
const char *thesaurus_word=NULL;
int thesaurus_recid=0;

bool flag_import_thesaurus = 0;
bool flag_dump_thesaurus = 0;
bool flag_stack_report = 0;
bool flag_enumerate = 0;
bool flag_verbose = 0;

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
   FILE *f = fopen("files/mthesaur.txt", "r");
   if (f)
   {
      Result result;
      TTABS ttabs;
      
      TTB.init(&ttabs);
      if (!(result = TTB.open(&ttabs, thesaurus_name)))
      {
         read_thesaurus_file(f, flag_verbose, save_thesaurus_word, &ttabs);

         
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


/**
 * Internal function pointer typedef
 *
 * Using a typedef allows me to try different methods
 * of collecting words, mainly using *alloc* or AVOIDING
 * *alloca* by using recursion and VLA instead.
 */
typedef void (*word_list_user)(const char **list, int length, void *closure);

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

/**
 * One callback function option for showing the results of one
 * of the related word collection functions.
 */
void show_recid_word_list(TTABS *ttabs, RecID *list, int length)
{
   char buff[64];
   TREC *trec = (TREC*)buff;

   RecID *ptr = list;
   RecID *end = list + length;

   Result result;

   while (ptr < end)
   {
      if (!(result = TTB.get_word_rec(ttabs, *ptr, trec, sizeof(buff))))
         printf(" %s%s\x1b[m", cycle_color(), trec->value);
      else
         printf(" %s%u\x1b[m", cycle_color(), *ptr);
      
      ++ptr;
   }

   printf("\n");
}

void show_recid_recid_list(RecID *list, int length)
{
   RecID *ptr = list;
   RecID *end = list + length;
   const char *preface = "";

   while (ptr < end)
   {
      printf("%s%s%u\x1b[m", preface, cycle_color(), *ptr);
      preface = ", ";
      ++ptr;
   }
   printf("\n");   
}

void show_word_list(const char **list, int length, void *closure)
{
   if (flag_stack_report)
      display_stack_report(1);
   else
   {
      int curlen, maxlen = 0;
      const char **ptr, **end = list + length;

      ptr = list;
      while (ptr < end)
      {
         curlen = strlen(*ptr);
         if (curlen > maxlen)
            maxlen = curlen;

         ++ptr;
      }

      ptr = list;
      while (ptr < end)
      {
         printf("%s%s\x1b[m\n", cycle_color(), *ptr);
         ++ptr;
      }
   }
}

void show_thesaurus_word_callback(TTABS *ttabs, RecID *list, int length, void *data)
{
   printf("Making a list of %d words.\n", length);

   /* show_recid_recid_list(list, length); */
   /* show_recid_word_list(ttabs, list, length); */

   /* word_list_user = show_word_list; // local, for debugging */
   word_list_user wlu = show_words;     // production, from term.c

   if (flag_stack_report)
      printf("Stack report for alloca method.\n");
   build_word_list_alloca(ttabs, list, length, wlu, data);

   if (flag_stack_report)
      printf("Stack report for recursive/vla method.\n");
   build_word_list_recurse(ttabs, list, length, wlu, data);
}

/**
 * Fulfills command line option -t (or non-option argument)
 * to show the list of words related to the *word* argument.
 */
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
         TTB.get_words(&ttabs, id, show_thesaurus_word_callback, NULL);
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

typedef struct enumerator_track {
   int entries;
   int words;
   RecID min_syns_id;
   int min_syns;
   RecID max_syns_id;
   int max_syns;
} ET;

void enumerator_callback(TTABS *ttabs, RecID id, TREC *trec, void *closure)
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

int enumerate_words(void)
{
   TTABS ttabs;
   Result result;
      
   TTB.init(&ttabs);
   if ((result = TTB.open(&ttabs, thesaurus_name)))
   {
      printf("Error opening %s: %s.\n", thesaurus_name, db_strerror(result));
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
   {'h', "help", "This help display", &ra_show_help_agent },

   {'i', "id", "Search thesaurus word by id", &ra_int_agent, &thesaurus_recid },
   {'e', "enumerate", "Count synonyms per entry", &ra_flag_agent, &flag_enumerate },

   {'T', "import_thesaurus", "Import thesaurus contents", &ra_flag_agent, &flag_import_thesaurus },
   {'t', "thesaurus_word", "Word to be sought in thesaurus", &ra_string_agent, &thesaurus_word },

   {'f', "thesaurus_name", "Base name of thesaurus database", &ra_string_agent, &thesaurus_name },
   {'d', "thesaurus_dump", "Dump contents of thesaurus database", &ra_flag_agent, &flag_dump_thesaurus },
   {'s', "stack_report", "Show stack report for word lists", &ra_flag_agent, &flag_stack_report },
   {'v', "verbose", "Verbose output", &ra_flag_agent, &flag_verbose },
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
      else if (flag_enumerate)
         return enumerate_words();
      else if (thesaurus_word)
         return show_thesaurus_word(thesaurus_word);
      else if (thesaurus_recid)
         return thesaurus_word_by_recid(thesaurus_recid);
   }

   return 0;
}
