#include "bdb.h"
#include "ivtable.h"
#include "utils.h"
#include "thesaurus.h"
#include "rrtable.h"

#define __USE_MISC
#include <stdlib.h>  // for qsort, alloca unlocked with __USE_MISC for BSD

#include <string.h>    // for memset()

#include <sys/types.h> // for stat()
#include <sys/stat.h>
#include <unistd.h>

#include <ctype.h>     // for isspace()

#include <fcntl.h>     // for open()
#include <errno.h>     // ENOENT return value

const char *thesaurus_name = NULL;

// "class" member functions, for TTABS_Class instance TTB
void   ttabs_init_imp(TTABS *ttabs);
Result ttabs_open_imp(TTABS *ttabs, const char *name, bool create);
void   ttabs_close_imp(TTABS *ttabs);
Result ttabs_add_word_imp(TTABS *ttabs, const char *str, int size, bool newline);
Result ttabs_get_word_rec_imp(TTABS *ttabs, RecID id, TREC *buffer, DataSize size);
RecID  ttabs_lookup_imp(TTABS *ttabs, const char *str);
Result ttabs_get_words_imp(DB *db, TTABS *ttabs, RecID id, recid_list_user user, void *closure);
Result ttabs_get_result_imp (TTABS *ttabs, RecID id, tresult_user user, void *closure);
Result ttabs_get_twresult_imp(TTABS *ttabs, RecID id, twresult_user user, void *closure);
Result ttabs_walk_entries_imp(TTABS *ttabs, tword_user user, void *closure);
int ttabs_count_synonyms_imp(TTABS *ttabs, RecID id);
DataSize ttabs_word_rank_imp(TTABS *ttabs, const char *word, int size);

void i2i_opener(const char *path, void *data);
Result open_linker(DB **db, int create, const char *name, const char *ext);

bool save_word_links(TTABS *ttabs, RecID root, RecID word);

TTABS_Class TTB = {
   ttabs_init_imp,
   ttabs_open_imp,
   ttabs_close_imp,
   ttabs_add_word_imp,
   ttabs_get_word_rec_imp,
   ttabs_lookup_imp,
   ttabs_get_words_imp,
   ttabs_get_result_imp,
   ttabs_get_twresult_imp,
   ttabs_walk_entries_imp,
   ttabs_count_synonyms_imp,
   ttabs_word_rank_imp
};

void ttabs_init_imp(TTABS *ttabs)
{
   memset(ttabs, 0, sizeof(TTABS));
   init_IVTable(&ttabs->ivt);
   ttabs->db_r2w = NULL;
   ttabs->db_w2r = NULL;
}

Result ttabs_open_imp(TTABS *ttabs, const char *name, bool create)
{
   DB *db_r2w, *db_w2r;
   Result result;
   IVTable *ivt = &ttabs->ivt;

   if ((result = ivt->open(ivt, name, create)))
      goto abandon_function;

   if ((result = open_linker(&db_r2w, create, name, "r2w")))
      goto abandon_ivt;

   if ((result = open_linker(&db_w2r, create, name, "w2r")))
      goto abandon_r2w;

   ttabs->db_r2w = db_r2w;
   ttabs->db_w2r = db_w2r;
   goto abandon_function;

  abandon_r2w:
   db_r2w->close(db_r2w, 0);

  abandon_ivt:
   ivt->close(ivt);

  abandon_function:
   return result;
}

void ttabs_close_imp(TTABS *ttabs)
{
   ttabs->db_r2w->close(ttabs->db_r2w, 0);
   ttabs->db_w2r->close(ttabs->db_w2r, 0);
   ttabs->ivt.close(&ttabs->ivt);

   memset(ttabs, 0, sizeof(TTABS));
}

Result ttabs_add_word_imp(TTABS *ttabs, const char *str, int size, bool newline)
{
   static RecID rootid = 0;

   Result result;
   DB *db = NULL;    // in case we need to report an error

   IVTable *ivt = &ttabs->ivt;

   DBT key;
   set_dbt(&key, (void*)str, size);
   DBT value;
   memset(&value, 0, sizeof(DBT));

   RecID recid;

   int bufflen = 0;
   TREC *trec = NULL;

   if ((result = ivt->get_record_by_key(ivt, &recid, &key, &value)))
   {
      if (result == DB_NOTFOUND)
      {
         // Allocate memory for the record
         bufflen = size + sizeof(TREC);
         char buffer[bufflen];
         memset(&buffer, 0, bufflen);
         trec = (TREC*)buffer;

         // Set the record values
         ++trec->count;
         trec->rec_length = bufflen;
         trec->frank = TTB.get_rank_from_ranker(ttabs, str, size);
         if (newline)
            ++trec->is_root;
         memcpy(trec->value, str, size);

         value.data = buffer;
         value.size = bufflen;

         if ((result = ivt->add_record_raw(ivt, &recid, &key, &value)))
         {
            db = ivt->get_records_db(ivt);
            db->err(db, result, "Unexpected error adding a record for \"%.*s\".", size, str);
            return 0;
         }
      }
      else
      {
         db = ivt->get_records_db(ivt);
         db->err(db, result, "Unexpected error searching for \"%.*s\".", size, str);
         return 0;
      }
   }
   else
   {
      // Update the existing record value, mainly
      // incrementing the count and updaing is_root
      // if appropriate
      trec = (TREC*)value.data;
      ++trec->count;
      if (newline)
         ++trec->is_root;

      if ((result = ivt->update_record(ivt, recid, &value)))
      {
         db = ivt->get_records_db(ivt);
         db->err(db, result, "Unexpected error updating the record for \"%.*s\".", size, str);
         return 0;
      }
   }

   // Errors will have exited by now.
   // Update inter-word indexes
   if (recid)
   {
      if (newline)
         rootid = recid;
      else
         save_word_links(ttabs, rootid, recid);
         /* rootid = rootid; */
   }

   return 1;
}

/**
 * Copies the word record, a string preceded by a TREC struct, into
 * the provided buffer, adding the terminating \0 for convenience.
 *
 * Some adjustments will be made for a too-small buffer.  If
 * the buffer is large enough to contain the TREC header plus
 * one extra byte to make a zero-length string, then as much of
 * the content as fits will be copied to the buffer as fits.
 * Otherwise (if the buffer is too small for even that), the
 * entire buffer will be set to 0s.
 *
 * In either case of too-small buffer, an error will be
 * indicated by the return value.
 */
Result ttabs_get_word_rec_imp(TTABS *ttabs, RecID id, TREC *buffer, DataSize size)
{
   Result result;

   // Exit immediately if buffer not sufficient to hold anything useful:
   if (size < sizeof(TREC) + 1)
   {
      memset(buffer, 0, size);
      result = DB_BUFFER_SMALL;
      goto abandon_function;
   }

   DBT value;
   memset(&value, 0, sizeof(DBT));
   char *rawbuff = (char*)buffer;

   if (!(result = ttabs->ivt.get_record_by_recid(&ttabs->ivt, id, &value)))
   {
      if (value.size < size-1)
      {
         memcpy(rawbuff, value.data, value.size);
         rawbuff[value.size] = '\0';
      }
      else
      {
         memcpy(rawbuff, value.data, size-1);
         rawbuff[size-1] = '\0';

         result = DB_BUFFER_SMALL;
      }
   }
      
  abandon_function:
   return result;
}

RecID ttabs_lookup_imp(TTABS *ttabs, const char *str)
{
   IVTable *ivt = &ttabs->ivt;
   return ivt->get_recid(ivt, (void*)str, strlen(str));
}


/****
 * Beginning of get_words() section
 * The next section (typedef + get_words_closure() + get_words_callback() + ttabs_get_word_list_imp()
 * work together, with callbacks, closure struct.
 */
typedef bool (*cursor_callback)(DBT *key, DBT *value, void *closure);

void run_cursor_with_closure(DBC *cursor, DBT *key, cursor_callback cb, void *closure)
{
   Result result;
   DBT value;
   memset(&value, 0, sizeof(DBT));

   if (!(result = cursor->get(cursor, key, &value, DB_SET)))
   {
      while ((*cb)(key, &value, closure))
         cursor->get(cursor, key, &value, DB_NEXT);
   }
}

typedef struct get_words_closure {
   RecID           id;
   recid_list_user rluser;
   void            *rlclosure;
   int             count;
   RecID           *list;
} GWC;

/** This is used as a *cursor_callback function pointer */
bool get_words_callback(DBT *key, DBT *value, void *closure)
{
   GWC *gwc = (GWC*)closure;

   if (*(RecID*)key->data == gwc->id)
   {
      if (gwc->list)
         *gwc->list++ = *(RecID*)value->data;
      else
         ++gwc->count;

      return 1;
   }
   else
      return 0;
}

Result ttabs_get_words_imp(DB *db, TTABS *ttabs, RecID id, recid_list_user user, void *closure)
{
   GWC gwc = { id, user, closure, 0, NULL };

   DBC *cursor;
   Result result;

   if (!(result = db->cursor(db, NULL, &cursor, 0)))
   {
      DBT key;
      set_dbt(&key, &id, sizeof(RecID));
      run_cursor_with_closure(cursor, &key, get_words_callback, &gwc);
      if (gwc.count)
      {
         RecID idlist[gwc.count];
         gwc.list = idlist;
         memset(&idlist, 0, sizeof(idlist));

         // The key must reset to rerun the list
         set_dbt(&key, &id, sizeof(RecID));
         run_cursor_with_closure(cursor, &key, get_words_callback, &gwc);
         (*user)(ttabs, idlist, gwc.count, closure);
      }
      else
         (*user)(ttabs, NULL, 0, closure);

      cursor->close(cursor);
   }

   return result;
}

struct get_result_closure {
   TRESULT      *tresult;
   tresult_user user;
   void         *closure;
};


void ttabs_get_result_stage_two(TTABS *ttabs, RecID *list, int len, void *closure)
{
   // Saving word to roots list
   struct get_result_closure *rclosure = (struct get_result_closure*)closure;
   rclosure->tresult->roots = list;
   rclosure->tresult->roots_count = len;

   (*rclosure->user)(ttabs, rclosure->tresult, rclosure->closure);
}

void ttabs_get_result_stage_one(TTABS *ttabs, RecID *list, int len, void *closure)
{
   // Saving root to words list
   struct get_result_closure *rclosure = (struct get_result_closure*)closure;
   rclosure->tresult->entries = list;
   rclosure->tresult->entries_count = len;

   ttabs_get_words_imp(ttabs->db_w2r,
                       ttabs,
                       rclosure->tresult->tword,
                       ttabs_get_result_stage_two,
                       closure);
}

Result ttabs_get_result_imp(TTABS *ttabs, RecID id, tresult_user user, void *closure)
{
   TRESULT tresult = { id };
   memset(&tresult, 0, sizeof(tresult));
   tresult.tword = id;

   struct get_result_closure rclosure = { &tresult, user, closure };

   return ttabs_get_words_imp(ttabs->db_r2w,
                              ttabs,
                              id,
                              ttabs_get_result_stage_one,
                              &rclosure);
}


typedef struct get_twresult_closure {
   RecID         id;
   twresult_user user;
   void          *closure;
   TTABS         *ttabs;
   TWRESULT      *twresult;
} TWR_Closure;



void ttabs_get_root_trecs(const TREC **list, int length, void *closure)
{
   TWR_Closure *twr_closure = (TWR_Closure*)closure;
   twr_closure->twresult->roots = list;
   twr_closure->twresult->roots_count = length;

   (*twr_closure->user)(twr_closure->ttabs, twr_closure->twresult, twr_closure->closure);
}

void ttabs_get_root_recids(TTABS *ttabs, RecID *list, int len, void *closure)
{
   build_trec_list_alloca(ttabs, list, len, ttabs_get_root_trecs, closure);
}

void ttabs_get_entry_trecs(const TREC **list, int length, void *closure)
{
   TWR_Closure *twr_closure = (TWR_Closure*)closure;
   twr_closure->twresult->entries = list;
   twr_closure->twresult->entries_count = length;

   // Using word-to-root (w2r) index to
   // get roots that include this word
   ttabs_get_words_imp(twr_closure->ttabs->db_w2r,
                       twr_closure->ttabs,
                       twr_closure->id,
                       ttabs_get_root_recids,
                       closure);
}

void ttabs_get_entry_recids(TTABS *ttabs, RecID *list, int len, void *closure)
{
   build_trec_list_alloca(ttabs, list, len, ttabs_get_entry_trecs, closure);
}

Result ttabs_get_twresult_imp(TTABS *ttabs, RecID id, twresult_user user, void *closure)
{
   TWRESULT twresult;
   memset(&twresult, 0, sizeof(twresult));
   TWR_Closure twr_closure = { id, user, closure, ttabs, &twresult };

   // Using root-to-word (r2w) index to
   // get words with this word as root.
   return ttabs_get_words_imp(ttabs->db_r2w,
                              ttabs,
                              id,
                              ttabs_get_entry_recids,
                              &twr_closure);
}






/**
 * For each word record in the table, this function will call
 * the *user* function with the record and the *closure*.
 */
Result ttabs_walk_entries_imp(TTABS *ttabs, tword_user user, void *closure)
{
   DB *db = ttabs->ivt.get_records_db(&ttabs->ivt);
   DBC *cursor;
   Result result;

   if ((result = db->cursor(db, NULL, &cursor, 0)))
      goto abandon_function;

   DBT key;
   memset(&key, 0, sizeof(DBT));
   DBT value;
   memset(&value, 0, sizeof(DBT));

   while (!(result = cursor->get(cursor, &key, &value, DB_NEXT)))
      (*user)(ttabs, *(RecID*)key.data, (TREC*)value.data, value.size - sizeof(TREC), closure);

   if (result != DB_NOTFOUND)
      db->err(db, result, "Fell out of entries walk.");

  abandon_function:
   return result;
}

int ttabs_count_synonyms_imp(TTABS *ttabs, RecID id)
{
   DB *db = ttabs->db_r2w;
   int count = 0;

   DBT key;
   set_dbt(&key, &id, sizeof(RecID));

   DBT value;
   memset(&value, 0, sizeof(DBT));

   DBC *cursor;

   Result result;
   if ((result = db->cursor(db, NULL, &cursor, 0)))
      goto abandon_function;

   if ((result = cursor->get(cursor, &key, &value, DB_SET)))
      goto abandon_function;

   do
   {
      ++count;
      if ((result = cursor->get(cursor, &key, &value, DB_NEXT)))
         break;
      if (*(RecID*)key.data != id)
         break;
   }
   while( !result);
   
  abandon_function:
   return count;
}

DataSize ttabs_word_rank_imp(TTABS *ttabs, const char *word, int size)
{
   if (ttabs->rankobj && ttabs->ranker)
      return ttabs->ranker(ttabs->rankobj, word, size);
   else
      return 0;
}

/******
 * End of get_words section
 */


typedef struct _db_opening {
   DB     *db;
   Result result;
   bool   create;
} DBO;

// Callback function for strargs_builder when called by open_linker
void i2i_opener(const char *path, void *data)
{
   DBO *dbo = (DBO*)data;
   DB *db;
   if (!(dbo->result = rrt_open(&db, dbo->create, path)))
      dbo->db = db;
}

Result open_linker(DB **db, int create, const char *name, const char *ext)
{
   DBO dbo;
   memset(&dbo, 0, sizeof(DBO));

   dbo.create = create;

   strargs_builder(i2i_opener, &dbo, name, ".", ext, NULL);

   if (dbo.result==0)
      *db = dbo.db;

   return dbo.result;
}

bool save_word_links_inkey(TTABS *ttabs, RecID root, RecID word)
{
   RecID arr_r2w[2] = { root, word };
   DBT r2w;
   set_dbt(&r2w, arr_r2w, sizeof(arr_r2w));

   RecID arr_w2r[2] = { word, root };
   DBT w2r;
   set_dbt(&w2r, arr_w2r, sizeof(arr_w2r));

   DBT value;
   memset(&value, 0, sizeof(DBT));

   DB *db;
   Result result;
   
   db = ttabs->db_r2w;
   if ((result = db->put(db, NULL, &r2w, &value, DB_NODUPDATA)))
      if (result != DB_KEYEXIST)
         db->err(db, result, "Making root-to-word (%u -> %u) link.", root, word);

   db = ttabs->db_w2r;
   if ((result = db->put(db, NULL, &w2r, &value, DB_NODUPDATA)))
      if (result != DB_KEYEXIST)
         db->err(db, result, "Making word-to-root (%u -> %u) link.", word, root);

   return 1;
}

bool save_word_links(TTABS *ttabs, RecID root, RecID word)
{
   DBT dbt_root;
   set_dbt(&dbt_root, &root, sizeof(RecID));
   DBT dbt_word;
   set_dbt(&dbt_word, &word, sizeof(RecID));

   DB     *db;
   Result result;

   db = ttabs->db_r2w;
   if ((result = db->put(db, NULL, &dbt_root, &dbt_word, DB_NODUPDATA)))
   {
      if (result != DB_KEYEXIST)
         db->err(db, result, "Making root-to-word (%u -> %u) link.", root, word);
   }

   db = ttabs->db_w2r;
   if ((result = db->put(db, NULL, &dbt_word, &dbt_root, DB_NODUPDATA)))
   {
      if (result != DB_KEYEXIST)
         db->err(db, result, "Making word-to-root (%u -> %u) link.", word, root);
   }

   return 1;
}


bool save_thesaurus_word(const char *str, int size, bool newline, void *data)
{
   TTABS *ttabs = (TTABS*)data;
   return TTB.add_word(ttabs, str, size, newline);
}

bool thesaurus_dumpster(DBT *key, DBT *value, void *data)
{
   TREC *trec = (TREC*)value->data;
   int word_size = value->size - sizeof(TREC);
   const char *color = trec->is_root ? "\x1b[32;1m" : "";
   printf("%s%7u %5u %.*s\x1b[m\n",
          color,
          *(RecID*)key->data,
          trec->count,
          word_size, trec->value);
   return 1;
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


/**
 * This function tries to find the best version of the
 * thesaurus, with priorities being:
 * 1. Command-line thesaurus name (-f/thesaurus_name)
 * 2. Contents of /etc/th.conf
 * 3. ./thesaurus.db in PWD
 */
Result open_existing_thesaurus(TTABS *ttabs)
{
   if (thesaurus_name)
      return TTB.open(ttabs, thesaurus_name, 0);
   else
   {
      struct stat fstat;
      memset(&fstat, 0, sizeof(fstat));
      int result = stat("/etc/th.conf", &fstat);
      if (result == 0)
      {
         int bytesread, nlen = fstat.st_size;
         char buff[nlen+1];
         int fh = open("/etc/th.conf", O_RDONLY);
         if (fh)
         {
            bytesread = read(fh, &buff, fstat.st_size);
            close(fh);

            if (bytesread == fstat.st_size)
            {
               // Back-off spaces (newline) and terminate after last non-space:
               char *end = buff + nlen-1;
               while (isspace(*end))
                  --end;
               *(end+1) = '\0';

               return TTB.open(ttabs, buff, 0);
            }
         }
      }

      return TTB.open(ttabs, "thesaurus", 0);
   }

   return ENOENT;
}

#ifdef THESAURUS_MAIN

#include <stdio.h>
#include "bdb.c"
#include "ivtable.c"
#include "rrtable.c"
#include "utils.c"

void test_result_user(TTABS *ttabs, TRESULT *tresult, void *closure)
{
   char buff[128];
   TREC *trec = (TREC*)buff;
   TTB.get_word_rec(ttabs, tresult->tword, trec, sizeof(buff));

   printf("\nfor entry \x1b[32;1m%s\x1b[m, there are\n"
          "%d synonyms, and\n"
          "%d roots that include the word.\n",
          trec->value,
          tresult->entries_count,
          tresult->roots_count);
}

void test_get_result(TTABS *ttabs, const char *str)
{
   RecID id = TTB.lookup(ttabs, str);
   if ( id < 1 )
      printf("Failed to find entry for %s.\n", str);
   else
      TTB.get_result(ttabs, id, test_result_user, NULL);
}

int main(int argc, const char **argv)
{
   const char *tname = "../thesaurus";
   Result result;
   TTABS ttabs;
   TTB.init(&ttabs);

   if ((result = TTB.open(&ttabs, tname, 1)))
      printf("Failed to build \"%s\".\n", tname);
   else
   {
      if (argc == 1)
         printf("Successfully opened \"%s\".\n", tname);
      else
      {
         const char **ptr = argv + 1;
         const char **end = argv + argc;

         while (ptr < end)
         {
            test_get_result(&ttabs, *ptr);
            ++ptr;
         }
      }
   }

   return 0;
}
#endif


/* Local Variables: */
/* compile-command: "b=thesaurus; \*/
/*  cc -Wall -Werror -ggdb        \*/
/*  -std=c99 -pedantic            \*/
/*  -ldb  -lreadargs              \*/
/*  -D${b^^}_MAIN -o $b ${b}.c"   \*/
/* End: */
