#ifndef RRTABLE_H
#define RRTABLE_H

#include "bdb.h"

typedef void (*rrt_list_user)(RecID *list, int count, void *closure);

typedef Result (*rrt_open_m)(DB **db, bool create, const char *name);
typedef Result (*rrt_add_link_m)(DB *db, RecID left, RecID right);
typedef Result (*rrt_get_list_m)(DB *db, RecID id, rrt_list_user user, void *closure);

typedef struct rrt_class {
   rrt_open_m     open;
   rrt_add_link_m add_link;
   rrt_get_list_m get_list;
} RRTable_Class;



Result rrt_open(DB **db_out, bool create, const char *name);
Result rrt_add_link(DB *db, RecID left, RecID right);
Result rrt_get_list(DB *db, RecID id, rrt_list_user user, void *closure);

extern RRTable_Class RRT;

#endif
