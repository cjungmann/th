#include "rrtable.h"
#include <string.h>

Result rrt_open(DB **db_out, bool create, const char *name)
{
   Result result;
   DB *db;

   if ((result = db_create(&db, NULL, 0)))
      goto abandon_function;

   if ((result = db->set_flags(db, DB_DUPSORT)))
      goto abandon_db;

   DBFlags dbflags = create ? (DB_CREATE | DB_OVERWRITE) : 0;

   if ((result = db->open(db, NULL, name, NULL, DB_BTREE, dbflags, 0664)))
      goto abandon_db;

   *db_out = db;
   goto abandon_function;

  abandon_db:
   db->close(db, 0);

  abandon_function:
   return result;
}

Result rrt_add_link(DB *db, RecID left, RecID right)
{
   DBT key;
   set_dbt(&key, &left, sizeof(RecID));
   DBT value;
   set_dbt(&value, &right, sizeof(RecID));

   return db->put(db, NULL, &key, &value, DB_NODUPDATA);
}

// cycle_cursor() Closure used by rtt_get_list()
typedef struct get_list_closure {
   RecID id;
   int idcount;
   RecID *list;
} GLC;

// record-count cycle_cursor() callback for rtt_get_list()
bool glc_counter(DBT *key, DBT *value, void *closure)
{
   GLC *glc = (GLC*)closure;

   if (*(RecID*)key->data == glc->id)
   {
      ++glc->idcount;
      return 1;
   }

   return 0;
}

// element-writting cycle_cursor() callback for rtt_get_list()
bool glc_list_adder(DBT *key, DBT *value, void *closure)
{
   GLC *glc = (GLC*)closure;

   if (*(RecID*)key->data == glc->id)
   {
      *glc->list = *(RecID*)value->data;
      ++glc->list;
      return 1;
   }

   return 0;
}

Result rrt_get_list(DB *db, RecID id, rrt_list_user user, void *closure)
{
   Result result = 0;
   DBT key;
   set_dbt(&key, &id, sizeof(RecID));
   DBT value;
   memset(&value, 0, sizeof(DBT));

   DBC *cursor;
   if ((result = db->cursor(db, NULL, &cursor, 0)))
      goto abandon_function;

   *(RecID*)key.data = id;
   if ((result = cursor->get(cursor, &key, &value, DB_SET)))
      goto abandon_cursor;

   GLC glc = {id};
   if ((result = cycle_cursor(cursor, &key, glc_counter, &glc)))
      goto abandon_cursor;

   if (glc.idcount)
   {
      // Reposition cursor at beginning of records
      *(RecID*)key.data = id;
      if ((result = cursor->get(cursor, &key, &value, DB_SET)))
         goto abandon_cursor;

      RecID list[glc.idcount];
      glc.list = list;
      
      if ((result = cycle_cursor(cursor, &key, glc_list_adder, &glc)))
         goto abandon_cursor;

      (*user)(list, glc.idcount, closure);
   }

  abandon_cursor:
   cursor->close(cursor);
   
  abandon_function:
   return 0;
}

RRTable_Class RRT = {
   rrt_open,
   rrt_add_link,
   rrt_get_list
};




#ifdef RRTABLE_MAIN

#include <stdio.h>
#include "utils.c"

void list_user(RecID *list, int count, void *closure)
{
   printf("We got %d RecIDs.\n", count);
   RecID *ptr = list;
   RecID *end = list + count;

   while (ptr < end)
   {
      printf("  %u.\n", *ptr);
      ++ptr;
   }
}

int main(int argc, const char **argv)
{
   const char *ctxt = "Adding a record";
   DB *db;
   if (!rrt_open(&db, 1, "rrtable.db"))
   {
      bdberr(rrt_add_link(db, 1, 2), stderr, ctxt);
      bdberr(rrt_add_link(db, 1, 3), stderr, ctxt);
      bdberr(rrt_add_link(db, 1, 4), stderr, ctxt);
      bdberr(rrt_add_link(db, 5, 6), stderr, ctxt);
      bdberr(rrt_add_link(db, 5, 7), stderr, ctxt);
      bdberr(rrt_add_link(db, 5, 8), stderr, ctxt);
      bdberr(rrt_add_link(db, 5, 9), stderr, ctxt);
      bdberr(rrt_add_link(db, 5, 10), stderr, ctxt);
      bdberr(rrt_add_link(db, 5, 11), stderr, ctxt);
      bdberr(rrt_add_link(db, 12, 13), stderr, ctxt);
      bdberr(rrt_add_link(db, 12, 14), stderr, ctxt);
      bdberr(rrt_add_link(db, 12, 15), stderr, ctxt);
      bdberr(rrt_add_link(db, 12, 16), stderr, ctxt);

      rrt_get_list(db, 5, list_user, NULL);

      db->close(db, 0);
   }

   return 0;
}


#endif



/* Local Variables: */
/* compile-command: "b=rrtable; \*/
/*  cc -Wall -Werror -ggdb      \*/
/*  -std=c99 -pedantic          \*/
/*  -D${b^^}_MAIN -o $b ${b}.c  \*/
/*  -ldb"                       \*/
/* End: */
