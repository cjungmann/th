#ifndef IVTABLE_H
#define IVTABLE_H

#include "bdb.h"

typedef struct ivtable_t IVTable;

typedef Result(*ivt_open)          (IVTable *t, const char *name);
typedef void  (*ivt_close)         (IVTable *t);
typedef Result(*ivt_get_recid_raw) (IVTable *t, RecID *id, DBT *key);
typedef Result(*ivt_add_record_raw)(IVTable *t, RecID *id, DBT *key, DBT *value);
typedef RecID (*ivt_get_recid)     (IVTable *t, void* data, DataSize size);
typedef RecID (*ivt_add_record)    (IVTable *t, RecPair *pair);
typedef Result(*ivt_update_record) (IVTable *t, RecID id, DBT *value);

typedef Result(*ivt_get_record_by_recid) (IVTable *t, RecID id, DBT *value);
typedef Result(*ivt_get_record_by_key)   (IVTable *t, RecID *id, DBT *key, DBT *value);

typedef DB*   (*ivt_get_db) (IVTable *t);

struct ivtable_t {
   Table t_records;
   Table t_index;

   Opener opener;
   RecLen reclen;

   ivt_open            open;
   ivt_close           close;
   ivt_get_recid_raw   get_recid_raw;
   ivt_add_record_raw  add_record_raw;
   ivt_get_recid       get_recid;
   ivt_add_record      add_record;
   ivt_update_record  update_record;

   ivt_get_record_by_recid get_record_by_recid;
   ivt_get_record_by_key   get_record_by_key;

   ivt_get_db     get_records_db;
   ivt_get_db     get_index_db;
};

void init_IVTable(IVTable *t);
void ivt_make_queue_table(IVTable *t, RecLen reclen);



#endif
