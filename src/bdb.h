#ifndef BDB_H
#define BDB_H

// u_int and u_long are assumed in <db.h>
typedef unsigned int u_int;
typedef unsigned long u_long;

#include <db.h>

typedef u_int32_t RecID;
typedef u_int32_t RecLen;
typedef u_int32_t DataSize;
typedef u_int32_t boolean, bool;
typedef u_int32_t DBFlags;
typedef int       Result;

// Bundle key and value for ease of setting and passing
typedef struct rec_pair_t {
   DBT key;
   DBT value;
} RecPair;

// Function pointer typedefs for Table member functions
typedef Result (*db_open_f)        (void *this, const char *name, bool create, RecLen reclen);
typedef void   (*db_close_f)       (void *this);
typedef Result (*db_add_record_f)  (void *this, RecID *recid, RecPair *pair);
typedef RecID  (*add_string_f)     (void *this, RecID *recid, const char *value);
typedef Result (*db_get_setting_f) (void *this);
typedef bool   (*db_get_flag_f)    (void *this);

typedef struct class_table {
   DB               *db;
   db_close_f       close;
   db_add_record_f  add;
   db_get_flag_f    is_open;
} Table;

// DBT-setting section
typedef struct pairset {
   void *key_data;
   RecLen key_size;
   void *value_data;
   RecLen value_size;
} PairSet;

DBT *set_dbt(DBT *dbt, void *data, DataSize size);
void init_pair(RecPair *pair);
RecPair* set_pair(RecPair *pair, PairSet ps);
void set_dbt_str(DBT *dbt, const char *str);
RecPair* set_put_string(RecPair *pair, const char *str);
RecPair* set_put_string_index(RecPair *pair, const char *str, RecID *rec);



typedef int(*Opener)(Table *table, const char *name, bool create, RecLen reclen);

// Built-in implementations of the function pointer type Opener.  Pass one of
// these functions, or a custom one of your own, to the open_table function.
Result O_Queue(Table *table, const char *name, bool create, RecLen reclen);
Result O_Recno(Table *table, const char *name, bool create, RecLen reclen);
Result O_Index_S2I(Table *table, const char *name, bool create, RecLen reclen);
Result O_Index_I2I(Table *table, const char *name, bool create, RecLen reclen);


// basic functions
int open_table(Table *table, Opener opener, const char *name, bool create, RecLen reclen);


// Utilities
typedef bool (*Dumpster)(DBT *key, DBT *value, void *data);
void dump_table(Table *table, Dumpster dumpster, void *data);




#include "istringt.h"


#endif

