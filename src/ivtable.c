// -*- compile-command: "cc -Wall -Werror -std=c99 -pedantic -m64 -ggdb -o ivtable ivtable.c -ldb -DIVTABLE_MAIN" -*-

#include <string.h>
#include "ivtable.h"

Result ivt_open_imp(IVTable *t, const char *name)
{
   Result result;
   int namelen = strlen(name);

   char name_records[namelen + 4];
   memcpy(name_records, name, namelen);
   strcpy(&name_records[namelen], ".db");

   char name_index[namelen + 5];
   memcpy(name_index, name, namelen);
   strcpy(&name_index[namelen], ".k2r");

   if ((result = open_table(&t->t_records, t->opener, name_records, 1, t->reclen)))
      goto abandon_function;

   if ((result = open_table(&t->t_index, O_Index_S2I, name_index, 1, 0)))
      goto abandon_root;

   return result;

  abandon_root:
   t->t_records.close(&t->t_records);

  abandon_function:
   return result;
}

void ivt_close_imp(IVTable *t)
{
   t->t_records.close(&t->t_records);
   t->t_index.close(&t->t_index);
}

Result ivt_get_recid_raw_imp(IVTable *t, RecID *id, DBT *key)
{
   DB* db = t->t_index.db;

   Result result;
   DBC *cursor;

   if ((result = db->cursor(db, NULL, &cursor, 0)))
      goto abandon_function;

   DBT value;
   memset(&value, 0, sizeof(DBT));

   if ((result = cursor->get(cursor, key, &value, DB_SET)))
      goto abandon_cursor;

   *id = *(RecID*)value.data;

  abandon_cursor:
   cursor->close(cursor);

  abandon_function:
   return result;
}

RecID ivt_get_recid_imp(IVTable *t, void *key_data, DataSize size)
{
   Result result;
   RecID id = 0;
   DBT key;
   set_dbt(&key, key_data, size);
   if ((result = ivt_get_recid_raw_imp(t, &id, &key)))
      return 0;
   else
      return id;
}

Result ivt_add_record_raw_imp(IVTable *t, RecID *id, DBT *key, DBT *value)
{
   Result result;
   DB *db;

   DBT dbt_id;
   memset(&dbt_id, 0, sizeof(DBT));

   db = t->get_records_db(t);
   if ((result = db->put(db, NULL, &dbt_id, value, DB_APPEND)))
      goto abandon_function;

   db = t->get_index_db(t);
   if ((result = db->put(db, NULL, key, &dbt_id, 0)))
      goto abandon_record;

   *id = *(RecID*)dbt_id.data;
   goto abandon_function;

  abandon_record:
   db = t->get_records_db(t);
   db->del(db, NULL, &dbt_id, 0);

  abandon_function:
   return result;
}


/**
 * Returns RecID both if record already exists or if
 * successful adding a new record.
 */
RecID ivt_add_record_imp(IVTable *t, RecPair *pair)
{
   DB *db;
   RecID id = 0;
   Result result = t->get_recid_raw(t, &id, &pair->key);

   if (result)
   {
      if (result == DB_NOTFOUND)
      {
         // Attempt to add record if key doesn't exist
         if ((result = t->add_record_raw(t, &id, &pair->key, &pair->value)))
         {
            db = t->get_records_db(t);
            db->err(db, result, "Error adding indexed record.");
         }
      }
      else
      {
         db = t->get_index_db(t);
         db->err(db, result, "Error seeking indexed record.");
      }
   }

   return id;
}

Result ivt_get_record_by_recid_raw(IVTable *t, DBT *key, DBT *value)
{
   DB *db = t->get_records_db(t);
   return db->get(db, NULL, key, value, 0);
}

Result ivt_get_record_by_recid_imp(IVTable *t, RecID id, DBT *value)
{
   DBT key;
   set_dbt(&key, &id, sizeof(RecID));

   return ivt_get_record_by_recid_raw(t, &key, value);
}

Result ivt_get_record_by_key_imp(IVTable *t, RecID *id, DBT *key, DBT *value)
{
   *id = 0;

   Result result;
   DB *db = t->get_index_db(t);
   

   DBT rect;
   memset(&rect, 0, sizeof(DBT));

   if ((result = db->get(db, NULL, key, &rect, 0)))
   {
      if (result == DB_NOTFOUND)
         return result;
   }
   else
   {
      if ((result = ivt_get_record_by_recid_raw(t, &rect, value)))
      {
         db = t->get_records_db(t);
         db->err(db, result, "Unexpected error getting keyed record.");
      }
      else
         *id = *(RecID*)rect.data;
   }

   return result;
}

Result ivt_update_record_imp(IVTable *t, RecID id, DBT *value)
{
   DB *db = t->get_records_db(t);

   DBT key;
   set_dbt(&key, &id, sizeof(RecID));

   return db->put(db, NULL, &key, value, 0);
}


DB* ivt_get_records_db(IVTable *t) { return t->t_records.db; }
DB* ivt_get_index_db(IVTable *t)   { return t->t_index.db; }

void init_IVTable(IVTable *t)
{
   memset(t, 0, sizeof(IVTable));

   t->opener = O_Recno;
   t->reclen = 0;

   t->open           = ivt_open_imp;
   t->close          = ivt_close_imp;
   t->get_recid_raw  = ivt_get_recid_raw_imp;
   t->add_record_raw = ivt_add_record_raw_imp;
   t->get_recid      = ivt_get_recid_imp;
   t->add_record     = ivt_add_record_imp;
   t->update_record  = ivt_update_record_imp;

   t->get_record_by_key = ivt_get_record_by_key_imp;
   t->get_record_by_recid = ivt_get_record_by_recid_imp;

   t->get_records_db = ivt_get_records_db;
   t->get_index_db   = ivt_get_index_db;
}

void ivt_make_queue_table(IVTable *t, RecLen reclen)
{
   if (! t->t_records.is_open(&t->t_records))
   {
      t->opener = O_Queue;
      t->reclen = reclen;
   }
   else
      fprintf(stderr, "Table type cannot be changed after it is created.\n");
}


#ifdef IVTABLE_MAIN

#include <stdint.h>
#include "bdb.c"

typedef struct srec_head {
   bool     is_head;
   uint32_t count;
   char     value[];
} SREC;

bool dump_headed_string(DBT *key, DBT *value, void *data)
{
   SREC *srec = (SREC*)value->data;
   int slen = value->size - sizeof(SREC);

   printf("%3u (x%u): %.*s.\n",
          *(RecID*)key->data,
          srec->count,
          slen,
          srec->value);

   return 1;
}

RecID add_headed_string_record(IVTable *t, const char *str)
{
   Result result;
   RecID id = 0;
   SREC *srec;

   DataSize str_size = strlen(str);
   DBT key;
   set_dbt(&key, (void*)str, str_size);

   DBT value;
   memset(&value, 0, sizeof(DBT));

   if ((result = t->get_record_by_key(t, &id, &key, &value)))
   {
      RecLen srec_size = sizeof(SREC) + str_size;
      char buffer[srec_size];
      srec = (SREC*)buffer;

      memset(srec, 0, sizeof(SREC));
      memcpy(srec->value, str, str_size);
      // The first word needs a count of 1 (not 0):
      ++srec->count;

      DBT value;
      set_dbt(&value, srec, srec_size);

      if ((result = t->add_record_raw(t, &id, &key, &value)))
      {
         DB *db = t->get_records_db(t);
         db->err(db, result, "Unexpected error adding \"%s\" to table.", str);
      }
   }
   else
   {
      srec = (SREC*)value.data;
      srec->count++;

      if ((result = t->update_record(t, id, &value)))
      {
         DB *db = t->get_records_db(t);
         db->err(db, result, "Unexpected error updating record.");
      }
   }

   return id;
}

RecID add_simple_string_record(IVTable *t, const char *str)
{
   // Add records that consist only of the string (no header)
   return 0;
}


int main(int argc, const char **argv)
{
   Result result;
   IVTable ivt;
   init_IVTable(&ivt);
   
   if (!(result = ivt.open(&ivt, "aaa")))
   {
      printf("Opened a database.\n");

      const char **ptr = argv;
      const char **end = argv + argc;

      while (++ptr < end)
      {
         RecID id = add_headed_string_record(&ivt, *ptr);
         if (id)
            printf("Added \"%s\" at %u.\n", *ptr, id);
      }

      dump_table(&ivt.t_records, dump_headed_string, NULL);

      ivt.close(&ivt);
   }

   return 0;
}


#endif
