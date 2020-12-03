#ifndef BDB_H
#define BDB_H

typedef unsigned int u_int;
typedef unsigned long u_long;

#include <db.h>

typedef u_int32_t RecID;
typedef u_int32_t RecLen;
typedef u_int32_t boolean, bool;
typedef u_int32_t DBFlags;

// Bundle key and value for ease of setting and passing
typedef struct rec_pair_t {
   DBT key;
   DBT value;
} RecPair;

// Function pointer typedefs for Table member functions
typedef int (*db_open_f)(void *this, const char *name, bool create, RecLen reclen);
typedef void (*db_close_f)(void *this);
typedef int (*db_add_record_f)(void *this, RecID *recid, RecPair *pair);
typedef RecID (*add_string_f)(void *this, RecID *recid, const char *value);

typedef struct class_table {
   DB              *db;
   db_close_f      close;
   db_add_record_f add;
} Table;
Table table_base;

struct class_string_table {
   Table        table;
   add_string_f adder;
};


// DBT-setting functions
void init_pair(RecPair *pair);
void set_dbt_str(DBT *dbt, const char *str);
RecPair* set_put_string(RecPair *pair, const char *str);
RecPair* set_put_string_index(RecPair *pair, const char *str, RecID *rec);



typedef int(*Opener)(Table *table, const char *name, bool create, RecLen reclen);

// Built-in implementations of the function pointer type Opener.  Pass one of
// these functions, or a custom one of your own, to the open_table function.
int O_Queue(Table *table, const char *name, bool create, RecLen reclen);
int O_Recno(Table *table, const char *name, bool create, RecLen reclen);
int O_Index_S2I(Table *table, const char *name, bool create, RecLen reclen);
int O_Index_I2I(Table *table, const char *name, bool create, RecLen reclen);


// basic functions
int open_table(Table *table, Opener opener, const char *name, bool create, RecLen reclen);


// Utilities
typedef int(*Dumpster)(DBT *key, DBT *value, void *data);
void dump_table(Table *table, Dumpster dumpster, void *data);


#endif

