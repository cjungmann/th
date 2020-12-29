#ifndef RRTABLE_H
#define RRTABLE_H

/*****************************************************************
 * RRT designates a RecID to RecID Table.
 *
 * An RRT allows a single RecID key to refer to many RecID values.
 * While there cannot be a key-value duplicate, this table allows
 * for recording many-to-one relationships.
 ****************************************************************/

#include "bdb.h"

typedef void (*rrt_list_user)(RecID *list, int count, void *closure);

Result rrt_open(DB **db_out, bool create, const char *name);
Result rrt_add_link(DB *db, RecID left, RecID right);
Result rrt_get_list(DB *db, RecID id, rrt_list_user user, void *closure);


#endif
