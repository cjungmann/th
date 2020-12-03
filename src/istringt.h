#ifndef ISTRING_H
#define ISTRING_H

#include "bdb.h"

typedef struct istring_t IStringT;

typedef void (*ist_init)(IStringT *t);
typedef int (*ist_open)(IStringT *t, const char *name);
typedef void (*ist_close)(IStringT *t);
typedef RecID (*ist_get_recid)(IStringT *t, const char *str);
typedef int (*ist_put_string)(IStringT *t, RecID *id, const char *str);
typedef DB* (*ist_get_db)(IStringT *t);
typedef DB* (*ist_get_ndx_db)(IStringT *t);

struct istring_t {
   Table t_strings;
   Table t_strndx;

   Opener opener;
   int reclen;

   // Methods
   ist_init        init;
   ist_open        open;
   ist_close       close;
   ist_put_string  put_string;
   ist_get_recid   get_recid;
   ist_get_db      get_db;
   ist_get_ndx_db  get_ndx_db;
};
void init_istringt(IStringT *t);


#endif
