// -*- compile-command: "cc -Wall -Werror -std=c99 -pedantic -m64 -ggdb -o istringt istringt.c -ldb -DISTRINGT_MAIN" -*-

#include "istringt.h"
#include <string.h>

int ist_open_imp(IStringT *t, const char *name)
{
   IStringT *this = t;
   int result;
   int namelen = strlen(name);

   char name_root[namelen + 4];
   memcpy(name_root, name, namelen);
   strcpy(&name_root[namelen], ".db");

   char name_sndx[namelen + 5];
   memcpy(name_sndx, name, namelen);
   strcpy(&name_sndx[namelen], ".s2i");

   if ((result = open_table(&this->t_strings, this->opener, name_root, 1, this->reclen)))
      goto abandon_function;

   if ((result = open_table(&this->t_strndx, O_Index_S2I, name_sndx, 1, 0)))
      goto abandon_root;

   return result;

  abandon_root:
   this->t_strings.close(&this->t_strings);

  abandon_function:
   return result;
}

void ist_close_imp(IStringT *t)
{
   t->t_strings.close(&t->t_strings);
   t->t_strndx.close(&t->t_strndx);
}

int ist_get_recid_base(IStringT *t, RecID *id, const char *str)
{
   *id = 0;

   DB *db = t->get_ndx_db(t);
   int result;

   DBC *cursor;
   if ((result = db->cursor(db, NULL, &cursor, 0)))
      goto abandon_function;

   DBT key;
   DBT value;
   memset(&key, 0, sizeof(DBT));
   memset(&value, 0, sizeof(DBT));

   key.data = (char*)str;
   key.size = strlen(str);

   if (!(result = cursor->get(cursor, &key, &value, DB_SET)))
      *id = *(RecID*)value.data;

   cursor->close(cursor);

  abandon_function:
   return result;
}

RecID ist_get_recid_imp(IStringT *t, const char *str)
{
   RecID rval;
   int result = ist_get_recid_base(t, &rval, str);
   if (!result)
      return rval;
   else
      return 0;
}

int ist_put_string_imp(IStringT *t, RecID *id, const char *str)
{
   *id = 0;

   int result;
   DB *db;
   if (!(result = ist_get_recid_base(t, id, str)))
   {
      // If already in index, leave function with
      // *id* set by ist_get_recid_base().
      goto abandon_function;
   }
   else if (result!=DB_NOTFOUND)
   {
      db = t->get_ndx_db(t);
      db->err(db, result, "Unexpected error during cursor search.");
      goto abandon_function;
   }

   DBT d_string;
   DBT d_recid;
   memset(&d_string, 0, sizeof(DBT));
   memset(&d_recid, 0, sizeof(DBT));

   d_string.data = (char*)str;
   d_string.size = strlen(str);

   db = t->t_strings.db;

   if ((result = db->put(db, NULL, &d_recid, &d_string, DB_APPEND)))
      goto abandon_function;

   db = t->t_strndx.db;

   if ((result = db->put(db, NULL, &d_string, &d_recid, 0)))
   {
      if (result != DB_NOTFOUND)
         db->err(db, result, "Unexpected error while adding key to string.");
      goto abandon_record;
   }

   *id = *(RecID*)d_recid.data;
   goto abandon_function;
   
  abandon_record:
   db = t->t_strings.db;
   db->del(db, NULL, &d_recid, 0);

  abandon_function:
   return result;
}

DB *ist_get_db_imp(IStringT *t) { return t->t_strings.db; }
DB *ist_get_ndx_db_imp(IStringT *t) { return t->t_strndx.db; }

void init_istringt(IStringT *t)
{
   memset(t, 0, sizeof(IStringT));

   // Default set to fixed-length record, Queue-type table.
   // These values can be changed after init_istringt().
   t->opener = O_Queue;
   t->reclen = 45;
   
   t->open = ist_open_imp;
   t->close = ist_close_imp;
   t->put_string = ist_put_string_imp;
   t->get_recid = ist_get_recid_imp;
   t->get_db = ist_get_db_imp;
   t->get_ndx_db = ist_get_ndx_db_imp;
}


#ifdef ISTRINGT_MAIN
#include "bdb.c"

int dumper(DBT *key, DBT *value, void *data)
{
   RecID id = *(RecID*)key->data;
   char *str = (char*)value->data;
   int slen = value->size;

   printf("%3u: %.*s\n", id, slen, str);

   return 1;
 }

int main(int argc, const char **argv)
{
   IStringT ist;
   init_istringt(&ist);

   int result = ist.open(&ist, "test");
   if (!result)
   {
      printf("Opened the file(s).\n");

      const char **ptr = argv;
      const char **end = argc+argv;
      RecID newid;

      while (++ptr < end)
      {
         if ((result = ist.put_string(&ist, &newid, *ptr)))
         {
            DB *db = ist.get_db(&ist);
            db->err(db, result, "Failed to add a string.");
            break;
         }
         else
            printf("Added \"%s\" at %u.\n", *ptr, newid);
      }

      dump_table(&ist.t_strings, dumper, NULL);

      ist.close(&ist);
   }

   return 0;
}
#endif
