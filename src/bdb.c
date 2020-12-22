// -*- compile-command: "cc -ggdb -o bdb bdb.c -ldb -DDBD_MAIN" -*-

#include <string.h>
#include "bdb.h"

Result db_open_imp(DB **ret_db,
                   const char *name,
                   DBTYPE type,
                   bool create,
                   RecLen reclen,
                   DBFlags set_flags)
{
   DB *db;
   Result result;

   if ((result = db_create(&db, NULL, 0)))
      goto abandon_function;

   if (reclen > 0)
   {
      db->set_re_len(db, reclen);
      db->set_re_pad(db, 0);
   }

   if (set_flags)
   {
      if ((result = db->set_flags(db, set_flags)))
         goto abandon_db;
   }

   DBFlags dbflags = (create ? DB_CREATE|DB_OVERWRITE : DB_RDONLY);

   if ((result = db->open(db, NULL, name, NULL, type, dbflags, 0664)))
      goto abandon_db;

   *ret_db = db;
   return 0;

  abandon_db:
   db->close(db, 0);

  abandon_function:
   return result;
}

void db_close_imp(void *th)
{
   Table *this = (Table*)th;
   if (this->db)
   {
      this->db->close(this->db,0);
      this->db = NULL;
   }
}

bool db_is_open_imp(void *t) { return ((Table*)t)->db != NULL; }

Result db_add_record_basic(void *th, RecID *recid, RecPair *pair)
{
   Table *this = (Table*)th;
   DB *db = this->db;

   return db->put(db, NULL, &pair->key, &pair->value, DB_APPEND);
}

Result db_add_record_recno(void *th, RecID *recid, RecPair *pair)
{
   Table *this = (Table*)th;
   Result result = 1;

   DB *db = this->db;
   if (!(result = db->put(db, NULL, &pair->key, &pair->value, DB_APPEND)))
      *recid = *(RecID*)pair->key.data;
   else
      *recid = 0;

   return result;
}



Result O_Queue(Table *table, const char *name, bool create, RecLen reclen)
{
   DB *db = NULL;
   Result result = db_open_imp(&db, name, DB_QUEUE, create, reclen, 0);
   if (!result)
   {
      table->db = db;
      table->close = db_close_imp;
      table->add = db_add_record_recno;
   }

   return result;
}

Result O_Recno(Table *table, const char *name, bool create, RecLen reclen)
{
   DB *db = NULL;

   Result result = db_open_imp(&db, name, DB_RECNO, create, reclen, 0);
   if (!result)
   {
      table->db = db;
      table->close = db_close_imp;
      table->add = db_add_record_recno;
   }

   return result;
}

Result O_Index_S2I(Table *table, const char *name, bool create, RecLen reclen)
{
   DB *db = NULL;
   int result = db_open_imp(&db, name, DB_BTREE, create, 0, 0);
   if (!result)
   {
      table->db = db;
      table->close = db_close_imp;
      table->add = db_add_record_basic;
   }

   return result;
}

Result O_Index_I2I(Table *table, const char *name, bool create, RecLen reclen)
{
   DB *db = NULL;
   Result result = db_open_imp(&db, name, DB_BTREE, create, 0, DB_DUPSORT);
   if (!result)
   {
      table->db = db;
      table->close = db_close_imp;
      table->add = db_add_record_basic;
   }

   return result;
}

Result open_table(Table *table, Opener opener, const char *name, bool create, RecLen reclen)
{
   table->db = NULL;
   table->close = db_close_imp;
   table->is_open = db_is_open_imp;

   return (*opener)(table, name, create, reclen);
}

void dump_db(DB *db, Dumpster dumpster, void *data)
{
   DBC *cursor;
   Result result;
   if ((result = db->cursor(db, NULL, &cursor, 0)))
      goto abandon_function;

   DBT key;
   memset(&key, 0, sizeof(DBT));
   DBT value;
   memset(&value, 0, sizeof(DBT));

   while (!(result = cursor->get(cursor, &key, &value, DB_NEXT)))
   {
      if (!(*dumpster)(&key, &value, data))
         goto abandon_cursor;
   }

   if (result != DB_NOTFOUND)
      db->err(db, result, "Unexpected cursor error.");

  abandon_cursor:
   cursor->close(cursor);
   
  abandon_function:
   ;
}

void dump_table(Table *table, Dumpster dumpster, void *data)
{
   dump_db(table->db, dumpster, data);
}

RecPair* set_pair(RecPair *pair, PairSet ps)
{
   set_dbt(&pair->key, ps.key_data, ps.key_size);
   set_dbt(&pair->value, ps.value_data, ps.value_size);
   return pair;
}

void init_pair(RecPair *pair)
{
   memset(&pair->key, 0, sizeof(DBT));
   memset(&pair->value, 0, sizeof(DBT));
}

void set_dbt_str(DBT *dbt, const char *str)
{
   dbt->data = (char *)str;
   dbt->size = strlen(str);
}

RecPair* set_put_string(RecPair *pair, const char *str)
{
   init_pair(pair);
   set_dbt_str(&pair->value, str);
   return pair;
}

RecPair* set_put_string_index(RecPair *pair, const char *str, RecID *rec)
{
   init_pair(pair);

   set_dbt_str(&pair->key, str);

   pair->value.data = rec;
   pair->value.size = sizeof(RecID);

   return pair;
}

#ifdef DBD_MAIN

#include <stdio.h>

bool dump_string_record(DBT *key, DBT *value, void *data)
{
   if (value->size > 0)
      printf("%3u: %.*s\n", *(RecID*)key->data, value->size, (char*)value->data);
   else
      printf("na\n");
}

typedef struct tfiles_t {
   Table t_strings;
   Table t_strndx;
} TFiles;

/* int open_tfiles_queue(TFiles *tfiles, const char *name) */
/* { */
/*    int result; */
/*    int namelen = strlen(name); */

/*    char name_root[namelen + 4]; */
/*    memcpy(name_root, name, namelen); */
/*    strcpy(&name_root[namelen], ".db"); */

/*    char name_sndx[namelen + 5]; */
/*    memcpy(name_sndx, name, namelen); */
/*    strcpy(&name_sndx[namelen], ".s2i"); */

/*    if ((result = open_table(&table->t_strings, O_Queue, name_root, 1, 45))) */
/*       goto abandon_function; */

/*    if ((result = open_table(&table->t_strndx, O_Index_S2I, name_sndx, 1, 0))) */
/*       goto abandon_root; */

/*    return result; */

/*   abandon_root: */
/*    table->t_strings->close(table->t_strings); */

/*   abandon_function: */
/*    return result; */
/* } */

/* void process_tables_group(const char **ptr, const char **end) */
/* { */
/*    TFiles tfiles; */
/*    memset(tfiles, 0, sizeof(TFiles)); */

/*    open_tfiles_queue(&tfiles, "thesaurus"); */
/* } */

void process_single_table(const char **ptr, const char **end)
{
   Result result;
   Table table;
   RecID recid;
   RecPair pair;

   /* if (!(result = open_table(&table, O_Queue, "mytable.db", 1, 45))) */
   if (!(result = open_table(&table, O_Recno, "mytable.db", 1, 0)))
   {
      printf("Opened the table.\n");

      while (++ptr < end)
      {
         if ((result = table.add(&table, &recid, set_put_string(&pair, *ptr))))
         {
            table.db->err(table.db, result, "Adding record.");
            break;
         }
         else
            printf("Successfully added \"%s\" at recid %u.\n",
                   (char*)pair.value.data,
                   recid);
            
      }

      dump_table(&table, dump_string_record, NULL);

      table.close(&table);
   }
   else
      table.db->err(table.db, result, "Opening mytable.db.");
}


int main(int argc, const char **argv)
{
   process_single_table(argv, argv+argc);

   
   
   printf("Got call, man.\n");
   return 0;
}

#endif

